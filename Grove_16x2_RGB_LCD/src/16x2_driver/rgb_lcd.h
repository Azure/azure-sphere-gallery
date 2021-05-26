/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <inttypes.h>
#include <stdbool.h>

int  RGBLCD_Init(int ISU);
void RGBLCD_SetColor(uint8_t r, uint8_t g, uint8_t b);
void RGBLCD_SetText(char* data);
