// This minimal Azure Sphere app repeatedly allocates & frees heap memory, 
// While displaying the tracked heap amount in the global heap_allocated variable. 
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


size_t consumeHeap_malloc(void)
{
    char *ptr = NULL;
    const size_t block_sz = 1024;
    size_t allocated = 0;

    Log_Debug("consumeHeap_malloc --> Heap status: max available(%zu bytes), allocated (%zd bytes)\n", heap_threshold, heap_allocated);

    while (true)
    {
        allocated += block_sz;
        ptr = malloc(allocated);
        if (heap_allocated > heap_threshold)
        {
            _free(ptr, allocated);
            break;
        }

        _free(ptr, allocated);
    }

    Log_Debug("consumeHeap_malloc --> Currently available heap up to given heap_threshold: %zuKb (%zu bytes)\n", allocated / block_sz, allocated);

    return allocated;
}

size_t consumeHeap_realloc(void)
{
    char *ptr = NULL, *new_ptr;
    const size_t block_sz = 1024;
    size_t allocated = 0;

    Log_Debug("consumeHeap_realloc --> Heap status: max available(%zu bytes), allocated (%zd bytes)\n", heap_threshold, heap_allocated);

    while (true)
    {
        new_ptr = _realloc(ptr, allocated, allocated + block_sz);
        if (heap_allocated > heap_threshold)
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

    Log_Debug("consumeHeap_realloc --> Currently available heap up to given heap_threshold: %zuKb (%zu bytes)\n", allocated / block_sz, allocated);

    return allocated;
}

int main(void)
{
    Log_Debug("Starting Heap Tracker test application...\n");

    const struct timespec sleepTime = {.tv_sec = 5, .tv_nsec = 0};

    while (true) {

#if (1)
        consumeHeap_malloc();
#else
        consumeHeap_realloc();
#endif
        nanosleep(&sleepTime, NULL);
    }

    return 0;
}