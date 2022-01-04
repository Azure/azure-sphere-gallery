/*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
#pragma once
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES & DEFINES
//////////////////////////////////////////////////////////////////////////////////
#define ENABLE_DEBUG_VERBOSE_LOGS				1	// Enables(1)/Disables(0) verbose logging.
#define ENABLE_THREAD_SAFETY					1	// Enables(1)/Disables(0) thread safety.
#define ENABLE_POINTER_TRACKING                 1	// Enables(1)/Disables(0) pointer tracking.
#define POINTER_TRACK_INC						50	// Defines the growth size (in # of elements) for the internal pointer tracking array, 
													// once the number of allocated pointers overflows the current array size.
extern const size_t		heap_threshold;				// Sets a reference allocation threshold (in bytes) after which the library will log warnings.
extern volatile ssize_t	heap_allocated;				// Currently allocated heap (in bytes).


//////////////////////////////////////////////////////////////////////////////////
// Heap-tracker initialization function
//////////////////////////////////////////////////////////////////////////////////

/// <summary>
///		Heap-Tracker initialization function. Must be called before any memory function is used.
/// </summary>
/// <param name="">none</param>
void heap_track_init(void);

#if !ENABLE_POINTER_TRACKING
////////////////////////////////////////////////////////////////////////////////////
// Heap-tracking free and realloc functions (when pointer tracking is disabled)
////////////////////////////////////////////////////////////////////////////////////

/// <summary>
///		A custom heap-tracking free() wrapper.
///		Behaves like standard free(), while also tracking the heap consumption
///		within the global 'heap_allocated' variable.
/// </summary>
/// <param name="ptr">A pointer to the heap memory to be freed.</param>
/// <param name="size">The size (in bytes) of the memory that was allocated to the <paramref name="ptr"/> pointer.</param>
void _free(void *ptr, size_t size);

/// <summary>
///		A custom heap-tracking realloc() wrapper.
///		Behaves live standard realloc(), while also tracking the heap consumption
///		within the global 'heap_allocated' variable.
/// </summary>
/// <param name="ptr">A pointer to the heap memory to be freed.</param>
/// <param name="old_size">The size (in bytes) of the memory that is currently allocated to the <paramref name="ptr"/> pointer.</param>
/// <param name="new_size">The new size (in bytes) of the memory to be allocated.</param>
/// <returns>
///		On success, it returns a new pointer to the newly allocated memory.
///		On failure, it returns NULL.
/// </returns>
void *_realloc(void *ptr, size_t old_size, size_t new_size);
#endif // ENABLE_POINTER_TRACKING