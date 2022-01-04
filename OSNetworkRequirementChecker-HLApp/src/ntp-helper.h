/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include "common.h"

/// <summary>
///     Set up event handlers.
/// </summary>
/// <returns>ExitCode_Success if all resources were allocated successfully; otherwise another
/// ExitCode value which indicates the specific failure.</returns>
ExitCode InitializeNTP(void);

/// <summary>
///     Configures custom NTP server. Sets the exit code if it encounters any error.
/// </summary>
void ConfigureCustomNtpServer(const char *primaryNtpServer, const char *secondaryNtpServer);

/// <summary>
///     NTP sync status timer: Checks the NTP sync status of the device.
/// </summary>
/// <param name="timer">Timer which has fired.</param>
void NtpSyncStatusTimerEventHandler(EventLoopTimer *timer);

/// <summary>
///     Retrieves the last NTP sync information.
/// </summary>
void GetLastNtpSyncInformation(void);

/// <summary>
///     Clean up the resources previously allocated for custom NTP test.
/// </summary>
void CustomNTPCleanUp(void);

/// <summary>
///     Run custom NTP diagnostic test.
/// </summary>
bool RunNTPDiagnostic(void);

/// <summary>
///     Print custom NTP diagnostic summary.
/// </summary>
void PrintNTPSummary(void);