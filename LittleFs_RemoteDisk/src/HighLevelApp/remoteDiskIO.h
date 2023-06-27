/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdint.h>
#include "constants.h"

typedef struct {
   uint8_t block[STORAGE_BLOCK_SIZE];
   uint8_t metadata[STORAGE_METADATA_SIZE];
} StorageBlock;

_Static_assert(sizeof(StorageBlock) == STORAGE_BLOCK_SIZE + STORAGE_METADATA_SIZE);

int readBlockData(uint32_t blockNum, StorageBlock* block);
int writeBlockData(uint32_t blockNum, const StorageBlock* sectorData);
