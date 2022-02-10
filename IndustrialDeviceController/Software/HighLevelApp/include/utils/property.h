/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

/**
 * read a property from file
 * @param key property key to read
 * @return string value if key exist, memory been allocated to hold value, caller need to release memory.
 * NULL if file or key not exist
 */
char *read_property(const char *key);


/**
 * write a property to file, if property file not exist, it will be created first, if
 * key not exist, it will be added, if key exist, it value will be replaced with provided
 * value
 * @param key property key to write
 * @param value property value to write
 * @return 0 on succeed or -1 if failed
 */
int write_property(const char *key, char *value);