/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application tests:
//  1. DNS resolver against known prod endpoints
//  (https://learn.microsoft.com/en-us/azure-sphere/network/ports-protocols-domains).
//  2. NTP time sync with known time servers
//
// It uses the APIs in the following Azure Sphere application libraries:
// - log (displays messages in the Device Output window during debugging)
// - networking (get network interface connection status)
// - eventloop (system invokes handlers for timer events)

#include "dns-helper.h"
#include "ntp-helper.h"

void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

int main(void)
{
    bool success = RunDNSDiagnostic();
    success &= RunNTPDiagnostic();

    PrintDNSSummary();
    PrintNTPSummary();

    DNSResolverCleanUp();
    CustomNTPCleanUp();

    if (success) {
        Log_Debug("PASS: Diagnostic App Finished Successfully.\n");
    } else {
        Log_Debug("ERROR: Diagnostic App Finished with Failure.\n");
    }

    Log_Debug("INFO: Application exiting in 20 seconds.\n");
    sleep(20);
    return exitCode;
}