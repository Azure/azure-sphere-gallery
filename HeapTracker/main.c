#include <stdlib.h>
#include <time.h>

#include "applibs_versions.h"
#include <applibs/log.h>

#include "heap_tracker_lib.h"

// define this to simulate a leakage
//#define SIMULATE_LEAKAGE

// threshold to alarm in example
#define CFG_HEAP_THRESHOLD					(1024 * 100)

int main(void)
{
    heap_tracker_init();

    Log_Debug("Starting Heap Tracker test application...\n");

    srand(time(NULL));

    while (1) {

        int n = rand() % 1024;
        char * ptr = malloc(n);

#ifdef SIMULATE_LEAKAGE
        if (n % 43 != 0)        // A number defined to simulate the (how frequent) leakage
#endif 
            free(ptr);

        ssize_t allocated = heap_tracker_get_allocated();
        ssize_t peak_allocated = heap_tracker_get_peak_allocated();
        size_t leakage = heap_tracker_get_leakage();

        Log_Debug("INFO: allocated: (%d) bytes, peak allocated: (%d) bytes, possible leakge calls: (%d)\n", allocated, peak_allocated, leakage);
        if ((allocated > CFG_HEAP_THRESHOLD) || (peak_allocated > CFG_HEAP_THRESHOLD)) {
            Log_Debug("WARNING: allocated memory is/was above limit!\n");
            break;
        }
    }

    return 0;
}