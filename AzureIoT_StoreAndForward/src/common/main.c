/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application demonstrates how to use Azure Sphere devices with Azure IoT
// services, using the Azure IoT C SDK.
//
// It implements a simulated thermometer device, with the following features:
// - Telemetry upload (simulated temperature, device moved events) using Azure IoT Hub events.
// - Reporting device state (serial number) using device twin/read-only properties.
// - Mutable device state (telemetry upload enabled) using device twin/writeable properties.
// - Alert messages invoked from the cloud using device methods.
//
// It can be configured using the top-level CMakeLists.txt to connect either directly to an
// Azure IoT Hub, to an Azure IoT Edge device, or to use the Azure Device Provisioning service to
// connect to either an Azure IoT Hub, or an Azure IoT Central application. All connection types
// make use of the device certificate issued by the Azure Sphere security service to authenticate,
// and supply an Azure IoT PnP model ID on connection.
//
// It uses the following Azure Sphere libraries:
// - eventloop (system invokes handlers for timer events)
// - gpio (digital input for button, digital output for LED)
// - log (displays messages in the Device Output window during debugging)
// - networking (network interface connection status)
//
// You will need to provide information in the application manifest to use this application. Please
// see README.md and the other linked documentation for full details.

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/eventloop.h>
#include <applibs/networking.h>
#include <applibs/log.h>

#include "eventloop_timer_utilities.h"
#include "user_interface.h"
#include "exitcodes.h"
#include "cloud.h"
#include "options.h"
#include "connection.h"

static volatile sig_atomic_t exitCode = ExitCode_Success;

// Initialization/Cleanup
static ExitCode InitPeripheralsAndHandlers(void);
static void ClosePeripheralsAndHandlers(void);

// Interface callbacks
static void ExitCodeCallbackHandler(ExitCode ec);
static void ButtonPressedCallbackHandler(UserInterface_Button button);

// Cloud
static const char *CloudResultToString(Cloud_Result result);
static void ConnectionChangedCallbackHandler(bool connected);
static void CloudTelemetryUploadEnabledChangedCallbackHandler(bool status);
static void DisplayAlertCallbackHandler(const char *alertMessage);

// Timer / polling
static EventLoop *eventLoop = NULL;
static EventLoopTimer *telemetryTimer = NULL;

static bool isConnected = false;

// Business logic
static void DeviceMoved(void);
static void SetThermometerTelemetryUploadEnabled(bool uploadEnabled);
static bool telemetryUploadEnabled = false; // False by default - do not send telemetry until told
                                            // by the user or the cloud

static const char *serialNumber = "TEMPMON-01234";

// Simple File System
#include "sfs.h"
#include "remoteDiskIO.h"

#define BLOCK_SIZE     512
#define TOTAL_BLOCKS   8192

typedef struct
{
    float temperature;
    time_t timestamp;
} FileData;

FileData fileData;

static int ReadBlock(uint32_t block, uint8_t* buffer, size_t size);
static int WriteBlock(uint32_t block, uint8_t* buffer, size_t size);

static int ReadBlock(uint32_t block, uint8_t* buffer, size_t size)
{
    uint8_t* data = readBlockData(block * BLOCK_SIZE, size);

    if (data == NULL)
    {
        return -1;
    }

    memcpy(buffer, data, size);
    return 0;
}

static int WriteBlock(uint32_t block, uint8_t* buffer, size_t size)
{
    writeBlockData(buffer, size, block * BLOCK_SIZE);

    return 0;
}


/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Azure IoT Application starting.\n");

    bool isNetworkingReady = false;
    if ((Networking_IsNetworkingReady(&isNetworkingReady) == -1) || !isNetworkingReady) {
        Log_Debug("WARNING: Network is not ready. Device cannot connect until network is ready.\n");
    }

    exitCode = Options_ParseArgs(argc, argv);

    if (exitCode != ExitCode_Success) {
        return exitCode;
    }

    exitCode = InitPeripheralsAndHandlers();

    // Main loop
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    ClosePeripheralsAndHandlers();

    Log_Debug("Application exiting.\n");

    return exitCode;
}

static void ExitCodeCallbackHandler(ExitCode ec)
{
    exitCode = ec;
}

static const char *CloudResultToString(Cloud_Result result)
{
    switch (result) {
    case Cloud_Result_OK:
        return "OK";
    case Cloud_Result_NoNetwork:
        return "No network connection available";
    case Cloud_Result_OtherFailure:
        return "Other failure";
    }

    return "Unknown Cloud_Result";
}

static void SetThermometerTelemetryUploadEnabled(bool uploadEnabled)
{
    telemetryUploadEnabled = uploadEnabled;
    UserInterface_SetStatus(uploadEnabled);

    Cloud_Result result = Cloud_SendThermometerTelemetryUploadEnabledChangedEvent(uploadEnabled);
    if (result != Cloud_Result_OK) {
        Log_Debug(
            "WARNING: Could not send thermometer telemetry upload enabled changed event to cloud: "
            "%s",
            CloudResultToString(result));
    }
}

static void DeviceMoved(void)
{
    Log_Debug("INFO: Device moved.\n");

    time_t now;
    time(&now);

    Cloud_Result result = Cloud_SendThermometerMovedEvent(now);
    if (result != Cloud_Result_OK) {
        Log_Debug("WARNING: Could not send thermometer moved event to cloud: %s\n",
                  CloudResultToString(result));
    }
}

static void ButtonPressedCallbackHandler(UserInterface_Button button)
{
    if (button == UserInterface_Button_A) {
        bool newTelemetryUploadEnabled = !telemetryUploadEnabled;
        Log_Debug("INFO: Telemetry upload enabled state changed (via button press): %s\n",
                  newTelemetryUploadEnabled ? "enabled" : "disabled");
        SetThermometerTelemetryUploadEnabled(newTelemetryUploadEnabled);
    } else if (button == UserInterface_Button_B) {
        DeviceMoved();
    }
}

static void CloudTelemetryUploadEnabledChangedCallbackHandler(bool uploadEnabled)
{
    Log_Debug("INFO: Thermometer telemetry upload enabled state changed (via cloud): %s\n",
              uploadEnabled ? "enabled" : "disabled");
    SetThermometerTelemetryUploadEnabled(uploadEnabled);
}

static void DisplayAlertCallbackHandler(const char *alertMessage)
{
    Log_Debug("ALERT: %s\n", alertMessage);
}

static void ConnectionChangedCallbackHandler(bool connected)
{
    isConnected = connected;

    if (isConnected) {
        Cloud_Result result = Cloud_SendDeviceDetails(serialNumber);
        if (result != Cloud_Result_OK) {
            Log_Debug("WARNING: Could not send device details to cloud: %s\n",
                      CloudResultToString(result));
        }
    }
}

static void TelemetryTimerCallbackHandler(EventLoopTimer *timer)
{
    static Cloud_Telemetry telemetry = {.temperature = 50.f};

    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_TelemetryTimer_Consume;
        return;
    }

    time_t now;
    time(&now);
    struct tm* t = gmtime(&now);
    int numFiles = 0;

    if (isConnected) {
        if (t->tm_sec % 5 == 0)
        {
            // Generate a simulated temperature.
            float delta = ((float)(rand() % 41)) / 20.0f - 1.0f; // between -1.0 and +1.0
            telemetry.temperature += delta;

            numFiles = FS_GetNumberOfFilesInDirectory("data");

            // if we can upload, and there aren't any files in storage, then upload the data
            if (telemetryUploadEnabled && numFiles == 0)
            {
                Cloud_Result result = Cloud_SendTelemetry(&telemetry, now);
                if (result != Cloud_Result_OK) {
                    Log_Debug("WARNING: Could not send thermometer telemetry to cloud: %s\n",
                        CloudResultToString(result));
                }
            }
            else
            {
                // if upload isn't enabled, or there are still files in storage, then store the data (data order is preserved)

                fileData.temperature = telemetry.temperature;
                fileData.timestamp = now;

                // write temperature and time of iso8601 time format event data/time
                assert(FS_WriteFile("data", "temperature.txt", (uint8_t*)&fileData, sizeof(fileData)) == 0);
                numFiles = FS_GetNumberOfFilesInDirectory("data");
                Log_Debug("%d telemetry item%sstored\n", numFiles, numFiles == 1 ? " " : "s ");
            }
        }
        else
        {
            if (!telemetryUploadEnabled)
            {
                return;
            }
            // See if there's anything in storage that we need to upload.
            // Note that this is happening every second (skipping seconds % 5 == 0).
            numFiles = FS_GetNumberOfFilesInDirectory("data");
            if (numFiles == 0)
            {
                // nothing to upload, bail...
                return;
            }

            struct fileEntry fileInfo;
            FS_GetOldestFileInfo("data", &fileInfo);
            uint8_t* data = (uint8_t*)malloc(fileInfo.fileSize);
            memset(data, 0x00, fileInfo.fileSize);
            FS_ReadOldestFile("data", data, fileInfo.fileSize);

            FileData* fData = (FileData*)data;

            // Send the telemetry
            telemetry.temperature = fData->temperature;
            // get the date/time.
            t = gmtime(&(fData->timestamp));
            Log_Debug("(%d telemetry items in storage) upload: %04d/%02d/%02d - %02d:%02d:%02d - %3.2f\n",numFiles,t->tm_year+1900,t->tm_mon+1,t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, fData->temperature);
            Cloud_Result result = Cloud_SendTelemetry(&telemetry, fData->timestamp);

            // if upload was ok, delete the oldest file.
            if (result == Cloud_Result_OK) {
                FS_DeleteOldestFileInDirectory("data");
            }
            free(data);
        }
    }
}

/// <summary>
/// Initialize the Simple File System
/// </summary>
int InitializeFileSystem(void)
{
    if (FS_Init(WriteBlock, ReadBlock, TOTAL_BLOCKS) == -1)
        return -1;

    // try to mount the file system first, if this fails, format, then mount.
    if (FS_Mount() == -1)
    {
        if (FS_Format() == -1)
            return -1;

        if (FS_Mount() == -1)
            return -1;
    }

    // file system is initialized and mounted.

    // see if the 'data' directory already exists.
    struct dirEntry dir;
    if (FS_GetDirectoryByName("data", &dir) == -1)
    {
        // directory doesn't exist, add it.

        dir.maxFiles = 4000;    // 4000 files is 8000 blocks (one for the file header, one for the file data)
        dir.maxFileSize = 256;
        snprintf(dir.dirName, 8, "data");

        if (FS_AddDirectory(&dir) == -1)
            return -1;
    }

    return 0;
}


/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    if (InitializeFileSystem() != 0)
        return Exit_Code_Init_FileSystem;

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    struct timespec telemetryPeriod = {.tv_sec = 1, .tv_nsec = 0};
    telemetryTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &TelemetryTimerCallbackHandler, &telemetryPeriod);
    if (telemetryTimer == NULL) {
        return ExitCode_Init_TelemetryTimer;
    }

    ExitCode interfaceExitCode =
        UserInterface_Initialise(eventLoop, ButtonPressedCallbackHandler, ExitCodeCallbackHandler);

    if (interfaceExitCode != ExitCode_Success) {
        return interfaceExitCode;
    }

    UserInterface_SetStatus(telemetryUploadEnabled);

    void *connectionContext = Options_GetConnectionContext();

    return Cloud_Initialize(eventLoop, connectionContext, ExitCodeCallbackHandler,
                            CloudTelemetryUploadEnabledChangedCallbackHandler,
                            DisplayAlertCallbackHandler, ConnectionChangedCallbackHandler);
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    DisposeEventLoopTimer(telemetryTimer);
    Cloud_Cleanup();
    UserInterface_Cleanup();
    Connection_Cleanup();
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors\n");
}
