#ifndef HEAP_TRACKER_LIB_H
#define HEAP_TRACKER_LIB_H

#include <stdio.h>

int 	heap_tracker_init(void);
ssize_t heap_tracker_get_allocated(void);
ssize_t heap_tracker_get_peak_allocated(void);
size_t	heap_tracker_get_leakage(void);

#endif