/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <applibs/log.h>
#include "curlFunctions.h"

void initCurl(void)
{
#ifdef ENABLE_CURL_MEMORY_TRACE
    curl_global_init_mem(CURL_GLOBAL_ALL, malloc_callback, free_callback, realloc_callback, strdup_callback, calloc_callback);
#else
    curl_global_init(CURL_GLOBAL_ALL);
#endif
}

void cleanupCurl(void)
{
    curl_global_cleanup();
}

// Curl memory tracking functions.
void* malloc_callback(size_t size)
{
    void* retPtr = malloc(size);
    Log_Debug(">>> %s - size %u - pointer 0x%08lx\n", __func__, size, retPtr);
    return retPtr;
}

void free_callback(void* ptr)
{
    Log_Debug(">>> %s - pointer 0x%08lx\n", __func__, ptr);
    free(ptr);
}

void* realloc_callback(void* ptr, size_t size)
{
    void* retPtr = realloc(ptr, size);

    Log_Debug(">>> %s, pointer 0x%08lx, size %u, new pointer 0x%08lx\n", __func__, ptr, size, retPtr);

    return retPtr;
}

char* strdup_callback(const char* str)
{

    char* retPtr = strdup(str);

    Log_Debug(">>> %s - string '%s', new pointer 0x%08lx\n", __func__, str, retPtr);

    return retPtr;
}

void* calloc_callback(size_t nmemb, size_t size)
{
    void* retPtr = calloc(nmemb, size);

    Log_Debug(">>> %s - nmenb %u, size %u, pointer 0x%08lx\n", __func__, nmemb, size, retPtr);

    return retPtr;
}
