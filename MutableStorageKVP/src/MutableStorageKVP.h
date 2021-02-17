/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "applibs/log.h"

bool WriteProfileString(char *keyName, char *value);
ssize_t GetProfileString(char *keyName, char *returnedString, size_t Size);
bool DeleteProfileString(char *keyName);
