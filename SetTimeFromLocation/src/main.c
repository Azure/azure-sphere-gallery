/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/networking.h>

#include "location_from_ip.h"
#include "settime.h"

void delay(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

int main(void)
{
    Log_Debug("Starting application...\n");

    bool isNetworkReady = false;
    while (!isNetworkReady)
    {
        Networking_IsNetworkingReady(&isNetworkReady);
        if (!isNetworkReady)
        {
            Log_Debug("Network is not ready\n");
            delay(5000);
        }
    }

    // get country code and lat/long
    struct location_info *locInfo = GetLocationData();
    if (locInfo != NULL)
    {
        SetLocalTime(locInfo->lat, locInfo->lng);
    }

    // don't let the app exit.
    while (true);
}
