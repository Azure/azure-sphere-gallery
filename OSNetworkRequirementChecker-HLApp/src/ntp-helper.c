/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "ntp-helper.h"

#define NTP_TIME_OUT 30
#define TIME_BUFFER_SIZE 26
#define NTP_SERVER_LEN 256

// List of time server to be tested
const char *NTPServerList[] = { "168.61.215.74", "129.6.15.28", "20.43.94.199", "20.189.79.72",
                                "40.81.94.65", "40.81.188.85", "40.119.6.228", "40.119.148.38",
                                "20.101.57.9", "51.137.137.111", "51.145.123.29",
                                "52.148.114.188", "52.231.114.183"};
const unsigned int NTPServerListLen = 12;
char **timeBeforeSyncList = NULL;
char **timeAfterSyncList = NULL;
int ntpIndex = 0;
int ntpRetryCounter = 0;

const char *secondaryNtpServer = NULL;
Networking_NtpOption fallbackServerNtpOption = Networking_NtpOption_FallbackServerDisabled;

bool networkReady = false;
EventLoopTimer *ntpSyncStatusTimer = NULL;
EventLoop *ntpEventLoop = NULL;

ExitCode InitializeNTP(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    ntpEventLoop = EventLoop_Create();
    if (ntpEventLoop == NULL) {
        Log_Debug("ERROR: Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    static const struct timespec ntpSyncStatusTimerInterval = {.tv_sec = 1, .tv_nsec = 0};
    ntpSyncStatusTimer = CreateEventLoopPeriodicTimer(ntpEventLoop, &NtpSyncStatusTimerEventHandler,
                                                      &ntpSyncStatusTimerInterval);

    if (ntpSyncStatusTimer == NULL) {
        return ExitCode_Init_CreateNtpSyncStatusTimer;
    }
    return ExitCode_Success;
}

void ConfigureCustomNtpServer(const char *primaryNtpServer, const char *secondaryNtpServer)
{
    Log_Debug("EVENT: Configuring Custom NTP server\nINFO: Primary Server: %s\n", primaryNtpServer);
    if (secondaryNtpServer != NULL) {
        Log_Debug("INFO: Secondary Server: %s\n", secondaryNtpServer);
    }
    Log_Debug("INFO: Fallback Server NTP Option: %d\n", fallbackServerNtpOption);
    if (Networking_TimeSync_EnableCustomNtp(primaryNtpServer, secondaryNtpServer,
                                            fallbackServerNtpOption) == -1) {
        Log_Debug("ERROR: Configure Custom NTP failed: %d (%s)\n", errno, strerror(errno));
        exitCode = ExitCode_TimeSync_CustomNtp_Failed;
    }
}

void NtpSyncStatusTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_SyncStatusTimer_Consume;
        return;
    }
    bool currentNetworkingReady = false;
    if ((Networking_IsNetworkingReady(&currentNetworkingReady) == -1)) {
        Log_Debug("INFO: Error in retrieving ready state from IsNetworkingReady.\n");
    }
    if (currentNetworkingReady != networkReady) {
        // Toggle the states.
        ntpRetryCounter = 0;
        networkReady = currentNetworkingReady;

        // Retrieve and record time sync information
        GetLastNtpSyncInformation();
        if ((++ntpIndex) >= NTPServerListLen) {
            exitCode = ExitCode_Test_Finish;
        } else {
            networkReady = false;
            ConfigureCustomNtpServer(NTPServerList[ntpIndex], secondaryNtpServer);
            exitCode = InitializeNTP();
        }
    }
    if ((++ntpRetryCounter) >= NTP_TIME_OUT) {
        timeBeforeSyncList[ntpIndex++] = NULL;
        timeAfterSyncList[ntpIndex] = NULL;
        if ((ntpIndex) >= NTPServerListLen) {
            exitCode = ExitCode_Test_Finish;
        } else {
            ConfigureCustomNtpServer(NTPServerList[ntpIndex], secondaryNtpServer);
        }
        ntpRetryCounter = 0;
    }
}

void GetLastNtpSyncInformation(void)
{
    if (!networkReady) {
        Log_Debug("EVENT: Device has not yet successfully time synced.\n");
        return;
    }
    size_t ntpServerLength = NTP_SERVER_LEN;
    char ntpServer[ntpServerLength];
    struct tm timeBeforeSync;
    struct tm adjustedNtpTime;

    if (Networking_TimeSync_GetLastNtpSyncInfo(ntpServer, &ntpServerLength, &timeBeforeSync,
                                               &adjustedNtpTime) == -1) {
        if (errno == ENOENT) {
            Log_Debug("INFO: The device has not yet successfully completed a time sync.\n");
            return;
        }
        if (errno == ENOBUFS) {
            Log_Debug("ERROR: Buffer is too small to hold the NTP server. Size required is %zu\n",
                      ntpServerLength);
        }
        Log_Debug("ERROR: Get last NTP sync info failed: %d (%s)\n", errno, strerror(errno));
        exitCode = ExitCode_TimeSync_GetLastSyncInfo_Failed;
        return;
    }
    Log_Debug("EVENT: Successfully time synced to server %s\n", ntpServer);
    char *displayTimeBuffer = (char *)malloc(TIME_BUFFER_SIZE);

    if (strftime(displayTimeBuffer, TIME_BUFFER_SIZE, "%c", &timeBeforeSync) != 0) {
        timeBeforeSyncList[ntpIndex] = displayTimeBuffer;
    } else {
        free(displayTimeBuffer);
    }

    displayTimeBuffer = (char *)malloc(TIME_BUFFER_SIZE);
    if (strftime(displayTimeBuffer, TIME_BUFFER_SIZE, "%c", &adjustedNtpTime) != 0) {
        timeAfterSyncList[ntpIndex] = displayTimeBuffer;
    } else {
        free(displayTimeBuffer);
    }
}

void CustomNTPCleanUp(void)
{
    for (int i = 0; i < NTPServerListLen; ++i) {
        if (timeBeforeSyncList[i]) {
            free(timeBeforeSyncList[i]);
            free(timeAfterSyncList[i]);
        }
    }

    free(timeBeforeSyncList);
    free(timeAfterSyncList);

    DisposeEventLoopTimer(ntpSyncStatusTimer);
    EventLoop_Close(ntpEventLoop);
}

bool RunNTPDiagnostic(void)
{
    exitCode = ExitCode_Success;
    timeBeforeSyncList = (char **)malloc(NTPServerListLen * sizeof(char *));
    timeAfterSyncList = (char **)malloc(NTPServerListLen * sizeof(char *));
    Log_Debug("INFO: Custom NTP Test starting in 3 seconds.\n");
    sleep(3);

    exitCode = InitializeNTP();
    ConfigureCustomNtpServer(NTPServerList[ntpIndex], secondaryNtpServer);

    // Custom NTP loop
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(ntpEventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
            Log_Debug("Error: eventloop failed with error code: %d %d %s\n", result, errno,
                      strerror(errno));
        }
    }
    for (int i = 0; i < NTPServerListLen; ++i) {
        if (!timeBeforeSyncList[i]) {
            return false;
        }
    }
    return true;
}

void PrintNTPSummary(void)
{
    // Print out Custom NTP time sync result
    Log_Debug("\n\nCustom NTP Time Server Sync list:\n");
    for (int i = 0; i < NTPServerListLen; ++i) {
        if (!timeBeforeSyncList[i]) {
            Log_Debug("\tIndex: %d,\tName: %s,\tERROR: Failed to sync from timer server\n", i,
                      NTPServerList[i]);
            continue;
        }
        char *displayTimeBufferBefore = timeBeforeSyncList[i];
        char *displayTimeBufferAfter = timeAfterSyncList[i];
        Log_Debug("\tIndex: %d,\tName: %s,\tUTC time before sync: %s,\tafter sync: %s\n\n", i,
                  NTPServerList[i], displayTimeBufferBefore, displayTimeBufferAfter);
    }
}