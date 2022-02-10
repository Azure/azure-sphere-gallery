/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stddef.h>

// enable assert for all build
#define ASSERT(x)             if (!(x)) massert_fail(#x, __LINE__, __FILE__)

/**
 * assert and print which file, line expression cause the assert
 */
void massert_fail(const char *expr, int line, const char *file);

/**
 * our memory wrappers
 * On DEBUG version it will track memory free/allocation using an
 * array, this will help to detect memory leak and leak detail. But will
 * have some overhead.
 * On non-debug version it will check the malloc result and assert fail
 * so we don't need to check memory alloc result everywhere from within
 * the code, after all, there is no better way to recover when malloc failed
 * other than restart app.
 */
#ifdef DEBUG

#define MALLOC(size)           mmalloc(size, __LINE__, __FILE__)
#define CALLOC(nmemb, size)    mcalloc(nmemb, size, __LINE__, __FILE__)
#define REALLOC(ptr, size)     mrealloc(ptr, size, __LINE__, __FILE__)
#define STRDUP(s)              mstrdup(s, __LINE__, __FILE__)
#define STRNDUP(s, len)        mstrndup(s, len, __LINE__, __FILE__)
#define FREE(ptr)              do {mfree(ptr); ptr=NULL;}while(0)
#define MEMORY_REPORT(d)        memory_report(d)
/**
 * wrapper for malloc
 */
void *mmalloc(size_t size, int line, const char *file);

/**
 * wrapper for alloc
 */
void *mcalloc(size_t nmemb, size_t size, int line, const char *file);

/**
 * wrapper for realloc
 */
void *mrealloc(void *ptr, size_t size, int line, const char *file);

/**
 * wrapper for strdup
 */
char *mstrdup(const char *s, int line, const char *file);

/**
 * wrapper for strndup
 */
char *mstrndup(const char *s, size_t len, int line, const char *file);

/**
 * wrapper for free
 */
void mfree(void *ptr);

/**
 * report memory usage
 * @param show_detail whether should unreleased memory detail
 */
void memory_report(int show_detail);

#else

#define MALLOC   mmalloc
#define CALLOC   mcalloc
#define REALLOC  mrealloc
#define STRDUP   mstrdup
#define STRNDUP  mstrndup
#define FREE(ptr)     do {mfree(ptr);ptr=NULL;} while(0)
#define MEMORY_REPORT(d) (void)0

void *mmalloc(size_t size);
void *mcalloc(size_t nmemb, size_t size);
void *mrealloc(void *ptr, size_t size);
char *mstrdup(const char *s);
char *mstrndup(const char *s, size_t len);
void mfree(void *ptr);

#endif
