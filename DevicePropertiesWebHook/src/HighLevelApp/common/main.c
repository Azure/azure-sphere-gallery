/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

/* Note that this is a modified version of the Azure IoT sample from the Azure Sphere samples github repo */
/* Please see the accompanying README.md for instructions for integrating these code files back into the sample */

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

#include <hw/sample_appliance.h>
#include <applibs/gpio.h>
int redLedFd = -1, greenLedFd = -1;

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
static EventLoopTimer *rebootTimer = NULL;

static bool isConnected = false;

// Business logic
static void SetThermometerTelemetryUploadEnabled(bool uploadEnabled);
static bool telemetryUploadEnabled = false; // False by default - do not send telemetry until told
                                            // by the user or the cloud

static const char *serialNumber = "TEMPMON-01234";

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

static const char* CloudResultToString(Cloud_Result result) {
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

static void ButtonPressedCallbackHandler(UserInterface_Button button)
{
    if (button == UserInterface_Button_A) {
        bool newTelemetryUploadEnabled = !telemetryUploadEnabled;
        Log_Debug("INFO: Telemetry upload enabled state changed (via button press): %s\n",
                  newTelemetryUploadEnabled ? "enabled" : "disabled");
        SetThermometerTelemetryUploadEnabled(newTelemetryUploadEnabled);
    } else if (button == UserInterface_Button_B) {
        Log_Debug("INFO: Device moved.\n");
        Cloud_Result result = Cloud_SendThermometerMovedEvent();
        if (result != Cloud_Result_OK) {
            Log_Debug("WARNING: Could not sent thermometer moved event to cloud: %s\n",
                CloudResultToString(result));
        }
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

    if (isConnected) {
        if (telemetryUploadEnabled) {
            // Generate a simulated temperature.
            float delta = ((float)(rand() % 41)) / 20.0f - 1.0f; // between -1.0 and +1.0
            telemetry.temperature += delta;

            Cloud_Result result = Cloud_SendTelemetry(&telemetry);
            if (result != Cloud_Result_OK) {
                Log_Debug("WARNING: Could not send thermometer telemetry to cloud: %s\n",
                          CloudResultToString(result));
            }
        } else {
            Log_Debug("INFO: Telemetry upload disabled; not sending telemetry.\n");
        }
    }
}

#include <applibs/powermanagement.h>
static void RebootTimerCallbackHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_TelemetryTimer_Consume;
        return;
    }

    PowerManagement_ForceSystemReboot();
}

#include <applibs/sysevent.h>
static EventRegistration *updateEventReg = NULL;
static void UpdateCallback(SysEvent_Events event, SysEvent_Status status, const SysEvent_Info *info,
                           void *context);

 #define   ExitCode_Init_RegisterEvent 121
#define ExitCode_UpdateCallback_GetUpdateEvent 122
#define ExitCode_UpdateCallback_UnexpectedEvent 123

static ExitCode SetupUpdateCheck(void)
{
    updateEventReg = SysEvent_RegisterForEventNotifications(eventLoop, SysEvent_Events_Mask,
                                                            UpdateCallback, NULL);
    if (updateEventReg == NULL) {
        Log_Debug("ERROR: could not register update event: %s (%d).\n", strerror(errno), errno);
        // this isnt the right way to exit - should return this
        return ExitCode_Init_RegisterEvent;
    }
    return 0;
}

/// <summary>
///     This function matches the SysEvent_EventsCallback signature, and is invoked
///     from the event loop when the system wants to perform an application or system update.
///     See <see cref="SysEvent_EventsCallback" /> for information about arguments.
/// </summary>
static void UpdateCallback(SysEvent_Events event, SysEvent_Status status, const SysEvent_Info *info,
                           void *context)
{
    switch (event) {
    case SysEvent_Events_NoUpdateAvailable: {
        Log_Debug("INFO: Update check finished. No updates available\n");
        //Cloud_SendEvent("NoUpdateAvailable");
        Cloud_SignalNoUpdatePending();
        GPIO_SetValue(redLedFd, GPIO_Value_Low);
        break;
    }

    // Downloading updates has started. Change the blink interval to indicate this event has
    // occured, and keep waiting.
    case SysEvent_Events_UpdateStarted: {
        Log_Debug("INFO: Updates have started downloading\n");
//        Cloud_SendEvent("UpdateDownloading");
        break;
    }

    // Updates are ready for install
    case SysEvent_Events_UpdateReadyForInstall: {
        Log_Debug("INFO: Update download finished and is ready for install.\n");

        SysEvent_Info_UpdateData data;
        int result = SysEvent_Info_GetUpdateData(info, &data);
        if (result == -1) {
            Log_Debug("ERROR: SysEvent_Info_GetUpdateData failed: %s (%d).\n", strerror(errno),
                      errno);
            exitCode = ExitCode_UpdateCallback_GetUpdateEvent;
            break;
        }

        GPIO_SetValue(greenLedFd, GPIO_Value_Low);
        Cloud_SignalUpdateInstalling();

        if (data.update_type == SysEvent_UpdateType_App) {
//            Cloud_SendEvent("AppUpdateInstalling");
        } else if (data.update_type == SysEvent_UpdateType_System) {
//            Cloud_SendEvent("OSUpdateInstalling");
        } else {
//            Cloud_SendEvent("UnknownUpdateInstalling");
        }

        break;
    }

    default:
        Log_Debug("ERROR: Unexpected event\n");
        exitCode = ExitCode_UpdateCallback_UnexpectedEvent;
        break;
    }

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

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    ExitCode updatecheckExitCode = SetupUpdateCheck();
     if (updatecheckExitCode != ExitCode_Success) {
        return updatecheckExitCode;
    }

    struct timespec telemetryPeriod = {.tv_sec = 60, .tv_nsec = 0};
    telemetryTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &TelemetryTimerCallbackHandler, &telemetryPeriod);
    if (telemetryTimer == NULL) {
        return ExitCode_Init_TelemetryTimer;
    }

    
    struct timespec rebootPeriod = {.tv_sec = 300, .tv_nsec = 0};
    rebootTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &RebootTimerCallbackHandler, &rebootPeriod);
    if (rebootTimer == NULL) {
        return ExitCode_Init_TelemetryTimer; // not changed
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

                            
    // Open LEDs for accept mode status.
    redLedFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_RED, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (redLedFd < 0) {
        Log_Debug("ERROR: Could not open start red LED: %s (%d).\n", strerror(errno), errno);
        return 159;
    }

    greenLedFd =
        GPIO_OpenAsOutput(SAMPLE_RGBLED_GREEN, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (greenLedFd < 0) {
        Log_Debug("ERROR: Could not open check for updates green LED: %s (%d).\n", strerror(errno),
                  errno);
        return 160;
    }
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

    DisposeEventLoopTimer(rebootTimer);
    DisposeEventLoopTimer(telemetryTimer);
    EventLoop_Close(eventLoop);

    close(redLedFd);
    close(greenLedFd);
    Log_Debug("Closing file descriptors\n");
}
