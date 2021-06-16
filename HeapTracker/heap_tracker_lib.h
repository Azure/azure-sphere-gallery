#ifndef HEAP_TRACKER_LIB_H
#define HEAP_TRACKER_LIB_H

#include <stdio.h>

#define CFG_HEAP_TRACKER_COMPATIBLE_API
#define CFG_HEAP_TRACKER_DEBUG
//#define CFG_HEAP_TRACKER_TRHEADSAFE

#ifdef CFG_HEAP_TRACKER_COMPATIBLE_API
#define CFG_HEAP_TRACKER_MAX_ALIGNMENT		16
#else
#define CFG_HEAP_TRACKER_MAX_ALIGNMENT		0
void	heap_tracker_free(void* ptr, size_t size);
void*	heap_tracker_realloc(void* ptr, size_t old_size, size_t new_size);
#endif

int 	heap_tracker_init(void);
ssize_t heap_tracker_get_allocated(void);
ssize_t heap_tracker_get_peak_allocated(void);
size_t	heap_tracker_get_number_of_mismatch(void);

#endif