/*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "heap_tracker_lib.h"

#include "applibs_versions.h"
#include <applibs/log.h>


//////////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////////

// Sets the maximum allowed heap that can be allocated (in bytes) (set this variable based on practical tests on the App's memory usage).
const size_t heap_limit = 250 * 1024;

// Heap allocated amount (in bytes). This is signed so the user can debug de-allocation issues.
volatile ssize_t heap_allocated = 0;


//////////////////////////////////////////////////////////////////////////////////
// LOGGING
//////////////////////////////////////////////////////////////////////////////////

#define HEAP_TRACKER_LIB_LOG_PREFIX		"Heap-Tracker lib: "
#define HeapTracker_Log(...)			if (DEBUG_LOGS_ON) Log_Debug(HEAP_TRACKER_LIB_LOG_PREFIX __VA_ARGS__)
#define LogHeapStatus()					if (DEBUG_LOGS_ON) log_heap_status()

void log_heap_status(void)
{
	if (heap_allocated < 0)
	{
		Log_Debug("WARNING: current allocated memory (%zd) is NEGATIVE --> 'heap_allocated' will not be reliable from now on!\n", heap_allocated);
	}
	else if (heap_allocated > heap_limit)
	{
		Log_Debug("WARNING: allocated heap (%zd bytes) is above available heap_limit (%zu bytes)\n", heap_allocated, heap_limit);
	}
	else
	{
		Log_Debug("SUCCESS: available heap (%zd bytes)\n", (ssize_t)heap_limit - heap_allocated);
	}
}

//////////////////////////////////////////////////////////////////////////////////
// NATIVE malloc/free WRAPPERS
//////////////////////////////////////////////////////////////////////////////////
/*
*	NOTE: the library does not *intentionally *alter the behaviors of the native functions,
*	in order to not disrupt the functionality on other system libraries that use them.
*	Altering the implementations is NOT reccomended as it could result in unpredicted App behavior!
*/

// Native malloc/free stubs, which will be inter-positioned by the wrappers at link-time
void *__real_malloc(size_t size);
void *__real_realloc(void *ptr, size_t new_size);
void *__real_calloc(size_t num, size_t size);
void *__real_aligned_alloc(size_t alignment, size_t size);
void __real_free(void *ptr);


// Heap-tracking malloc() wrapper (tracks heap increse-only)
void *__wrap_malloc(size_t size)
{	
	void *ptr = __real_malloc(size);

	HeapTracker_Log("malloc(%zu)=%p... ", size, ptr);
	if (NULL != ptr)
	{
		heap_allocated += (ssize_t)size;
		//memset(ptr, 0, size); // Commit by accessing the pointer (because of Linux's optimistic memory allocation)
	}
	
	LogHeapStatus();

	return ptr;
}

// Custom heap-tracking calloc() wrapper (tracks heap increse-only)
void *__wrap_calloc(size_t num, size_t size)
{
	void *ptr = __real_calloc(num, size);

	HeapTracker_Log("calloc(%zu,%zu)=%p...", num, size, ptr);
	if (ptr)
	{
		heap_allocated += (ssize_t)(num * size);
		LogHeapStatus();
	}
	
	LogHeapStatus();

	return ptr;
}

// Custom heap-tracking aligned_alloc() wrapper (tracks heap increse-only)
void *__wrap_aligned_alloc(size_t alignment, size_t size)
{
	void *ptr = __real_aligned_alloc(alignment, size);

	HeapTracker_Log("aligned_alloc(%zu,%zu)=%p...", alignment, size, ptr);
	if (ptr)
	{
		heap_allocated += (ssize_t)size;
	}
	
	LogHeapStatus();

	return ptr;
}

// Custom heap-tracking realloc() wrapper (does NOT track heap as it is unaware of the previous memory block size!)
void *__wrap_realloc(void *ptr, size_t new_size)
{
	HeapTracker_Log("WARNING! Native realloc(%p,%zu) was called instead of _realloc() helper: 'heap_allocated' will not be reliable from now on!\n", ptr, new_size);
	return __real_realloc(ptr, new_size);
}

// Native free() wrapper (does NOT track heap!)
void __wrap_free(void *ptr)
{
	HeapTracker_Log("WARNING! Native free(%p) was called instead of _free() helper: 'heap_allocated' will not be reliable from now on!\n", ptr);
	__real_free(ptr);
}


//////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////////////////////////////////////////////////////////////////////////////

// Custom heap-tracking free() helper
void _free(void *ptr, size_t size)
{
	HeapTracker_Log("_free(%p,%zu)... ", ptr, size);

	__real_free(ptr);
	if (ptr)
	{
		heap_allocated -= (ssize_t)size;
	}

	LogHeapStatus();
}

// Custom heap-tracking realloc() helper
void *_realloc(void *ptr, size_t old_size, size_t new_size)
{
	void *new_ptr = __real_realloc(ptr, new_size);	
	
	HeapTracker_Log("_realloc(%p,%zu,%zu)=%p... ", ptr, old_size, new_size, new_ptr);
	if (NULL != new_ptr)
	{
		heap_allocated += (ssize_t)(new_size - old_size + 1);
	}

	LogHeapStatus();

	return new_ptr;
}