/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

int VS1053_Init(void);
void VS1053_SetVolume(uint16_t volume);
void VS1053_PlayByte(uint8_t data);
void VS1053_Cleanup(void);

