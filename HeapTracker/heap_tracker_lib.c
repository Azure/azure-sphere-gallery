/*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

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

int heap_track_pointer(void *ptr, size_t size);
int heap_untrack_pointer(void *ptr);

//////////////////////////////////////////////////////////////////////////////////
// THREAD SAFETY
//////////////////////////////////////////////////////////////////////////////////
#if ENABLE_THREAD_SAFETY
static pthread_mutex_t mux;
#endif

void heap_track_init(void)
{
#if ENABLE_THREAD_SAFETY
	pthread_mutex_init(&mux, NULL);
#endif
}

#if ENABLE_THREAD_SAFETY
#	define MUTEX_LOCK	pthread_mutex_lock(&mux);
#	define MUTEX_UNLOCK	pthread_mutex_unlock(&mux);
#else
#	define MUTEX_LOCK
#	define MUTEX_UNLOCK
#endif

//////////////////////////////////////////////////////////////////////////////////
// LOGGING
//////////////////////////////////////////////////////////////////////////////////
#define HEAP_TRACKER_LIB_LOG_PREFIX		"Heap-Tracker: "
#define HeapTracker_Log(...)			Log_Debug(HEAP_TRACKER_LIB_LOG_PREFIX __VA_ARGS__)
#define LogHeapStatus()					log_heap_status() 

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
#if ENABLE_DEBUG_VERBOSE_LOGS
	else
	{
		Log_Debug("SUCCESS: heap_allocated (%zd bytes) - delta with heap_threshold(%zd bytes)\n", heap_allocated, (ssize_t)heap_threshold - heap_allocated);
	}
#endif
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
	MUTEX_LOCK;

	void *ptr = __real_malloc(size);

	HeapTracker_Log("malloc(%zu)=%p... ", size, ptr);
	if (NULL != ptr)
	{
		heap_allocated += (ssize_t)size;

#if ENABLE_POINTER_TRACKING
		heap_track_pointer(ptr, size);
#endif // ENABLE_POINTER_TRACKING

	}

	LogHeapStatus();

	MUTEX_UNLOCK;

	return ptr;
}

// Custom heap-tracking calloc() wrapper
void *__wrap_calloc(size_t num, size_t size)
{
	MUTEX_LOCK;

	void *ptr = __real_calloc(num, size);

	HeapTracker_Log("calloc(%zu,%zu)=%p...", num, size, ptr);
	if (ptr)
	{
		heap_allocated += (ssize_t)(num * size);

#if ENABLE_POINTER_TRACKING
		heap_track_pointer(ptr, size);
#endif // ENABLE_POINTER_TRACKING
	}

	LogHeapStatus();

	MUTEX_UNLOCK;

	return ptr;
}

// Custom heap-tracking aligned_alloc() wrapper
void *__wrap_aligned_alloc(size_t alignment, size_t size)
{
	MUTEX_LOCK;

	void *ptr = __real_aligned_alloc(alignment, size);

	HeapTracker_Log("aligned_alloc(%zu,%zu)=%p...", alignment, size, ptr);
	if (ptr)
	{
		heap_allocated += (ssize_t)size;

#if ENABLE_POINTER_TRACKING
		heap_track_pointer(ptr, size);
#endif // ENABLE_POINTER_TRACKING
	}

	LogHeapStatus();

	MUTEX_UNLOCK;

	return ptr;
}

// Custom heap-tracking realloc() wrapper
void *__wrap_realloc(void *ptr, size_t new_size)
{
	MUTEX_LOCK;

	void *new_ptr = __real_realloc(ptr, new_size);

	HeapTracker_Log("realloc(%p, %zu)=%p... ", ptr, new_size, new_ptr);
	if (NULL != new_ptr)
	{
		heap_allocated += (ssize_t)(new_size);

#if ENABLE_POINTER_TRACKING
		if (ptr && -1 == heap_untrack_pointer(ptr))
		{
			HeapTracker_Log("WARNING: free(%p) was called for a non-tracked pointer.\n", ptr);
		}

		if (-1 == heap_track_pointer(new_ptr, new_size))
		{
			HeapTracker_Log("WARNING: free(%p) was called for a non-tracked pointer.\n", ptr);
		}
#else
		HeapTracker_Log("WARNING! Native realloc(%p,%zu) was called instead of _realloc() helper: 'heap_allocated' will not be reliable from now on!\n", ptr, new_size);
#endif
	}

	LogHeapStatus();

	MUTEX_UNLOCK;

	return new_ptr;
}

// Native free() wrapper (does NOT track heap!)
void __wrap_free(void *ptr)
{
	MUTEX_LOCK;

	HeapTracker_Log("free(%p)... ", ptr);

#if ENABLE_POINTER_TRACKING
	if (ptr && -1 == heap_untrack_pointer(ptr))
	{
		HeapTracker_Log("WARNING: free(%p) was called for a non-tracked pointer.\n", ptr);
	}
#else
	HeapTracker_Log("WARNING! Native free(%p) was called instead of _free() helper: 'heap_allocated' will not be reliable from now on!\n", ptr);
#endif // ENABLE_POINTER_TRACKING

	LogHeapStatus();

	__real_free(ptr);

	MUTEX_UNLOCK;
}

#if !ENABLE_POINTER_TRACKING
// Custom heap-tracking free() helper
void _free(void *ptr, size_t size)
{
	MUTEX_LOCK;

	HeapTracker_Log("_free(%p,%zu)... ", ptr, size);

	__real_free(ptr);
	if (ptr)
	{
		heap_allocated -= (ssize_t)size;
	}

	LogHeapStatus();

	MUTEX_UNLOCK;
}

// Custom heap-tracking realloc() helper
void *_realloc(void *ptr, size_t old_size, size_t new_size)
{
	MUTEX_LOCK;

	void *new_ptr = __real_realloc(ptr, new_size);

	HeapTracker_Log("_realloc(%p,%zu,%zu)=%p... ", ptr, old_size, new_size, new_ptr);
	if (NULL != new_ptr)
	{
		heap_allocated += (ssize_t)(new_size - old_size + 1);
	}

	LogHeapStatus();

	MUTEX_UNLOCK;

	return new_ptr;
}
#endif // ENABLE_POINTER_TRACKING


//////////////////////////////////////////////////////////////////////////////////
// POINTER TRACKING
//////////////////////////////////////////////////////////////////////////////////
#if ENABLE_POINTER_TRACKING

typedef struct
{
	void *address;
	size_t size;
} t_pointer;

static t_pointer **allocated_pointers = NULL;
static size_t allocated_pointers_size = 0;
static unsigned int allocated_pointers_count = 0;

int heap_track_pointer(void *ptr, size_t size)
{
	if (NULL == allocated_pointers || allocated_pointers_size < (allocated_pointers_count * sizeof(void *)))
	{
		size_t new_size = (allocated_pointers_count + POINTER_TRACK_INC) * sizeof(void *);
		void *new_ptr = __real_realloc(allocated_pointers, new_size);
		if (NULL == new_ptr)
		{
			HeapTracker_Log("heap_track_pointer(%p,%zu) FAILED - out of memory!!", ptr, size);
			return -1;
		}
		else
		{
			allocated_pointers = new_ptr;
			allocated_pointers_size = new_size;
		}
	}

	allocated_pointers[allocated_pointers_count]->address = ptr;
	allocated_pointers[allocated_pointers_count]->size = size;
	allocated_pointers_count++;

	return 0;
}

int heap_untrack_pointer(void *ptr)
{
	int pos = 0;
	t_pointer **cur = allocated_pointers;
	while (cur && pos < allocated_pointers_count)
	{
		if ((*cur)->address == ptr)
		{
			heap_allocated -= (ssize_t)(allocated_pointers[pos]->size);

			memcpy(allocated_pointers[pos], allocated_pointers[pos + 1], allocated_pointers_size - sizeof(t_pointer));
			memset(allocated_pointers[allocated_pointers_count - 1], 0, sizeof(t_pointer));
			allocated_pointers_count--;

			return 0;
		}

		cur++, pos++;
	}

	return -1;
}

#endif // ENABLE_POINTER_TRACKING