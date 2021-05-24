/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <inttypes.h>
#include <stdbool.h>

// Device I2C Addresses
#define LCD_ADDRESS     0x3e
#define RGB_ADDRESS     0x62

bool InitRGBLCD(int ISU);
void SetRGBLCDColor(uint8_t r, uint8_t g, uint8_t b);
void SetRGBLCDText(char* data);
