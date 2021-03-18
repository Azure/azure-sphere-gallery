/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stdint.h>

void initCurl(void);
void cleanupCurl(void);
void* malloc_callback(size_t size);
void free_callback(void* ptr);
void* realloc_callback(void* ptr, size_t size);
char* strdup_callback(const char* str);
void* calloc_callback(size_t nmemb, size_t size);
