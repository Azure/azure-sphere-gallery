/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

/*
 * Link-time interposition of malloc and free using the static
 * linker's (ld) "--wrap symbol" flag.
 *
 * Compile the executable using "-Wl,--wrap,malloc -Wl,--wrap,free".
 * This tells the linker to resolve references to malloc as
 * __wrap_malloc, free as __wrap_free, __real_malloc as malloc, and
 * __real_free as free.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <utils/memory.h>

void massert_fail(const char *expr, int line, const char *file)
{
    fprintf(stderr, "assertion \"%s\" failed at %s:%d\n", expr, file, line);
    abort();
}

#ifdef DEBUG

#define NUM_BLOCKS 1000

struct memblock_t {
    void *addr;
    size_t size;
#ifdef LEAK_DETAIL
    int line;
    const char *file;
#endif
};

static struct memblock_t blocks[NUM_BLOCKS];
static size_t g_allocated;
static size_t g_allocated_blocks;
static size_t g_allocated_max;

void *mmalloc(size_t size, int line, const char* file)
{
    if (size == 0) {
        return NULL;
    }

    void *ptr = malloc(size);
    ASSERT(ptr);

    g_allocated += size;
    g_allocated_blocks++;
    if (g_allocated > g_allocated_max) {
        g_allocated_max = g_allocated;
    }

    int i;
    for (i=0; i<NUM_BLOCKS; i++) {
        if (blocks[i].addr == NULL){
            blocks[i].addr = ptr;
            blocks[i].size = size;
#ifdef LEAK_DETAIL
            blocks[i].line = line;
            blocks[i].file = file;
#endif
            break;
        }
    }
    ASSERT(i < NUM_BLOCKS);
    return ptr;
}



void mfree(void *ptr)
{
    if (!ptr) {
        return;
    }

    int i;
    for (i=0; i<NUM_BLOCKS; i++) {
        if (blocks[i].addr == ptr) {
            blocks[i].addr = NULL;
            g_allocated -= blocks[i].size;
            g_allocated_blocks--;
            blocks[i].size = 0;
            break;
        }
    }

    // we may free memory from library, loose the check
    ASSERT(i<NUM_BLOCKS);
    free(ptr);
}


void *mcalloc(size_t nmemb, size_t size, int line, const char *file)
{
    size_t nbytes = nmemb * size;
    void *ptr = mmalloc(nbytes, line, file);
    memset(ptr, 0, nbytes);
    return ptr;
}


void *mrealloc(void *ptr, size_t size, int line, const char *file)
{
    if (!ptr) {
        return mmalloc(size, line, file);
    }

    void *new_ptr = realloc(ptr, size);
    ASSERT(new_ptr);

    int i = 0;
    for (i=0; i<NUM_BLOCKS; i++) {
        if (blocks[i].addr == ptr) {
            g_allocated = g_allocated + size - blocks[i].size;
            blocks[i].addr = new_ptr;
            blocks[i].size = size;
#ifdef LEAK_DETAIL
            blocks[i].line = line;
            blocks[i].file = file;
#endif
            break;
        }
    }

    if (g_allocated > g_allocated_max) {
        g_allocated_max = g_allocated;
    }

    ASSERT(i<NUM_BLOCKS);
    return new_ptr;
}

char *mstrdup(const char *s, int line, const char *file)
{
    size_t i = 0;

    while (*(s + i)) {
        i++;
    }

    char *out = (char *)mmalloc(i + 1, line, file);
    ASSERT(out);
    char *p = out;

    while (i--) {
        *p++ = *s++;
    }

    *p = '\0';
    return out;
}

char *mstrndup(const char *s, size_t n, int line, const char *file)
{
    size_t i = 0;

    while (i < n && *(s + i)) {
        i++;
    }

    char *out = (char *)mmalloc(i + 1, line, file);
    ASSERT(out);
    char *p = out;

    while (i--) {
        *p++ = *s++;
    }

    *p = '\0';
    return out;
}

void memory_report(int show_detail)
{
    fprintf(stderr, "Memory [max/current/blocks] = %zu/%zu/%zu\n", g_allocated_max, g_allocated, g_allocated_blocks);
    if (show_detail) {
        for (int i = 0; i < NUM_BLOCKS; i++) {
            if (blocks[i].addr) {
#ifdef LEAK_DETAIL
                fprintf(stderr, "Leak %zu bytes, allocated at %s:%d\n", blocks[i].size, blocks[i].file, blocks[i].line);
#else
                fprintf(stderr, "Leak %zu bytes at %p\n", blocks[i].size, blocks[i].addr);
#endif
            }
        }
    }
}

#else

void *mmalloc(size_t size)
{
    void *ptr = malloc(size);
    ASSERT(ptr);
    return ptr;
}


void *mcalloc(size_t nmemb, size_t size)
{
    void *ptr = calloc(nmemb, size);
    ASSERT(ptr);
    return ptr;

}

void *mrealloc(void *ptr, size_t size)
{
    void *new_ptr = realloc(ptr, size);
    ASSERT(new_ptr);
    return new_ptr;
}

char *mstrdup(const char *s)
{
    char *ptr = strdup(s);
    ASSERT(ptr);
    return ptr;
}

char *mstrndup(const char *s, size_t len)
{
    char *ptr = strndup(s, len);
    ASSERT(ptr);
    return ptr;
}

void mfree(void *ptr)
{
    free(ptr);
}

#endif