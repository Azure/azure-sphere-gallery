/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef SD_H_
#define SD_H_

#include <stdbool.h>
#include <stdint.h>

#include "lib/GPT.h"
#include "lib/Platform.h"
#include "lib/SPIMaster.h"

typedef struct SDCard SDCard;

SDCard  *SD_Open(SPIMaster *interface);
void     SD_Close(SDCard *card);

uint32_t SD_GetBlockLen(const SDCard *card);
bool     SD_SetBlockLen(SDCard *card, uint32_t len);

bool     SD_ReadBlock (const SDCard *card, uint32_t addr, void *data);
bool     SD_WriteBlock(SDCard *card, uint32_t addr, const void *data);

#endif // #ifndef SD_H_
