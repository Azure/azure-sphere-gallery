#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "applibs_versions.h"
#include <applibs/log.h>

#include "heap_tracker_lib.h"

/*

	When CFG_HEAP_TRACKER_COMPATIBLE_API is ON, the memory allocated looks like:

	|<-  CFG_HEAP_TRACKER_MAX_ALIGNMENT ->|
	|									  |
	+------------------------------------------------------------------------------------+
	| Magic Number | size |	   Unused     |				 n bytes    					 |
	+------------------------------------------------------------------------------------+
*/

#define HEAP_TRACKER_MAGIC_NUBMER		0x55AA1024

#define HEAP_TRACKER_MN_OFFSET			0
#define HEAP_TRACKER_SIZE_OFFSET		4

#if defined(CFG_HEAP_TRACKER_DEBUG)
	#define HEAP_TRACKER_DEBUG(...)		Log_Debug("INFO: "__VA_ARGS__)
#else
	#define HEAP_TRACKER_DEBUG(...)
#endif

#if defined(CFG_HEAP_TRACKER_ERROR)
#define HEAP_TRACKER_ERROR(...)			Log_Debug("ERROR: "__VA_ARGS__)
#else
#define HEAP_TRACKER_ERROR(...)
#endif

#if defined(CFG_HEAP_TRACKER_TRHEADSAFE)
	#define PTHREAD_MUTEX_INIT(p1, p2)	pthread_mutex_init(p1, p2)
	#define PTHREAD_MUTEX_LOCK(p1)		pthread_mutex_lock(p1)
	#define PTHREAD_MUTEX_UNLOCK(p1)	pthread_mutex_unlock(p1)
#else
	#define PTHREAD_MUTEX_INIT(p1, p2)	0 
	#define PTHREAD_MUTEX_LOCK(p1)		
	#define PTHREAD_MUTEX_UNLOCK(p1)
#endif

#ifdef CFG_HEAP_TRACKER_COMPATIBLE_API
#define CFG_HEAP_TRACKER_MAX_ALIGNMENT		16
#else
#define CFG_HEAP_TRACKER_MAX_ALIGNMENT		0
#endif

static pthread_mutex_t _lock;
static volatile ssize_t _heap_allocated = 0;
static volatile ssize_t _heap_peak_allocated = 0;
static volatile size_t _mismatch_call = 0;

extern void* __real_malloc(size_t size);
extern void* __real_calloc(size_t num, size_t size);
extern void __real_free(void* ptr);
extern void* __real_aligned_alloc(size_t alignment, size_t size);
extern void* __real_realloc(size_t num, size_t size);

int heap_tracker_init(void)
{
	int ret;

	ret = PTHREAD_MUTEX_INIT(&_lock, NULL);
	if (ret < 0) {
		HEAP_TRACKER_DEBUG("could not init mutex: %s (%d)\n", strerror(errno), errno);
	}

	return ret;
}

ssize_t heap_tracker_get_allocated(void)
{
	return _heap_allocated;
}

ssize_t heap_tracker_get_peak_allocated(void)
{
	return _heap_peak_allocated;
}

size_t heap_tracker_get_number_of_mismatch(void)
{
	return _mismatch_call;
}

void *__wrap_malloc(size_t size)
{	
	size_t actual_size = size + CFG_HEAP_TRACKER_MAX_ALIGNMENT;
	if (actual_size <= CFG_HEAP_TRACKER_MAX_ALIGNMENT) {
		return NULL;
	}

	void* ptr = __real_malloc(actual_size);
	if (ptr == NULL) {
		return NULL;
	}

	HEAP_TRACKER_DEBUG("malloc %zu bytes @ %p\n", actual_size, ptr);

	PTHREAD_MUTEX_LOCK(&_lock);
	_mismatch_call++;
	_heap_allocated += actual_size;
	if (_heap_allocated > _heap_peak_allocated) {
		_heap_peak_allocated = _heap_allocated;
	}
	PTHREAD_MUTEX_UNLOCK(&_lock);

#if defined(CFG_HEAP_TRACKER_COMPATIBLE_API)
	*(size_t *)(ptr + HEAP_TRACKER_MN_OFFSET) = HEAP_TRACKER_MAGIC_NUBMER;
	*(size_t *)(ptr + HEAP_TRACKER_SIZE_OFFSET) = size;
#endif

	return ptr + CFG_HEAP_TRACKER_MAX_ALIGNMENT;
}

#if defined(CFG_HEAP_TRACKER_COMPATIBLE_API)
static void heap_tracker_free(void* ptr, size_t size)
{
#else
void heap_tracker_free(void* ptr, size_t size)
{
	if (ptr == NULL) {
		return;
	}
#endif

	HEAP_TRACKER_DEBUG("free %zu bytes @ %p\n", size, ptr);

	__real_free(ptr);

	PTHREAD_MUTEX_LOCK(&_lock);
	_mismatch_call--;
	_heap_allocated -= (ssize_t)size;
	PTHREAD_MUTEX_UNLOCK(&_lock);
}

void __wrap_free(void* ptr)
{
#if defined(CFG_HEAP_TRACKER_COMPATIBLE_API)
	if (ptr == NULL) {
		return;
	}

	void *actual_buffer_pos = ptr - CFG_HEAP_TRACKER_MAX_ALIGNMENT;
	if (*(size_t *)(actual_buffer_pos + HEAP_TRACKER_MN_OFFSET) != HEAP_TRACKER_MAGIC_NUBMER) {
		HEAP_TRACKER_ERROR("free() on a incorrect address @ %p!\n", actual_buffer_pos);
		return;
	}

	size_t actual_size = *(size_t *)(actual_buffer_pos + HEAP_TRACKER_SIZE_OFFSET) + CFG_HEAP_TRACKER_MAX_ALIGNMENT;

	heap_tracker_free(actual_buffer_pos, actual_size);
#else
	HEAP_TRACKER_ERROR("free() @ %p instead of heap_tracker_free() helper, heap_allocated will not be reliable from now on!\n", ptr);
	__real_free(ptr);
#endif
}

void *__wrap_calloc(size_t num, size_t size)
{
	size_t total = num * size;

	size_t actual_size = total + CFG_HEAP_TRACKER_MAX_ALIGNMENT;
	if (actual_size <= CFG_HEAP_TRACKER_MAX_ALIGNMENT) {
		return NULL;
	}

	void* ptr = __real_malloc(actual_size);
	if (ptr == NULL) {
		return NULL;
	}

	HEAP_TRACKER_DEBUG("calloc() %zu bytes @ %p\n", actual_size, ptr);

	PTHREAD_MUTEX_LOCK(&_lock);
	_mismatch_call++;
	_heap_allocated += actual_size;
	if (_heap_allocated > _heap_peak_allocated) {
		_heap_peak_allocated = _heap_allocated;
	}
	PTHREAD_MUTEX_UNLOCK(&_lock);

#if defined(CFG_HEAP_TRACKER_COMPATIBLE_API)
	*(size_t *)(ptr + HEAP_TRACKER_MN_OFFSET) = HEAP_TRACKER_MAGIC_NUBMER;
	*(size_t *)(ptr + HEAP_TRACKER_SIZE_OFFSET) = size;
#endif

	return memset(ptr + CFG_HEAP_TRACKER_MAX_ALIGNMENT, 0, total);
}

void* __wrap_aligned_alloc(size_t alignment, size_t size)
{
#if defined(CFG_HEAP_TRACKER_COMPATIBLE_API)
	HEAP_TRACKER_ERROR("aligned_alloc() is not allowed, return NULL!\n");
	return NULL;
#else
	void* ptr = __real_aligned_alloc(alignment, size);
	if (NULL != ptr) {
		HEAP_TRACKER_DEBUG("INFO: aligned_alloc() %zu bytes @ %p with %zu bytes alignment\n", size, ptr, alignment);

		PTHREAD_MUTEX_LOCK(&_lock);
		_mismatch_call++;
		_heap_allocated += size;
		if (_heap_allocated > _heap_peak_allocated) {
			_heap_peak_allocated = _heap_allocated;
		}
		PTHREAD_MUTEX_UNLOCK(&_lock);
	}

	return ptr;
#endif
}

#if defined(CFG_HEAP_TRACKER_COMPATIBLE_API)
static
#endif
void* heap_tracker_realloc(void* ptr, size_t old_size, size_t new_size)
{
	void* new_ptr = __real_realloc(ptr, new_size);

	HEAP_TRACKER_DEBUG("realloc() %zu bytes @ %p, from previous %zu bytes\n", new_size, new_ptr, old_size);

	ssize_t diff = (ptr == NULL) ? new_size : (ssize_t)(new_size - old_size);
	if (diff != 0) {
		PTHREAD_MUTEX_LOCK(&_lock);
		_heap_allocated += diff;
		if (_heap_allocated > _heap_peak_allocated) {
			_heap_peak_allocated = _heap_allocated;
		}
		if (NULL == ptr) {					// equal to malloc()
			_mismatch_call++;
		} else {
			if (new_size == 0) {			// euqal to free()
				_mismatch_call--;
			}
		}
		PTHREAD_MUTEX_UNLOCK(&_lock);
	}

	return new_ptr;
}


void* __wrap_realloc(void* ptr, size_t new_size)
{
#if defined(CFG_HEAP_TRACKER_COMPATIBLE_API)

	void* actual_buffer_pos;
	size_t actual_old_size;
	size_t actual_new_size;

	if (NULL == ptr) {				// equal to malloc
		actual_buffer_pos = NULL;
		actual_old_size = 0;
	} else {
		actual_buffer_pos = ptr - CFG_HEAP_TRACKER_MAX_ALIGNMENT;
		if (*(size_t*)(actual_buffer_pos + HEAP_TRACKER_MN_OFFSET) != HEAP_TRACKER_MAGIC_NUBMER) {
			HEAP_TRACKER_ERROR("realloc() on a incorrect address @ %p!\n", actual_buffer_pos);
			return NULL;
		}

		actual_old_size = *(size_t *)(actual_buffer_pos + HEAP_TRACKER_SIZE_OFFSET) + CFG_HEAP_TRACKER_MAX_ALIGNMENT;
	}

	if (new_size == 0) {
		actual_new_size = 0;
	} else {
		actual_new_size = new_size + CFG_HEAP_TRACKER_MAX_ALIGNMENT;
	}

	void * new_ptr = heap_tracker_realloc(actual_buffer_pos, actual_old_size, actual_new_size);
	if (NULL == new_ptr) {
		return NULL;
	}

	*(size_t *)(new_ptr + HEAP_TRACKER_MN_OFFSET) = HEAP_TRACKER_MAGIC_NUBMER;
	*(size_t *)(new_ptr + HEAP_TRACKER_SIZE_OFFSET) = new_size;

	return new_ptr + CFG_HEAP_TRACKER_MAX_ALIGNMENT;
#else
	HEAP_TRACKER_ERROR("realloc() @ % p instead of heap_tracker_realloc() helper, heap_allocated will not be reliable from now on!\n", ptr);
	return __real_realloc(ptr, new_size);
#endif
}
