# Heap Tracker library

This library implements a custom heap tracking mechanism to provide the App a global `heap_allocated` variable that can be used to prevent OOM conditions.

The library accomplishes this by overriding the following native C memory functions, though the standard GNU C Library wrapping mechanism:
- `malloc()`, `realloc()`, `calloc()`, `alloc_aligned()` and `free()`


## Contents

| File/folder | Description |
|-------------|-------------|
|   main.c    | The sample App's source file. |
|   heap_tracker_lib.h    | Header source file for the heap custom memory management library. |
|   heap_tracker_lib.c    | Implementation source file for the heap custom memory management library. |
| app_manifest.json | The sample App's manifest file. |
| CMakeLists.txt | Contains the project information and produces the build, along with the memory-specific wrapping directives. |
| CMakeSettings.json| Configures CMake with the correct command-line options. |
|launch.vs.json | Tells Visual Studio how to deploy and debug the application.|
| README.md | This readme file. |
|.vscode | Contains settings.json that configures Visual Studio Code to use CMake with the correct options, and tells it how to deploy and debug the application. |

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [log.h](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Contains functions that log debug messages. |

## Prerequisites & Setup

- Any Azure Sphere-based device with development features.
- A development environment for Azure Sphere

See [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information.

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

2. In the library's header `heap_tracker_lib.h`, define wither verbose logs are enabled or not by setting `DEBUG_LOGS_ON` accordingly:

    ```c
    //////////////////////////////////////////////////////////////////////////////////
    // GLOBAL VARIABLES
    //////////////////////////////////////////////////////////////////////////////////
    #define DEBUG_LOGS_ON	1				// Enables(1)/Disables(0) verbose loggging
    extern const size_t		heap_limit;		// Sets the maximum allowed heap that can be allocated (in bytes).
    extern volatile ssize_t	heap_allocated;	// Currently allocated heap (in bytes). Note: this is NOT thread safe!
    ```

3. In the library's code `heap_tracker_lib.c`, define an initial value for the `heap_limit` constant. Please refer to [Memory available on Azure Sphere](https://docs.microsoft.com/en-us/azure-sphere/app-development/mt3620-memory-available) for more information on memory availability for High-level applications.

4. Use malloc() as usual in your App, BUT use the *_free()* and *_realloc()* helpers, in order to keep track of the available heap. If the native *free()* and *realloc()* functions will be used in the App, then the internal heap counter will become invalid, and the `heap_allocated` counter will become unreliable.

    **NOTE**: the library does not *intentionally* alter the behaviors of the native functions, in order to not disrupt the functionality on other system libraries that use them. **Altering the implementations is *not* recommended as it could result in unpredicted App behavior**.

5. Determine, by trial & error, what is the maximum heap usage for your App, and set the `heap_limit` constant accordingly.

## Example

The sample code in `main.c` will cyclicly grow in heap memory allocation by calling *getFreeHeapMemory_malloc* or *getFreeHeapMemory_realloc* (depending on what's uncommented in the `main()` preprocessor block), and fetch the remaining free heap memory up to the limit that has been set in the `heap_limit` variable:

- On success, *getFreeHeapMemory_malloc* or *getFreeHeapMemory_realloc* return the amount of memory (in bytes) successfully allocated by the incremental malloc()/realloc() attempts. If `DEBUG_LOGS_ON` is set to `1` (in `heap_tracker_lib.h`), the following output will be generated:

    ```c
    Remote debugging from host 192.168.35.1, port 63571
    Starting Heap Library test application...
    getFreeHeapMemory_malloc --> Heap status: max available(143360 bytes), allocated (0 bytes)
    Heap Tracker lib: malloc(1024)... INFO: available heap (142336 bytes)
    Heap Tracker lib: _free(0xbeeebb40, 1024)... INFO: available heap (143360 bytes)
    Heap Tracker lib: malloc(2048)... INFO: available heap (141312 bytes)
    Heap Tracker lib: _free(0xbee84640, 2048)... INFO: available heap (143360 bytes)
    ...
    ...
    Heap Tracker lib: malloc(143360)... INFO: available heap (0 bytes)
    Heap Tracker lib: _free(0xbedf5010, 143360)... INFO: available heap (143360 bytes)
    Heap Tracker lib: malloc(144384)... WARNING: allocated heap (144384l bytes) is above available heap_limit (143360 bytes)
    Heap Tracker lib: _free(0xbedf5010, 144384)... INFO: available heap (143360 bytes)
    getFreeHeapMemory_malloc --> Currently free heap: 141Kb (144384 bytes)
    ```

- On failure, the OS's OOM Killer will SIGKILL the App's process. This will be the indicator that the global variable `heap_limit` in `heap_tracker_lib.c` should be lowered to a value below to the last successful allocation:

    ```c
    Remote debugging from host 192.168.35.1, port 61736
    Starting Heap Tracker test application...
    getFreeHeapMemory_realloc --> Heap status: max available(256000 bytes), allocated (0 bytes)
    Heap-Tracker lib: _realloc(0,0,1024)=0xbee7b010... SUCCESS: available heap (254975 bytes)
    Heap-Tracker lib: _realloc(0xbee7b010,1024,2048)=0xbee7b010... SUCCESS: available heap (253950 bytes)
    Heap-Tracker lib: _realloc(0xbee7b010,2048,3072)=0xbeee9010... SUCCESS: available heap (252925 bytes)
    ...
    Heap-Tracker lib: _realloc(0xbeeff010,188416,189440)=0xbeeff010... SUCCESS: available heap (66375 bytes)
    Heap-Tracker lib: _realloc(0xbeeff010,189440,190464)=0xbeeff010... SUCCESS: available heap (65350 bytes)
    Heap-Tracker lib: _realloc(0xbeeff010,190464,191488)=0xbeeff010... SUCCESS: available heap (64325 bytes)

    Child terminated with signal = 0x9 (SIGKILL)

    ```

## Key concepts
The goal in using the Heap Tracker library, is to best-estimate the *actual* memory available to your application, in a more precise manner. This because the Linux OS (upon which the Azure sphere OS is based) manages memory with an [optimistic memory allocation strategy](https://man7.org/linux/man-pages/man3/malloc.3.html), by means of which the OS almost never returns a `null` pointer upon memory allocation requests that would i.e. absolutely go beyond the device's availability. This is not an "overlooked" behavior in the Os, but for the OS to actually cope with the 100s/1000s of allocation requests, it'll "speculate" that statistically not all memory requests will actually be committed.

Things though get tricky when dealing with memory on constrained devices, as often the App *must* be aware of its heap availability, in order to implement correlated strategies & behaviors. An Azure Sphere HL-Core Apps run in the OS's user space, therefore the developer must rely on his best estimation upon the memory consumption that his App will have. This is the problem that this library is aiming at supporting to solve.

## Next steps

- For more information and recommendations on Azure Sphere memory usage and debugging, see [Memory use in high-level applications](https://docs.microsoft.com/en-us/azure-sphere/app-development/application-memory-usage).


## Expected support for the code
There is no official support guarantee for this code, but we will make a best effort to respond to/address any issues you encounter.

## How to report an issue
If you run into an issue with this script, please open a GitHub issue against this repo.

## License

For information about the licenses that apply to this script, see [LICENSE.txt](./LICENCE.txt)
