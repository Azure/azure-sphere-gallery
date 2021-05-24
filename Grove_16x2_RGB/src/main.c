/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/i2c.h>
#include <signal.h>

#include "hw/mt3620_rdb.h"
#include "16x2_driver/rgb_lcd.h"
#include "utils.h"

int main(void)
{
    if (InitRGBLCD(MT3620_RDB_HEADER4_ISU2_I2C))
    {
        SetRGBLCDText("Hello world\nSecond line!");
        SetRGBLCDColor(0, 128, 64);

        delay(2000);
        for (int x = 0; x < 255; x++)
        {
            SetRGBLCDColor(x, 255 - x, 0);
            delay(100);
        }

        SetRGBLCDColor(0, 255, 0);
        SetRGBLCDText("Fin!");
    }

    while (true);   // spin...

    return 0;
}