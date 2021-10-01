# Heap Tracker library

HeapTracker is a thin-layer library that implements a custom heap tracking mechanism which provides a global `heap_allocated` variable that can be used to track memory requests being done by High-level applications. The library accomplishes this by overriding the following native C memory allocation functions, though the standard GNU C Library wrapping mechanism:
- `malloc()`, `realloc()`, `calloc()`, `alloc_aligned()` and `free()`

The library also implements an optional **pointer & size tracking** feature, which enables detecting which pointers are actually "measured" in accounting the memory usage balance, therefore excluding those memory allocations that are not performed by the code & libraries that are compiled with the HL App. One other benefit of the pointer tracking feature, is that the library will use standard `free()` & `realloc()` instead of the overridden `_free()` & `_realloc()`, which must be used if pointer tracking is disabled (as pointer sizes are unknown).

Because pointer tracking implies an additional computing overhead, an optional **thread-safety** feature is also implemented in order to make memory-function calls atomic, which is essential in case the HL App makes use of threads.
The additional tracking overhead though is only appreciable upon usages of `free()` & `realloc()`, which on a positive side puts the performance decrease exactly where developers normally put their attention on, especially when developing on embedded systems, that is, reducing the "chatter" of frequent allocations/de-allocations of memory. Therefore, a significant performance decrease while using the HeapTracker may be a likely indication that there's room for performance improvements in the App's memory management.

## Contents

| File/folder | Description |
|-------------|-------------|
| main.c    | The library's sample App source file. |
| heap_tracker_lib.h    | Header source file for the heap tracking library. |
| heap_tracker_lib.c    | Implementation source file for the heap tracking library. |
| app_manifest.json | The sample App's manifest file. |
| CMakeLists.txt | Contains the project information and produces the build, along with the memory-specific wrapping directives. |
| CMakeSettings.json| Configures CMake with the correct command-line options. |
| README.md | This readme file. |
| .vscode | Contains launch.json and settings.json that configures Visual Studio Code to use CMake with the correct options, and tells it how to deploy and debug the application. |

The library uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [log.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Contains functions that log debug messages. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quick-starts to set up your Azure Sphere device](https://docs.microsoft.com/en-us/azure-sphere/install/overview) for more information).

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

2. In the library's header file `heap_tracker_lib.h`, configure the defines as per what documented in the comments:

    ```c
    //////////////////////////////////////////////////////////////////////////////////
    // GLOBAL VARIABLES
    //////////////////////////////////////////////////////////////////////////////////
    #define ENABLE_DEBUG_VERBOSE_LOGS               1   // Enables(1)/Disables(0) verbose logging.
    #define ENABLE_THREAD_SAFETY                    1   // Enables(1)/Disables(0) thread safety.
    #define ENABLE_POINTER_TRACKING                 1   // Enables(1)/Disables(0) pointer tracking.
    #define POINTER_TRACK_INC                       50  // Defines the growth size (in # of elements) for the internal pointer tracking array, 
                                                        // once the number of allocated pointers overflows the current array size.
    extern const size_t		heap_threshold;             // Sets a reference allocation threshold (in bytes) after which the library will log warnings.
    extern volatile ssize_t	heap_allocated;             // Currently allocated heap (in bytes).
    ```

3. In the library's implementation file `heap_tracker_lib.c`, define an initial value for the `heap_threshold` constant. Please refer to [Memory available on Azure Sphere](https://docs.microsoft.com/en-us/azure-sphere/app-development/mt3620-memory-available) for more information on memory availability for High-level applications.

4. Use the `malloc()`, `calloc()` and `alloc_aligned()` functions as usual in your App, **with the exception of `free()` and `realloc()`*, which depending on if:
    - `ENABLE_POINTER_TRACKING` is **disabled** (0), the `_free()` and `_realloc()` helpers **must** be used, in order to keep correct tracking within the `heap_allocated` variable. If the App uses the native `free()` and `realloc()` functions, the `heap_allocated` variable cannot be considered reliable anymore.
    - `ENABLE_POINTER_TRACKING` is **enabled** (1), the standard `free()` & `realloc()` functions can be used as normal, since the tracking mechanism will transparently store the size corresponding to each pointer allocated by user and statically linked code, an therefore consistently track memory deallocations within the `heap_allocated` variable.

5. Track and monitor the `heap_allocated` value.

    **Note**: when `ENABLE_POINTER_TRACKING` is enabled, attention should be placed to occurrences of the following log:

    ```
    WARNING: free(0x.......) was called for a non-tracked pointer.
    ```
    This indicates that a non-tracked pointer has been free-d in your code: this is not necessarily an issue, as the pointer may have been allocated by code not linked with the HL App (and free-d by the HL App as expected), but if the pointer is expected to have been allocated by the HL App, then this is something that the developer should investigate on.

## Example

The sample code in `main.c` will cyclically grow in heap memory allocation by calling *consumeHeap_malloc* or *consumeHeap_realloc* (depending on what's uncommented in the `main()` pre-processor block), and fetch the remaining free heap memory up to the limit that has been set in the `heap_threshold` variable:

- On success, *getFreeHeapMemory_malloc* or *consumeHeap_realloc* return the amount of memory (in bytes) successfully allocated by the incremental malloc()/realloc() attempts, up to the given `heap_threshold`. If `DEBUG_LOGS_ON` is set to `1` and `ENABLE_POINTER_TRACKING` set to `1` (in `heap_tracker_lib.h`), the following output will be generated:

    ```c
    Remote debugging from host 192.168.35.1, port 60851
    Starting Heap Tracker test application...
    consumeHeap_malloc --> Heap status: max available(256000 bytes), allocated (0 bytes)
    Heap-Tracker: malloc(1024)=0xbeee9010... SUCCESS: heap_allocated (1024 bytes) - delta with heap_threshold(254976 bytes)
    Heap-Tracker: malloc(2048)=0xbee7b3e0... SUCCESS: heap_allocated (2048 bytes) - delta with heap_threshold(253952 bytes)
    ...
    ...
    Heap-Tracker: malloc(257024)=0xbedcf010... WARNING: heap_allocated (257024 bytes) is above heap_threshold (256000 bytes)
    consumeHeap_malloc --> Currently available heap up to given heap_threshold: 251Kb (257024 bytes)
    ```

- On failure, the OS's OOM Killer will SIGKILL the App's process. This will be the indicator that the global variable `heap_threshold` in `heap_tracker_lib.c` should be lowered to a value below to the last successful allocation:

    ```c
    Remote debugging from host 192.168.35.1, port 51014
    Starting Heap Tracker test application...
    consumeHeap_realloc --> Heap status: max available(256000 bytes), allocated (0 bytes)
    Heap-Tracker: realloc(0, 1024)=0xbee7b420... SUCCESS: heap_allocated (1024 bytes) - delta with heap_threshold(254976 bytes)
    Heap-Tracker: realloc(0xbee7b420, 2048)=0xbee7b420... SUCCESS: heap_allocated (2048 bytes) - delta with heap_threshold(253952 bytes)
    Heap-Tracker: realloc(0xbee7b420, 3072)=0xbeee9010... SUCCESS: heap_allocated (3072 bytes) - delta with heap_threshold(252928 bytes)
    Heap-Tracker: realloc(0xbeee9010, 4096)=0xbeeff010... SUCCESS: heap_allocated (4096 bytes) - delta with heap_threshold(251904 bytes)
    ...
    ...
    Heap-Tracker: realloc(0xbedbc010, 235520)=0xbedbc010... SUCCESS: heap_allocated (235520 bytes) - delta with heap_threshold(20480 bytes)
    Heap-Tracker: realloc(0xbedbc010, 236544)=0xbedbc010... SUCCESS: heap_allocated (236544 bytes) - delta with heap_threshold(19456 bytes)
    Heap-Tracker: realloc(0xbedbc010, 237568)=0xbedbc010... 
    Child terminated with signal = 0x9 (SIGKILL)
    ```

## Key concepts

The goal of the Heap Tracker library, is to support developers track their High-level application's memory requests to match the expected behavior throughout the application execution time (i.e. a constant raise in value of `heap_allocated` may indicate a potential memory leak).

During development phases, if possible, it's recommended enabling the `ENABLE_POINTER_TRACKING` option, as this would allow to detect memory leaks that are caused by the code & static libraries linked in the HL App image, much more precisely.

**Note** since the GNU function wrapping (used by HeapTracker) only applies to the user code and linked static libraries, HeapTracker cannot track memory allocations within libraries that are i.e. embedded in the OS image or, more in general, not compiled with the HL App.

## Next steps
For more information and recommendations on Azure Sphere memory usage and debugging, see [Memory use in high-level applications](https://docs.microsoft.com/en-us/azure-sphere/app-development/application-memory-usage).

## Project expectations

This library should be used during *development only*, for optimizing the memory usage in a High-level application. It is not recommended to be used productions versions of Apps.

### Expected support for the code

There is no official support guarantee for this code, but we will make a best effort to respond to/address any issues you encounter.

### How to report an issue

If you run into an issue with this library, please open a GitHub issue within this repo.

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

For information about the licenses that apply to this library, see [LICENSE.txt](./LICENSE.txt)
