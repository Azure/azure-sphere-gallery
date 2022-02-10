/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stddef.h> /* size_t */
#include <stdint.h>
#include <time.h>

#include <frozen/frozen.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


typedef struct {
    char *data;
    size_t size;
} memory_block_t;


/**
 * convert data to hex string for logging purpose
 * @param data bytes to translate
 * @param len number of bytes to translate
 * @return hex string in the format of "12 ED 3B EF ..", it use a internal static buffer, so no need to free
 */
const char *hex(const unsigned char *data, size_t len);

/**
 * simple hash function to calcuate 32bits hash from an array of bytes, mainly been used to
 * check if data been changed without need to save original copy of data
 * @param buf byte array been used to calc has
 * @param len number of bytes
 * @return 32 bits hash value
 */
uint32_t hash(const unsigned char *buf, int32_t len);

/**
 * compare two double value using EPSLION
 * @param a value 1
 * @param b value 2
 * @return true if equal
 */
bool is_double_equal(double a, double b);

/**
 * compare two string, allow s1 s2 to be NULL
 * @param s1 first string
 * @param s2 second string
 * @return return true if s1 and s2 all NULL or content are the same. false otherwise
 */
bool strequal(const char* s1, const char* s2);

/**
 * Get the application version
 * @return the application version
 */
const char* app_version(void);

/**
 * Set the application version
 * @param version the version string
 */
void set_app_version(const char* version);