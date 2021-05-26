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
    if (RGBLCD_Init(MT3620_RDB_HEADER4_ISU2_I2C) == 0)
    {
        RGBLCD_SetText("Hello world\nSecond line!");
        RGBLCD_SetColor(0, 128, 64);

        delay(2000);
        for (int x = 0; x < 255; x++)
        {
            RGBLCD_SetColor(x, 255 - x, 0);
            delay(100);
        }

        RGBLCD_SetColor(0, 255, 0);
        RGBLCD_SetText("Fin!");
    }

    while (true);   // spin...

    return 0;
}