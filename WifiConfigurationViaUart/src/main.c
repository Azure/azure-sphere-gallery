/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

#include <applibs/log.h>
#include "eventloop_timer_utilities.h"
#include "SerialWiFiConfig.h"

EventLoop* eventLoop = NULL;

bool init(void)
{
    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        return false;
    }

    if (!SerialWiFiConfig_Init(eventLoop))
        return false;

    return true;
}

int main(void)
{
    // Initialize Uart and EventLoop - exit on failure.
    if (!init())
        return -1;

    while (true) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            return -2;
        }
    }
}