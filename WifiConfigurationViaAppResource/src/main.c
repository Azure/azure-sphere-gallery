/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <applibs/log.h>

#include "AppResourceWiFiConfig.h"

int main(void)
{
    if (!setWiFiConfigurationFromAppResource())
    {
        Log_Debug("Failed to configure WiFi from Embedded Application Resource\n");
    }
    else
    {
        Log_Debug("Successfully configured WiFi from Embedded Application Resource\n");
    }

    while (true);   // spin...

    return 0;
}