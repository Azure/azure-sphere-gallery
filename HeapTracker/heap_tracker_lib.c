#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "applibs_versions.h"
#include <applibs/log.h>

#include "heap_tracker_lib.h"

#define HEAP_TRACKER_DEBUG
#define HEAP_MAX_ALIGNMENT_ON_SPHERE	16

static pthread_mutex_t _lock;
static volatile ssize_t _heap_allocated = 0;
static volatile ssize_t _heap_peak_allocated = 0;
static volatile size_t _leakage = 0;

extern void* __real_malloc(size_t size);
extern void* __real_calloc(size_t num, size_t size);
extern void __real_free(void* ptr);

int heap_tracker_init(void)
{
	int ret = 0;

	if (pthread_mutex_init(&_lock, NULL) < 0) {
#ifdef HEAP_TRACKER_DEBUG
		Log_Debug("ERROR: Could not init mutex: %s (%d)\n", strerror(errno), errno);
#endif
		ret = -1;
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

size_t heap_tracker_get_leakage(void)
{
	return _leakage;
}

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

#ifdef HEAP_TRACKER_DEBUG
	Log_Debug("INFO: malloc %zu bytes @ %p\n", actual_size, ptr);
#endif

	(void)pthread_mutex_lock(&_lock);
	_leakage++;
	_heap_allocated += actual_size;
	if (_heap_allocated > _heap_peak_allocated) {
		_heap_peak_allocated = _heap_allocated;
	}
	(void)pthread_mutex_unlock(&_lock);

	*(size_t *)ptr = size;

	return ptr + HEAP_MAX_ALIGNMENT_ON_SPHERE;
}

void __wrap_free(void* ptr)
{
	if (ptr == NULL) {
		return;
	}

	void *actual_buffer_pos = ptr - HEAP_MAX_ALIGNMENT_ON_SPHERE;

	size_t actual_size = *(size_t *)actual_buffer_pos + HEAP_MAX_ALIGNMENT_ON_SPHERE;

#ifdef HEAP_TRACKER_DEBUG
	Log_Debug("INFO: free %zu bytes @ %p\n", actual_size, actual_buffer_pos);
#endif

	__real_free(actual_buffer_pos);

	(void)pthread_mutex_lock(&_lock);
	_leakage--;
	_heap_allocated -= (ssize_t)actual_size;
	(void)pthread_mutex_unlock(&_lock);
}

void *__wrap_calloc(size_t num, size_t size)
{
	size_t total = num * size;

	size_t actual_size = total + HEAP_MAX_ALIGNMENT_ON_SPHERE;
	if (actual_size <= HEAP_MAX_ALIGNMENT_ON_SPHERE) {
		return NULL;
	}

	void* ptr = __real_malloc(actual_size);
	if (ptr == NULL) {
		return NULL;
	}

#ifdef HEAP_TRACKER_DEBUG
	Log_Debug("INFO: calloc() %zu bytes @ %p\n", actual_size, ptr);
#endif

	(void)pthread_mutex_lock(&_lock);
	_leakage++;
	_heap_allocated += actual_size;
	if (_heap_allocated > _heap_peak_allocated) {
		_heap_peak_allocated = _heap_allocated;
	}
	(void)pthread_mutex_unlock(&_lock);

	*(size_t*)ptr = size;

	return memset(ptr + HEAP_MAX_ALIGNMENT_ON_SPHERE, 0, total);
}



