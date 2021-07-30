/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <unistd.h>
#include <stdint.h>

#pragma once
int SDCard_Init(void);
void SDCard_Cleanup(void);

int SDCard_WriteBlock(uint32_t block, uint8_t* data, uint32_t size);
int SDCard_ReadBlock(uint32_t block, uint8_t* data, uint32_t size);
