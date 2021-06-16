#include <stdlib.h>
#include <time.h>

#include "applibs_versions.h"
#include <applibs/log.h>

#include "heap_tracker_lib.h"

// define one of three macro to test malloc or calloc or aligned_alloc
#define TEST_MALLOC
//#define TEST_CALLOC
//#define TEST_ALIGNED_ALLOC

// deifne this to add additional realloc after above call
#define TEST_REALLOC

// define this to simulate a leakage
#define SIMULATE_LEAKAGE

// threshold to alarm in example
#define CFG_HEAP_THRESHOLD					(1024 * 100)

int main(void)
{
    int n;
    char* ptr;
    ssize_t allocated;
    ssize_t peak_allocated;
    size_t mismatch_call;

    Log_Debug("Starting Heap Tracker test application...\n");

    heap_tracker_init();

    srand(time(NULL));

    while (1) {

        n = rand() % 1024;

#if defined (TEST_MALLOC)
        ptr = malloc(n);
#elif defined(TEST_CALLOC)
        ptr = calloc(1, n);
#elif defined(TEST_ALIGNED_ALLOC)
        if (n % 2 == 0) {
            ptr = aligned_alloc(n, n);        // alignment must be power of 2, size must be a multiple of alignemtn
        } else {
            continue;
        }
#endif

#if defined(TEST_REALLOC)
#if defined(CFG_HEAP_TRACKER_COMPATIBLE_API)
        ptr = realloc(ptr, n);
#else
        ptr = heap_tracker_realloc(ptr, n, n);
#endif
#endif

#if defined(SIMULATE_LEAKAGE)
        if (n % 43 != 0)                // A number defined to simulate the (how frequent) leakage
#endif 

#if defined(CFG_HEAP_TRACKER_COMPATIBLE_API)
            free(ptr);                  // if CFG_HEAP_TRACKER_COMPATIBLE_API is defined, user can use normal free()
#else
            heap_tracker_free(ptr, n);
#endif

        allocated = heap_tracker_get_allocated();
        peak_allocated = heap_tracker_get_peak_allocated();
        mismatch_call = heap_tracker_get_number_of_mismatch();

        Log_Debug("INFO: allocated: (%d) bytes, peak allocated: (%d) bytes, mismatch calls: (%d)\n", allocated, peak_allocated, mismatch_call);
        if ((allocated > CFG_HEAP_THRESHOLD) || (peak_allocated > CFG_HEAP_THRESHOLD)) {
            Log_Debug("WARNING: allocated memory is/was above limit!\n");
            break;
        }
    }

    return 0;
}