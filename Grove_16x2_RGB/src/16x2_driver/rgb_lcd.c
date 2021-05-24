/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <string.h>
#include <inttypes.h>

#include "rgb_lcd.h"

#include "time.h"
#include "errno.h"

#include <applibs/i2c.h>
#include <applibs/log.h>
#include "../utils.h"

static int _i2cFd = -1;

static void textCommand(uint8_t cmd);
static void write_byte_data(uint8_t address, uint8_t b0, uint8_t b1);

static int WriteI2CData(uint8_t address, void* data, size_t len)
{
    int ret = -1;

    if (_i2cFd < 0)
        return ret;

    ret = I2CMaster_Write(_i2cFd, address, data, len);
    if (ret != len) {
        Log_Debug("%s: I2CMaster_Write: errno=%d (%s)\n", __func__, errno, strerror(errno));
        ret = -1;
    }

    return ret;
}

static void write_byte_data(uint8_t address, uint8_t b0, uint8_t b1)
{
    uint8_t data[2] = { b0, b1 };
    WriteI2CData(address, data, 2);
}

static void textCommand(uint8_t cmd)
{
    uint8_t data[2] = { 0x80, cmd };
    WriteI2CData(LCD_ADDRESS, data, 2);
}

bool InitRGBLCD(int ISU)
{
    _i2cFd = I2CMaster_Open(ISU);
    if (_i2cFd > -1)
    {
        I2CMaster_SetBusSpeed(_i2cFd, I2C_BUS_SPEED_FAST);
        I2CMaster_SetTimeout(_i2cFd, 100);
        CheckI2CDevices(2, _i2cFd);
    }
    else
    {
        Log_Debug("Failed to open I2C ISU\n");
        return false;
    }

    return true;
}


void SetRGBLCDColor(uint8_t r, uint8_t g, uint8_t b)
{
    write_byte_data(RGB_ADDRESS, 0, 0);
    write_byte_data(RGB_ADDRESS, 1, 0);
    write_byte_data(RGB_ADDRESS, 0x08, 0xaa);
    write_byte_data(RGB_ADDRESS, 4, r);
    write_byte_data(RGB_ADDRESS, 3, g);
    write_byte_data(RGB_ADDRESS, 2, b);
}

void SetRGBLCDText(char* data)
{
    textCommand(0x01);
    delay(50);
    textCommand(0x08 | 0x04); // display on, no cursor
    textCommand(0x28); // 2 lines
    delay(50);

    int count = 0;
    int row = 0;

    size_t len = strlen(data);
    uint8_t chr = 0x00;

    for (size_t x = 0; x < 32; x++)
    {
        if (x >= len)
            chr = 0x20;
        else
            chr = data[x];

        if (chr == '\n' || count == 16)
        {
            count = 0;
            row += 1;
            if (row == 2)
                break;

            textCommand(0xc0);
        }
        else
        {
            write_byte_data(LCD_ADDRESS, 0x40, chr);
        }
        count += 1;
    }
}
