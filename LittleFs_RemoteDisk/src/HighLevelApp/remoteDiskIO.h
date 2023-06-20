/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdint.h>
uint8_t* readBlockData(uint32_t offset, uint32_t size);
int writeBlockData(const uint8_t* sectorData, uint32_t size, uint32_t offset);
