// This minimal Azure Sphere app repeatedly retrieves the available heap memory. 
//
// It uses the API for the following Azure Sphere application libraries:
// - log (messages shown in Visual Studio's Device Output window during debugging)

#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <assert.h>

#include "applibs_versions.h"
#include <applibs/log.h>

#include "heap_tracker_lib.h"


/// <summary>
///		Retrieves the "effective" free memory in the App's heap, by attempting 
///		maximum allocation possible with incremental malloc() attempts.
///		MUST ONLY USED DURING DEVELOPMENT, TO EXPERIMENTALLY FINE-TUNE THE 'heap_limit' VARIABLE!
/// </summary>
/// <param name="">none</param>
/// <returns>
///		On success, returns a size_t integer with the amount of memory (in bytes) successfully allocated by incremental malloc() attempts.
///		On failure, the OS will SIGKILL the App's process, and therefore the custom 'heap_limit' variable should be lowered.
/// </returns>
size_t getFreeHeapMemory_malloc(void)
{
    char *ptr = NULL;
    const size_t block_sz = 1024;
    size_t allocated = 0;

    Log_Debug("getFreeHeapMemory_malloc --> Heap status: max available(%zu bytes), allocated (%zd bytes)\n", heap_limit, heap_allocated);

    while (true)
    {
        allocated += block_sz;
        ptr = malloc(allocated);
        if (heap_allocated > heap_limit)
        {
            _free(ptr, allocated);
            break;
        }

        _free(ptr, allocated);
    }

    Log_Debug("getFreeHeapMemory_malloc --> Currently free heap: %zuKb (%zu bytes)\n", allocated / block_sz, allocated);

    return allocated;
}

size_t getFreeHeapMemory_realloc(void)
{
    char *ptr = NULL, *new_ptr;
    const size_t block_sz = 1024;
    size_t allocated = 0;

    Log_Debug("getFreeHeapMemory_realloc --> Heap status: max available(%zu bytes), allocated (%zd bytes)\n", heap_limit, heap_allocated);

    while (true)
    {
        new_ptr = _realloc(ptr, allocated, allocated + block_sz);
        if (heap_allocated > heap_limit)
        {
            if (new_ptr)
                _free(new_ptr, allocated + block_sz);
            else
                _free(ptr, allocated);
            break;
        }

        ptr = new_ptr;
        allocated += block_sz;
    }

    Log_Debug("getFreeHeapMemory_realloc --> Currently free heap: %zuKb (%zu bytes)\n", allocated / block_sz, allocated);

    return allocated;
}

int main(void)
{
    Log_Debug("Starting Heap Tracker test application...\n");

    const struct timespec sleepTime = {.tv_sec = 5, .tv_nsec = 0};

    while (true) {

#if (1)
        getFreeHeapMemory_malloc();
        nanosleep(&sleepTime, NULL);
#else
        getFreeHeapMemory_realloc();
        nanosleep(&sleepTime, NULL);
#endif
    }

    return 0;
}