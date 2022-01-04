/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

// See CMakeLists.txt to control whether USE_SOCKET_LOG is defined or not.
// If this is defined, then the Log_Debug call is sent via a UDP socket broadcast as well as printed on stderr (which goes to the PC when the app is started under debugger, like the standard Log_Debug).
// If this is not defined, then the standard Log_Debug is used.

// Note: This code uses a socket for output, this could be replaced with UART or other interface

#ifdef USE_SOCKET_LOG
    #include "udplog.h"
#else
    #include <applibs/log.h>
#endif

int main(void)
{
    int counter = 0;

    Log_Debug("Starting SphereUdpLogSender application...\n");

    const struct timespec sleepTime = {.tv_sec = 1, .tv_nsec = 0};
    while (true) {
        if (counter == 5)
        {
            Log_Debug("Info: Some information - counter %d\n", counter);
        }

        if (counter == 10)
        {
            Log_Debug("Error: some error information - counter %d\n", counter);
        }

        if (counter == 15)
        {
            counter = 0;
            Log_Debug("Resetting counter to zero\n");
        }
        counter++;

        nanosleep(&sleepTime, NULL);
    }
}
