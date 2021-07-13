# Heap Tracker library

HeapTracker is a thin-layer library that implements a custom heap tracking mechanism and provide interfaces to user to understand memory allocation statistic data, includ current allocated memory, peak allocated memory and number of mismatched malloc/free call. The library accomplishes this by overriding the following native C memory allocation functions, though the standard GNU C Library wrapping mechanism:

> `malloc()`, `realloc()`, `calloc()`, `alloc_aligned()` and `free()`

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
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://docs.microsoft.com/en-us/azure-sphere/install/overview) for more information).

## How to use

1. The following options must be added to the `target_link_libraries` option in CMakeLists.txt:

    ```cmake
    -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=free -Wl,--wrap=aligned_alloc -Wl,--wrap=realloc
    ```
    For example, in `CMakeLists.txt`:

    ```cmake
    ...
    target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=free -Wl,--wrap=aligned_alloc -Wl,--wrap=realloc)
    ...
    ```

2. In the library's header file `heap_tracker_lib.h`, user can manipulate following macro definitions to configure how the library works.

    | Macro | Purpose |
    |---------|---------|
    | CFG_HEAP_TRACKER_COMPATIBLE_API | Use compatible API or not
    | CFG_HEAP_TRACKER_DEBUG | Enable/Disable per call logs
    | CFG_HEAP_TRACKER_TRHEADSAFE | Enable/Disable thread safe mutex

3. If CFG_HEAP_TRACKER_COMPATIBLE_API is defined. Use the `malloc()`, `calloc()`, `realloc()` and `free()` functions as usual in your application, **with the exception of `aligned_alloc()`**, which will always return NULL. In this configuration, extra bytes (16 bytes) is allocated for each malloc/calloc/realloc call in order to store size information for paired `free()` later. The good part is to allow user to keep original `free()` compatible so no modification is required, at the expense of a small overhead and no support to `aligned_alloc()` due to alignment requirement. 

4. If CFG_HEAP_TRACKER_COMPATIBLE_API is not defined, use the `malloc()`, `calloc()` and `alloc_aligned()` functions as usual in your application, **with the exception of `free()` and `realloc()`**, instead of which the `heap_tracker_free()` and `heap_tracker_realloc()` helpers **must** be used with addtional size parameters, in order to keep correct tracking of memory. If the native `free()` and `realloc()` functions were to be used, the statistic data given by heap tracker library cannot be considered reliable anymore.


5. User application should call `heap_tracker_init()` in the beginning of their initialization code (before any malloc/free calls)
   
6. User application make their own decision (when and where) to call `heap_tracker_get_allocated`, `heap_tracker_get_peak_allocated` and/or `heap_tracker_get_number_of_mismatch` to get the latest statistic data from heap tracker. 

## Example

The sample code in `main.c` cyclicly allocate random number of bytes by calling `malloc` or `calloc` or `aligned_alloc`, and `realloc`. Then the sample code free the memory block, print statistic data from heap tracker library, and evalaute if a user defined limit (default 100KB) is reached. For most of the cases, there will be no leakage. But the sample code will simulate a leakage condition (by not calling free) and gradually you will see more leakage and finally reach the limit. 

Log example

```

INFO: malloc 119 bytes @ 0xbeeecf00
INFO: realloc() 119 bytes @ 0xbeeecf00, from previous 119 bytes
INFO: free 119 bytes @ 0xbeeecf00
INFO: allocated: (0) bytes, peak allocated: (119) bytes, mismatch calls: (0)

...

INFO: malloc 227 bytes @ 0xbeeecf00
INFO: realloc() 227 bytes @ 0xbeeecf00, from previous 227 bytes
INFO: free 227 bytes @ 0xbeeecf00
INFO: allocated: (0) bytes, peak allocated: (1030) bytes, mismatch calls: (0)
INFO: malloc 317 bytes @ 0xbee7c410
INFO: realloc() 317 bytes @ 0xbee7c410, from previous 317 bytes
INFO: allocated: (317) bytes, peak allocated: (1030) bytes, mismatch calls: (1)
INFO: malloc 686 bytes @ 0xbee7c560
INFO: realloc() 686 bytes @ 0xbee7c560, from previous 686 bytes
INFO: free 686 bytes @ 0xbee7c560
INFO: allocated: (317) bytes, peak allocated: (1030) bytes, mismatch calls: (1)

...

INFO: malloc 754 bytes @ 0xbef14350
INFO: realloc() 754 bytes @ 0xbef14350, from previous 754 bytes
INFO: free 754 bytes @ 0xbef14350
INFO: allocated: (101709) bytes, peak allocated: (102463) bytes, mismatch calls: (189)
WARNING: allocated memory is/was above limit!

```

## Key concepts and limitations

The goal of the Heap Tracker library, is to support developers track their High-level application's memory requests to match the expected behavior throughout the application execution time. It only trackes the memory in user code and static libraries, and does not include usage from Azure Sphere OS library and other C libraries. There are some cases where the memory is allocated by a library but must be freed by user code, in this case `__real_free()` should be used instead of `free()` to avoid error message from heap tracker library. 

Known case:

- Pointer returned by `Storage_GetAbsolutePathInImagePackage()` API

## Next steps
For more information and recommendations on Azure Sphere memory usage and debugging, see [Memory use in high-level applications](https://docs.microsoft.com/en-us/azure-sphere/app-development/application-memory-usage).

## Project expectations

This library should be used during *development only*, for optimizing the memory usage in a High-level application. It is not recommended to be used productions versions of application.

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
