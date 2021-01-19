#pragma once
/*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/

/*
* 
* Purpose of this library *
* This library allows to override the native C malloc() and free(), in order to allow implementing custom heap tracking
* through a global 'heap_allocated' variable, which can be used to prevent OOM conditions.
*
* Context *
* Under the default memory management strategy, the Linux's kernel follows an "optimistic memory allocation strategy",
* that is, the kernel will not "actually" allocate the requested memory until you actually try to use it. 
* Under this strategy, mallocs will *always* succeed and return a non-NULL pointer and as per Linux's overcommitment policy,
* under extremely low memory conditions the kernel will start firing off the "OOM Killer" routine.
* Processes are given "OOM scores" and the process with the highest score will get killed.
* Therefore, by design in optimistic strategy, an App will crash with "Child terminated with signal = 0x9 (SIGKILL)" 
* when the heap could not "effectively" allocate the requested memory, upon its first access.
*  
* Usage *
* 1) Compile the library by adding the following two options to target_link_libraries() in CMakeLists.txt: -Wl,--wrap malloc -Wl,--wrap,free
*    i.e.: target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c -Wl,--wrap=malloc -Wl,--wrap=free)
* 2) Determine, by trial & error, what is the maximum heap usage for your App, and set the 'heap_limit' constant accordingly.
* 3) Use malloc() as usual in your App, BUT use the _free() and _realloc() helpers, in order to keep track of the available heap.
*    If the native free() and realloc() functions will be used in the App, then the internal heap counter will become invalid, and the
*    'heap_allocated' counter will become unreliable
*/

#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////////
#define DEBUG_LOGS_ON	1				// Enables(1)/Disables(0) verbose loggging
extern const size_t		heap_limit;		// Sets the maximum allowed heap that can be allocated (in bytes).
extern volatile ssize_t	heap_allocated;	// Currently allocated heap (in bytes). Note: this is NOT thread safe!

//////////////////////////////////////////////////////////////////////////////////
// Heap-tracking free and realloc function
//////////////////////////////////////////////////////////////////////////////////

/// <summary>
///		A custom heap-tracking free() wrapper.
///		Behaves live standard free(), but also tracks the heap consumption
///		within the custom 'heap_allocated' variable.
/// </summary>
/// <param name="ptr">A pointer to the heap memory to be freed.</param>
/// <param name="size">The size (in bytes) of the memory that was allocated to the <paramref name="ptr"/> pointer.</param>
void _free(void *ptr, size_t size);

/// <summary>
///		A custom heap-tracking realloc() wrapper.
///		Behaves live standard realloc(), but also tracks the heap consumption
///		within the custom 'heap_allocated' variable.
/// </summary>
/// <param name="ptr">A pointer to the heap memory to be freed.</param>
/// <param name="old_size">The size (in bytes) of the memory that is currently allocated to the <paramref name="ptr"/> pointer.</param>
/// <param name="new_size">The new size (in bytes) of the memory to be allocated.</param>
/// <returns>
///		On success, it returns a new pointer to the newly allocated memory.
///		On failure, it returns NULL.
/// </returns>
void *_realloc(void *ptr, size_t old_size, size_t new_size);
