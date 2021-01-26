# Heap Tracker library

HeapTracker is a thin-layer library that implements a custom heap tracking mechanism which provides a global `heap_allocated` variable that can be used track the memory requests being done by the High-level application. The library accomplishes this by overriding the following native C memory allocation functions, though the standard GNU C Library wrapping mechanism:
- `malloc()`, `realloc()`, `calloc()`, `alloc_aligned()` and `free()`

## Contents

| File/folder | Description |
|-------------|-------------|
| main.c    | The sample App's source file. |
| heap_tracker_lib.h    | Header source file for the heap tracking library. |
| heap_tracker_lib.c    | Implementation source file for the heap tracking library. |
| app_manifest.json | The sample App's manifest file. |
| CMakeLists.txt | Contains the project information and produces the build, along with the memory-specific wrapping directives. |
| CMakeSettings.json| Configures CMake with the correct command-line options. |
| README.md | This readme file. |
| .vscode | Contains launch.json and settings.json that configures Visual Studio Code to use CMake with the correct options, and tells it how to deploy and debug the application. |

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [log.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Contains functions that log debug messages. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://docs.microsoft.com/en-us/azure-sphere/install/overview) for more information).

## How to use

1. The following two options must be added to the `target_link_libraries` option in CMakeLists.txt:

    ```cmake
    -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc -Wl,--wrap=alloc_aligned -Wl,--wrap=free
    ```
    For example, in `CMakeLists.txt`:

    ```cmake
    ...
    target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c -Wl,--wrap=malloc -Wl,--wrap=realloc -Wl,--wrap=calloc -Wl,--wrap=alloc_aligned -Wl,--wrap=free)
    ...
    ```

2. In the library's header file `heap_tracker_lib.h`, define wither verbose logs are enabled or not by setting `DEBUG_LOGS_ON` accordingly:

    ```c
    //////////////////////////////////////////////////////////////////////////////////
    // GLOBAL VARIABLES
    //////////////////////////////////////////////////////////////////////////////////
    #define DEBUG_LOGS_ON	1				// Enables(1)/Disables(0) verbose loggging
    extern const size_t		heap_threshold;	// Sets a reference allocation threshold (in bytes) after which the library will log warnings.
    extern volatile ssize_t	heap_allocated;	// Currently allocated heap (in bytes). Note: this is NOT thread safe!
    ```

3. In the library's implementation file `heap_tracker_lib.c`, define an initial value for the `heap_threshold` constant. Please refer to [Memory available on Azure Sphere](https://docs.microsoft.com/en-us/azure-sphere/app-development/mt3620-memory-available) for more information on memory availability for High-level applications.

4. Use the `malloc()`, `calloc()` and `alloc_aligned()` functions as usual in your App, **with the exception of `free()` and `realloc()`**, instead of which the `_free()` and `_realloc()` helpers **must** be used, in order to keep correct tracking within the `heap_allocated` variable. If the native `free()` and `realloc()` functions were to be used in the App, the `heap_allocated` variable cannot be considered reliable anymore.

5. Track and monitor the `heap_allocated` value.

## Example

The sample code in `main.c` will cyclicly grow in heap memory allocation by calling *consumeHeap_malloc* or *consumeHeap_realloc* (depending on what's uncommented in the `main()` preprocessor block), and fetch the remaining free heap memory up to the limit that has been set in the `heap_threshold` variable:

- On success, *getFreeHeapMemory_malloc* or *consumeHeap_realloc* return the amount of memory (in bytes) successfully allocated by the incremental malloc()/realloc() attempts, up to the given `heap_threshold`. If `DEBUG_LOGS_ON` is set to `1` (in `heap_tracker_lib.h`), the following output will be generated:

    ```c
    Remote debugging from host 192.168.35.1, port 60851
    Starting Heap Tracker test application...
    consumeHeap_malloc --> Heap status: max available(256000 bytes), allocated (0 bytes)
    Heap-Tracker lib: malloc(1024)=0xbeee9010... SUCCESS: heap_allocated (1024 bytes) - delta with heap_threshold(254976 bytes)
    Heap-Tracker lib: _free(0xbeee9010,1024)... SUCCESS: heap_allocated (0 bytes) - delta with heap_threshold(256000 bytes)
    Heap-Tracker lib: malloc(2048)=0xbee7b3e0... SUCCESS: heap_allocated (2048 bytes) - delta with heap_threshold(253952 bytes)
    ...
    ...
    Heap-Tracker lib: _free(0xbedcf010,256000)... SUCCESS: heap_allocated (0 bytes) - delta with heap_threshold(256000 bytes)
    Heap-Tracker lib: malloc(257024)=0xbedcf010... WARNING: heap_allocated (257024 bytes) is above heap_threshold (256000 bytes)
    Heap-Tracker lib: _free(0xbedcf010,257024)... SUCCESS: heap_allocated (0 bytes) - delta with heap_threshold(256000 bytes)
    consumeHeap_malloc --> Currently available heap up to given heap_threshold: 251Kb (257024 bytes)
    ```

- On failure, the OS's OOM Killer will SIGKILL the App's process. This will be the indicator that the global variable `heap_threshold` in `heap_tracker_lib.c` should be lowered to a value below to the last successful allocation:

    ```c
    Remote debugging from host 192.168.35.1, port 60827
    Starting Heap Tracker test application...
    consumeHeap_realloc --> Heap status: max available(256000 bytes), allocated (0 bytes)
    Heap-Tracker lib: _realloc(0,0,1024)=0xbeee9010... SUCCESS: heap_allocated (1025 bytes) - delta with heap_threshold(254975 bytes)
    Heap-Tracker lib: _realloc(0xbeee9010,1024,2048)=0xbeee9010... SUCCESS: heap_allocated (2050 bytes) - delta with heap_threshold(253950 bytes)
    Heap-Tracker lib: _realloc(0xbeee9010,2048,3072)=0xbeee9010... SUCCESS: heap_allocated (3075 bytes) - delta with heap_threshold(252925 bytes)
    ...
    ...
    Heap-Tracker lib: _realloc(0xbeeff010,189440,190464)=0xbeeff010... SUCCESS: heap_allocated (190650 bytes) - delta with heap_threshold(65350 bytes)
    Heap-Tracker lib: _realloc(0xbeeff010,190464,191488)=0xbeeff010... SUCCESS: heap_allocated (191675 bytes) - delta with heap_threshold(64325 bytes)

    Child terminated with signal = 0x9 (SIGKILL)
    ```

## Key concepts

The goal of the Heap Tracker library, is to support developers track their High-level application's memory requests to match the expected behavior throughout the application execution time (i.e. a constant raise in value of `heap_allocated` may indicate a potential memory leak).

## Next steps
For more information and recommendations on Azure Sphere memory usage and debugging, see [Memory use in high-level applications](https://docs.microsoft.com/en-us/azure-sphere/app-development/application-memory-usage).

## Project expectations

This library should be used during *development only*, for optimizing the memory usage in a High-level application. It is not recommended to be used productions versions of Apps.

### Expected support for the code

There is no official support guarantee for this code, but we will make a best effort to respond to/address any issues you encounter.

### How to report an issue

If you run into an issue with this script, please open a GitHub issue against this repo.

## Contributing

This project welcomes contributions and suggestions. Most contributions require you to
agree to a Contributor License Agreement (CLA) declaring that you have the right to,
and actually do, grant us the rights to use your contribution. For details, visit
https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need
to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the
instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/)
or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## License

For information about the licenses that apply to this script, see [LICENSE.txt](./LICENCE.txt)
