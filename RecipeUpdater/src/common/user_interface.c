/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <string.h>

#include <applibs/gpio.h>
#include <applibs/log.h>
#include <hw/mt3620.h>

#include "user_interface.h"
#include "eventloop_timer_utilities.h"

static void CloseFdAndPrintError(int fd, const char *fdName);

// File descriptors - initialized to invalid value
static int statusLedsGpioFd[4] = {
    -1, -1, -1, -1
};

static int statusLedGpioPins[4] = {
    MT3620_GPIO4,
    MT3620_GPIO8,
    MT3620_GPIO9,
    MT3620_GPIO10
};

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="fdName">File descriptor name to use in error message</param>
static void CloseFdAndPrintError(int fd, const char *fdName)
{
    if (fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}

ExitCode UserInterface_Initialize(void)
{
    Log_Debug("INFO: Opening LEDs as output.\n");

    for(int i = 0; i < 4; i++) {
        statusLedsGpioFd[i] = GPIO_OpenAsOutput(statusLedGpioPins[i], GPIO_OutputMode_PushPull, GPIO_Value_High);
        if (statusLedsGpioFd[i] == -1) {
            Log_Debug("ERROR: Could not open LED[%d]: %s (%d).\n", i, strerror(errno), errno);
            return ExitCode_Init_Led;
        }
    }

    return ExitCode_Success;
}

void UserInterface_Cleanup(void)
{
    // Leave the LEDs off
    for(int i = 0; i < 4; i++) {
        if (statusLedsGpioFd[i] >= 0) {
            GPIO_SetValue(statusLedsGpioFd[i], GPIO_Value_High);
            CloseFdAndPrintError(statusLedsGpioFd[i], "StatusLed");
        }
    }   
}

void UserInterface_SetStatus(int index, bool status)
{
    if (index >= 0 && index < 4) {
        GPIO_SetValue(statusLedsGpioFd[index], status ? GPIO_Value_Low : GPIO_Value_High);
    } else {
        Log_Debug("ERROR: invalid LED index: %d\n", index);
    }
}