/*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "heap_tracker_lib.h"

#include "applibs_versions.h"
#include <applibs/log.h>


//////////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////////

// Sets a reference allocation threshold (in bytes) after which the library will log warnings.
const size_t heap_threshold = 250 * 1024;

// Heap allocated amount (in bytes). This is signed so the user can debug allocation issues.
volatile ssize_t heap_allocated = 0;


//////////////////////////////////////////////////////////////////////////////////
// LOGGING
//////////////////////////////////////////////////////////////////////////////////

#define HEAP_MAX_ALIGNMENT_ON_SPHERE	8
#define HEAP_TRACKER_LIB_LOG_PREFIX		"Heap-Tracker lib: "
#define HeapTracker_Log(...)			if (DEBUG_LOGS_ON) Log_Debug(HEAP_TRACKER_LIB_LOG_PREFIX __VA_ARGS__)
#define LogHeapStatus()					if (DEBUG_LOGS_ON) log_heap_status()

void log_heap_status(void)
{
	if (heap_allocated < 0)
	{
		Log_Debug("WARNING: heap_allocated (%zd) is NEGATIVE --> 'heap_allocated' will not be reliable from now on!\n", heap_allocated);
	}
	else if (heap_allocated > heap_threshold)
	{
		Log_Debug("WARNING: heap_allocated (%zd bytes) is above heap_threshold (%zu bytes)\n", heap_allocated, heap_threshold);
	}
	else
	{
		Log_Debug("SUCCESS: heap_allocated (%zd bytes) - delta with heap_threshold(%zd bytes)\n", heap_allocated, (ssize_t)heap_threshold - heap_allocated);
	}
}

//////////////////////////////////////////////////////////////////////////////////
// NATIVE malloc/free WRAPPERS
//////////////////////////////////////////////////////////////////////////////////
/*
*	NOTE: the library does not intentionally alter the behaviors of the native functions,
*	in order to not disrupt the functionality on other system libraries that use them.
*	Altering the implementations is NOT reccomended as it could result in unpredicted App behavior!
*/

// C-Library malloc/free stubs, which will be inter-positioned by the wrappers at link-time
void *__real_malloc(size_t size);
void *__real_realloc(void *ptr, size_t new_size);
void *__real_calloc(size_t num, size_t size);
void *__real_aligned_alloc(size_t alignment, size_t size);
void __real_free(void *ptr);


// Heap-tracking malloc() wrapper
void *__wrap_malloc(size_t size)
{	
	size_t actual_size = size + HEAP_MAX_ALIGNMENT_ON_SPHERE;
	if (actual_size <= HEAP_MAX_ALIGNMENT_ON_SPHERE) {
		return NULL;
	}

	void* ptr = __real_malloc(actual_size);
	if (ptr == NULL) {
		return NULL;
	}

	HeapTracker_Log("malloc(%zu)=%p... ", actual_size, ptr);

	heap_allocated += actual_size;
	*(size_t *)ptr = size;

	LogHeapStatus();

	return ptr + HEAP_MAX_ALIGNMENT_ON_SPHERE;
}

void __wrap_free(void* ptr)
{
	if (ptr == NULL) {
		return;
	}

	void *actual_buffer_pos = ptr - HEAP_MAX_ALIGNMENT_ON_SPHERE;

	size_t actual_size = *(size_t *)actual_buffer_pos + HEAP_MAX_ALIGNMENT_ON_SPHERE;

	HeapTracker_Log("free(%p,%zu)... ", actual_buffer_pos, actual_size);

	__real_free(actual_buffer_pos);

	heap_allocated -= (ssize_t)actual_size;

	LogHeapStatus();
}

// Custom heap-tracking calloc() wrapper
void *__wrap_calloc(size_t num, size_t size)
{
	void *ptr = __real_calloc(num, size);

	HeapTracker_Log("calloc(%zu,%zu)=%p...", num, size, ptr);
	if (ptr)
	{
		heap_allocated += (ssize_t)(num * size);
	}
	
	LogHeapStatus();

	return ptr;
}

// Custom heap-tracking aligned_alloc() wrapper
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

// Custom heap-tracking realloc() wrapper
// NOTE: does NOT track heap as it is unaware of the previous memory block size!
void *__wrap_realloc(void *ptr, size_t new_size)
{
	HeapTracker_Log("WARNING! Native realloc(%p,%zu) was called instead of _realloc() helper: 'heap_allocated' will not be reliable from now on!\n", ptr, new_size);
	return __real_realloc(ptr, new_size);
}

//////////////////////////////////////////////////////////////////////////////////
// MUST-USE HELPERS
//////////////////////////////////////////////////////////////////////////////////

// Custom heap-tracking free() helper
//void _free(void *ptr, size_t size)
//{
//	HeapTracker_Log("_free(%p,%zu)... ", ptr, size);
//
//	__real_free(ptr);
//	if (ptr)
//	{
//		heap_allocated -= (ssize_t)size;
//	}
//
//	LogHeapStatus();
//}

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