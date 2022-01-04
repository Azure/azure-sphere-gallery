/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <applibs/log.h>
#include <applibs/networking.h>
#include "eventloop_timer_utilities.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_ConnectionTimer_Consume = 2,
    ExitCode_ConnectionTimer_ConnectionReady = 3,
    ExitCode_ConnectionTimer_Disarm = 4,

    ExitCode_Init_EventLoop = 5,
    ExitCode_Init_Socket = 6,
    ExitCode_Init_ConnectionTimer = 7,

    ExitCode_Main_EventLoopFail = 8,

    ExitCode_Test_Finish = 9,

    ExitCode_TimeSync_CustomNtp_Failed = 10,
    ExitCode_TimeSync_GetLastSyncInfo_Failed = 11,
    ExitCode_SyncStatusTimer_Consume = 12,
    ExitCode_Init_CreateNtpSyncStatusTimer = 13
} ExitCode;

// Termination state
volatile sig_atomic_t exitCode;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
void TerminationHandler(int signalNumber);