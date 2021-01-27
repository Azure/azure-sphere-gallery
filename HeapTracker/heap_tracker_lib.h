/*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
#pragma once
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////////
#define DEBUG_LOGS_ON	1				// Enables(1)/Disables(0) verbose loggging.
extern const size_t		heap_threshold;	// Sets a reference allocation threshold (in bytes) after which the library will log warnings.
extern volatile ssize_t	heap_allocated;	// Currently allocated heap (in bytes).

//////////////////////////////////////////////////////////////////////////////////
// Heap-tracking free and realloc function
//////////////////////////////////////////////////////////////////////////////////

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
