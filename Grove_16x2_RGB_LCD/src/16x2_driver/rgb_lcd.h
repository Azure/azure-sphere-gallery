/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <inttypes.h>
#include <stdbool.h>

int InitRGBLCD(int ISU);
void SetRGBLCDColor(uint8_t r, uint8_t g, uint8_t b);
void SetRGBLCDText(char* data);
