/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

#include <errno.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/sysevent.h>
#include <applibs/powermanagement.h>

#include <applibs/eventloop.h>
#include "eventloop_timer_utilities.h"

#include "curldefs.h"
#include "MutableStorageKVP.h"
#include "utils.h"
#include "intercore.h"
#include "parson.h"
#include "pthread.h"

#include "i2c_oled.h"
#include "SSD1306_Icons.h"

#include <applibs/adc.h>
#include <applibs/eventloop.h>

#include "intercore_messages.h"

#include <math.h>

// socket stuff for UDP RX.
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define OneMS 1000000

extern int sockFd;

// Azure IoT SDK
#include <iothub_client_core_common.h>
#include <iothub_device_client_ll.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <iothub.h>
#include <azure_sphere_provisioning.h>

#include "intercore_messages.h"

void InitUDPThread(void);
static void* UDPReadThread(void* arg);
void error(char* msg);
static pthread_t UDP_Thread = 0;

// contains current device information (pitch, yaw, roll, battery).
struct DEVICE_STATUS device_status;

// Azure IoT Hub/Central defines.
#define SCOPEID_LENGTH 20
static char scopeId[SCOPEID_LENGTH]; // ScopeId for the Azure IoT Central application, set in
                                     // app_manifest.json, CmdArgs
static bool isAppA = true;

static IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle = NULL;
static const int keepalivePeriodSeconds = 20;
static bool iothubAuthenticated = false;
static void SendMessageCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* context);
static void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload,
    size_t payloadSize, void* userContextCallback);
// static void TwinReportBoolState(const char* propertyName, bool propertyValue);
static void ReportStatusCallback(int result, void* context);
static const char* GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);
static const char* getAzureSphereProvisioningResultString(
    AZURE_SPHERE_PROV_RETURN_VALUE provisioningResult);
// static void SendTelemetry(const unsigned char* key, const unsigned char* value);
static void SendTelemetryInt(const unsigned char* key, const int value);
static void SendIoTMessageRaw(const char* message);
static void SetupAzureClient(void);
void SocketEventHandler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context);
void GetCompassDirection(float compassAngle, char* CompassString, size_t length);
static void TwinReportIntState(const char* propertyName, int propertyValue, int messageVersion);

static void ShowUpdatingIcon(void);
static void ShowWaitIcon(void);

static size_t lastDeviceTwinVersion = 0;

static char telemetryMessage[255];

enum Icon_Codes {
    None = 0,
    UpdateApp = 1,
    AppA = 2,
    AppB = 3,
    UpdateDeferred=4
};

int adcControllerFd=-1;
void SetupADC(void);
float sampleMaxVoltage = 2.5f;
int sampleBitCount = 0;

int BatteryLevel = -1;
bool HaveNetwork = false;
bool HaveIoTC = false;
int currentIcon = None;

void UpdateDisplay(uint8_t batteryLevel, bool haveNetwork, bool haveIoTC, int AppUpdateIcon);

static uint8_t fillBuffer[512];

// timer stuff
static EventLoop* eventLoop = NULL;
static EventLoopTimer* azureTimer = NULL;
static EventLoopTimer* batteryTimer = NULL;
static EventLoopTimer* intercoreTimer = NULL;
static EventLoopTimer* iconShowTimer = NULL;
static EventLoopTimer* imuStableTimer = NULL;

// Application update events are received via an event loop.
static EventRegistration* updateEventReg = NULL;

static void UpdateCallback(SysEvent_Events event, SysEvent_Status status, const SysEvent_Info* info, void* context);
static const char* EventStatusToString(SysEvent_Status status);
static const char* UpdateTypeToString(SysEvent_UpdateType updateType);
static bool updateDeferred = false;
static bool updateApplied = false;  // used to set icon on app exit.
static bool WaitForIMU = false;
static bool imuStable = false;  // used to determine when to turn off the timer icon.

// Azure IoT poll periods
static const int AzureIoTDefaultPollPeriodSeconds = 5;

static int azureIoTPollPeriodSeconds = -1;

static int CreateTimers(void);
static void IconShowEventHandler(EventLoopTimer* timer);
static void AzureTimerEventHandler(EventLoopTimer* timer);
static void BatteryTimerEventHandler(EventLoopTimer* timer);
static void IntercoreTimerEventHandler(EventLoopTimer* timer);
static void ImuStableTimerEventHandler(EventLoopTimer* timer);
static int GetBatteryLevel(void);

static struct UPDATE_ACTIVE updateStruct;

static volatile sig_atomic_t terminationRequired = false;

static bool haveFirstDeviceData = false;    // set true once we have telemetry from the device.

/// <summary>
///     This function matches the SysEvent_EventsCallback signature, and is invoked
///     from the event loop when the system wants to perform an application or system update.
///     See <see cref="SysEvent_EventsCallback" /> for information about arguments.
/// </summary>
static void UpdateCallback(SysEvent_Events event, SysEvent_Status status, const SysEvent_Info* info,
    void* context)
{
    Log_Debug("SysEvent_EventsCallback\n");

    if (event != SysEvent_Events_UpdateReadyForInstall) {
        Log_Debug("ERROR: unexpected event: 0x%x\n", event);
        return;
    }

    // Print information about received message.
    Log_Debug("INFO: Status: %s (%u)\n", EventStatusToString(status), status);

    SysEvent_Info_UpdateData data;
    int result = SysEvent_Info_GetUpdateData(info, &data);

    if (result == -1) {
        Log_Debug("ERROR: SysEvent_Info_GetUpdateData failed: %s (%d).\n", strerror(errno), errno);
        return;
    }

    Log_Debug("INFO: Max deferral time: %u minutes\n", data.max_deferral_time_in_minutes);
    Log_Debug("INFO: Update Type: %s (%u).\n", UpdateTypeToString(data.update_type), data.update_type);

    switch (status) {
        // If an update is pending, and the user has not allowed updates, then defer the update.
    case SysEvent_Status_Pending:
        // allow the update if the device is laying down (< 45 degrees roll angle).
        if (fabs(device_status.roll) < 45 && haveFirstDeviceData) {
            Log_Debug("INFO: Allowing update - device is at %3.2f degrees\n", fabs(device_status.roll));
            updateDeferred = false;
            currentIcon = UpdateApp;
            updateStruct.id = MSG_UPDATE_ACTIVE;
            updateStruct.updateActive = true;
            EnqueueIntercoreMessage(&updateStruct, sizeof(updateStruct));
        }
        else
        {
            // if the device is not laying down, defer the update.
            Log_Debug("INFO: Deferring update - device is upright (%3.2f degrees), or we don't have device telemetry\n", fabs(device_status.roll));
            result = SysEvent_DeferEvent(SysEvent_Events_UpdateReadyForInstall, 1);
            updateDeferred = true;
            currentIcon = UpdateDeferred;
        }
        UpdateDisplay(BatteryLevel, HaveNetwork, HaveIoTC, currentIcon);
        break;

    case SysEvent_Status_Final:
        Log_Debug("INFO: Final update. App will update in 10 seconds.\n");
        // The application may be restarted before the update is applied.
        updateApplied = true;
        ShowUpdatingIcon();
        break;

    case SysEvent_Status_Deferred:
        Log_Debug("INFO: Update deferred.\n");
        break;

    case SysEvent_Status_Complete:
    default:
        Log_Debug("ERROR: Unexpected status %d.\n", status);
        terminationRequired = true;
        break;
    }
}

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}

int main(int argc, char* argv[])
{
    Log_Debug("App Starting...\n");

    if (argc >= 2) {
        Log_Debug("Setting Azure Scope ID %s\n", argv[1]);
        strncpy(scopeId, argv[1], SCOPEID_LENGTH);
    }
    else {
        Log_Debug("ScopeId needs to be set in the app_manifest CmdArgs\n");
        return -1;
    }

    char deviceTwinString[20];
    // get last device twin version.
    if (GetProfileString("DeviceTwinVersion", deviceTwinString, 20) != -1)
    {
        // set last DeviceTwin version, so we don't duplicate that behavior.
        lastDeviceTwinVersion = atol(deviceTwinString);
    }


    // compare current/last ScopeId and reset IoTC Message value
    char lastScopeId[20];
    if (GetProfileString("LastScopeId", &lastScopeId[0], 20) != -1)
    {
        if (strncasecmp(lastScopeId, scopeId, 20) != 0)
        {
            Log_Debug("Resetting lastDeviceTwinVersion - this/last ScopeIDs don't matchz\n");
            lastDeviceTwinVersion = 0;
        }
    }
    // save current scope id to 'lastScopeID'
    WriteProfileString("LastScopeId", scopeId);

    if (argc == 3)
    {
        if (strncasecmp("appa", argv[2], 4) == 0)
        {
            isAppA = true;
            currentIcon = AppA;
        }

        if (strncasecmp("appb", argv[2], 4) == 0)
        {
            isAppA = false;
            currentIcon = AppB;
        }
    }

    SetupADC();
    InitUDPThread();

    // setup timers.
    int ret = CreateTimers();
    if (ret == -1)
    {
        Log_Debug("Failed to setup data refresh timers...\n");
    }

    InitInterCoreCommunications(eventLoop);

    // enable the motors on the M4 app.
    struct UPDATE_ACTIVE updateActive;
    updateActive.id = MSG_UPDATE_ACTIVE;
    updateActive.updateActive = false;
    EnqueueIntercoreMessage(&updateActive, sizeof(updateActive));

	curl_global_init(CURL_GLOBAL_DEFAULT);

	// initialize the SSD1306 display
	SSD1306_Init(true);
	delay(10);

	SSD1306_RotateImage(App_A_Icon, AppA_Rot180, 32, 32, 90);
	SSD1306_RotateImage(App_B_Icon, AppB_Rot180, 32, 32, 90);

	SSD1306_RotateImage(Wifi_Icon, WiFiIcon_Rot180, 32, 32, 180);
	SSD1306_RotateImage(Wifi_Icon, WiFiIcon_Rot180, 32, 32, 180);
	SSD1306_RotateImage(IoTC_Icon, IotcIcon_Rot180, 32, 32, 180);
	SSD1306_RotateImage(Update_Icon, UpdateIcon_Rot180, 32, 32, 180);
    SSD1306_RotateImage(Update_Icon_Defer_Rejected, Update_Icon_Defer_Rejected_Rot180, 32, 32, 180);

    BatteryLevel = GetBatteryLevel();

    // clear wait icon once we have imuStable from intercore comms.
    ShowWaitIcon();
    // UpdateDisplay(BatteryLevel, HaveNetwork, HaveIoTC, currentIcon);

	// Register a SIGTERM handler for termination requests
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = TerminationHandler;
	sigaction(SIGTERM, &action, NULL);

	// Main loop
	while (!terminationRequired) {
		EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
		// Continue if interrupted by signal, e.g. due to breakpoint being set.
		if (result == EventLoop_Run_Failed && errno != EINTR) {
			terminationRequired = true;
		}
	}

    // show 'updating' icon.
    if (updateApplied)
    {
        ShowUpdatingIcon();
    }
    else
    {
        // show 'white' display
        SSD1306_Clear();
        SSD1306_FillRegion(fillBuffer, 32, 128, 0, 0, 32, 128, true);
        SSD1306_DrawImage(fillBuffer, 32, 128, 0, 0);
        SSD1306_Display();
    }
}

static void ImuStableTimerEventHandler(EventLoopTimer* timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        return;
    }

    if (!imuStable)
    {
        struct IMU_STABLE_REQUEST sReq;
        sReq.id = MSG_IMU_STABLE_REQUEST;
        EnqueueIntercoreMessage(&sReq, sizeof(sReq));
    }
}

static void IconShowEventHandler(EventLoopTimer* timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        return;
    }

    if (imuStable)
    {
        // switch back to 'normal' icon.
        if (currentIcon == UpdateDeferred || WaitForIMU)
        {
            WaitForIMU = false;
            currentIcon = isAppA ? AppA : AppB;
            UpdateDisplay(BatteryLevel, HaveNetwork, HaveIoTC, currentIcon);
        }
    }
}

static int GetBatteryLevel(void)
{
    uint32_t sampleValue = 0;
    int result = ADC_Poll(adcControllerFd, 0, &sampleValue);
    if (result == -1)
    {
        return;
    }

    float voltage = (((float)sampleValue * sampleMaxVoltage) / (float)((1 << sampleBitCount) - 1) * 2);

    // shouldn't be more than 4.5, but check anyway.
    if (voltage > 3.9)
        voltage = 3.9;

    if (voltage < 3.50)
        voltage = 3.50;

    // this will have a value from 3.90 to 3.50 (based on Rechargeable batteries)
    // 0.4 difference
    float batteryLevel = (voltage - 3.50) * 250;

    int _BatteryLevel = (int)batteryLevel;
    return _BatteryLevel;
}

static void BatteryTimerEventHandler(EventLoopTimer* timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        return;
    }

    BatteryLevel = GetBatteryLevel();
    SendTelemetryInt("BatteryLevel", BatteryLevel);
}

static void SendTelemetryInt(const unsigned char* key, const int value)
{
    static char eventBuffer[100] = { 0 };
    static const char* EventMsgTemplate = "{ \"%s\": %d }";
    int len = snprintf(eventBuffer, sizeof(eventBuffer), EventMsgTemplate, key, value);
    if (len < 0)
        return;

    SendIoTMessageRaw(eventBuffer);
}

/// <summary>
/// Azure timer event:  Check connection status and send telemetry
/// </summary>
static void AzureTimerEventHandler(EventLoopTimer* timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        return;
    }

    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) != -1) {
        if (isNetworkReady && !iothubAuthenticated) {
            SetupAzureClient();
        }
    }
    else {
        Log_Debug("Failed to get Network state\n");
    }

    if (isNetworkReady)
    {
        Log_Debug("Have network\r\n");
        HaveNetwork = true;
    }
    else
    {
        HaveNetwork = false;
    }

    if (iothubAuthenticated)
    {
        HaveIoTC = true;
    }
    else
    {
        HaveIoTC = false;
    }

    // update status on the SSD1306
    UpdateDisplay(BatteryLevel, HaveNetwork, HaveIoTC, currentIcon);

    if (iothubAuthenticated) {
        IoTHubDeviceClient_LL_DoWork(iothubClientHandle);
    }
}

static int CreateTimers(void)
{
	eventLoop = EventLoop_Create();
	if (eventLoop == NULL) {
		Log_Debug("Could not create event loop.\n");
		return -1;
	}

    azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
    struct timespec azureTelemetryPeriod = { .tv_sec = azureIoTPollPeriodSeconds, .tv_nsec = 0 };
    azureTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &AzureTimerEventHandler, &azureTelemetryPeriod);
    if (azureTimer == NULL) {
        return -1;
    }

    struct timespec imuStableMsgPeriod = { .tv_sec = 1, .tv_nsec = 0 };
    imuStableTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &ImuStableTimerEventHandler, &imuStableMsgPeriod);
    if (imuStableTimer == NULL) {
        return -1;
    }


    struct timespec iconShowPeriod = { .tv_sec = 5, .tv_nsec = 0 };
    iconShowTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &IconShowEventHandler, &iconShowPeriod);
    if (iconShowTimer == NULL) {
        return -1;
    }

    struct timespec batteryReadPeriod = { .tv_sec = 60, .tv_nsec = 0 };
    batteryTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &BatteryTimerEventHandler, &batteryReadPeriod);
    if (batteryTimer == NULL) {
        return -1;
    }

    // read device state every 10 seconds
    struct timespec intercoreReadPeriod = { .tv_sec = 5, .tv_nsec = 0 };
    intercoreTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &IntercoreTimerEventHandler, &intercoreReadPeriod);
    if (intercoreTimer == NULL) {
        return -1;
    }

    updateEventReg = SysEvent_RegisterForEventNotifications(
        eventLoop, SysEvent_Events_UpdateReadyForInstall, UpdateCallback, NULL);
    if (updateEventReg == NULL) {
        Log_Debug("ERROR: could not register update event: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

	return 0;
}

/// <summary>
///     Sets the IoT Hub authentication state for the app
///     The SAS Token expires which will set the authentication state
/// </summary>
static void HubConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result,
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
    void* userContextCallback)
{
    iothubAuthenticated = (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED);
    Log_Debug("IoT Hub Authenticated: %s\n", GetReasonString(reason));
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     When the SAS Token for a device expires the connection needs to be recreated
///     which is why this is not simply a one time call.
/// </summary>
static void SetupAzureClient(void)
{
    if (iothubClientHandle != NULL) {
        IoTHubDeviceClient_LL_Destroy(iothubClientHandle);
    }

    AZURE_SPHERE_PROV_RETURN_VALUE provResult =
        IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning(scopeId, 10000,
            &iothubClientHandle);
        Log_Debug("IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning returned '%s'.\n",
        getAzureSphereProvisioningResultString(provResult));
        if (provResult.result == AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR) {
            Log_Debug("prov_device_error is %d.\n", provResult.prov_device_error);
        }
        else if (provResult.result == AZURE_SPHERE_PROV_RESULT_IOTHUB_CLIENT_ERROR) {
            Log_Debug("iothub_client_error is %d.\n", provResult.iothub_client_error);
        }

    if (provResult.result != AZURE_SPHERE_PROV_RESULT_OK) {
        bool connected = false;
        Networking_IsNetworkingReady(&connected);
        Log_Debug("Trying IoTC connection - Networking is %s\n", connected ? "Ready" : "Not Ready");

        struct timespec azureTelemetryPeriod = { azureIoTPollPeriodSeconds, 0 };
        SetEventLoopTimerPeriod(azureTimer, &azureTelemetryPeriod);

        Log_Debug("ERROR: failure to create IoTHub Handle - will retry in %i seconds.\n",
            azureIoTPollPeriodSeconds);
        return;
    }

    // Successfully connected, so make sure the polling frequency is back to the default
    azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
    struct timespec azureTelemetryPeriod = { .tv_sec = azureIoTPollPeriodSeconds, .tv_nsec = 0 };
    SetEventLoopTimerPeriod(azureTimer, &azureTelemetryPeriod);

    iothubAuthenticated = true;

    if (IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_KEEP_ALIVE,
        &keepalivePeriodSeconds) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: failure setting option \"%s\"\n", OPTION_KEEP_ALIVE);
        return;
    }

    IoTHubDeviceClient_LL_SetDeviceTwinCallback(iothubClientHandle, TwinCallback, NULL);
    IoTHubDeviceClient_LL_SetConnectionStatusCallback(iothubClientHandle,
        HubConnectionStatusCallback, NULL);
}

/// <summary>
///     Callback invoked when a Device Twin update is received from IoT Hub.
///     Updates local state for 'showEvents' (bool).
/// </summary>
/// <param name="payload">contains the Device Twin JSON document (desired and reported)</param>
/// <param name="payloadSize">size of the Device Twin JSON document</param>
static void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload,
    size_t payloadSize, void* userContextCallback)
{
    size_t nullTerminatedJsonSize = payloadSize + 1;
    char* nullTerminatedJsonString = (char*)malloc(nullTerminatedJsonSize);
    if (nullTerminatedJsonString == NULL) {
        Log_Debug("ERROR: Could not allocate buffer for twin update payload.\n");
        abort();
    }

    // Copy the provided buffer to a null terminated buffer.
    memcpy(nullTerminatedJsonString, payload, payloadSize);
    // Add the null terminator at the end.
    nullTerminatedJsonString[nullTerminatedJsonSize - 1] = 0;

    Log_Debug("INFO: DeviceTwinCallback - last Device Twin Version: %d\n", lastDeviceTwinVersion);
    Log_Debug("INFO: Device Twin Rx - %s\n", nullTerminatedJsonString);


    JSON_Value* rootProperties = NULL;
    rootProperties = json_parse_string(nullTerminatedJsonString);
    if (rootProperties == NULL) {
        Log_Debug("WARNING: Cannot parse the string as JSON content.\n");
        goto cleanup;
    }

    JSON_Object* rootObject = json_value_get_object(rootProperties);
    JSON_Object* desiredProperties = json_object_dotget_object(rootObject, "desired");
    if (desiredProperties == NULL) {
        desiredProperties = rootObject;
    }

    char MsgBuffer[10];
    memset(&MsgBuffer[0], 0x00, 10);

    JSON_Value* value = json_object_get_value(desiredProperties, "$version");

    bool processMessage = false;

    if (value != NULL)
    {
        size_t iVersion = (size_t)json_value_get_number(value);
        if (iVersion > lastDeviceTwinVersion)
        {
            processMessage = true;
            snprintf(MsgBuffer, 10, "%d", iVersion);
            WriteProfileString("DeviceTwinVersion", MsgBuffer);
            lastDeviceTwinVersion = iVersion;
            Log_Debug("Msg Version Updated: %d\n", iVersion);
        }
        else
        {
            Log_Debug("warning: Duplicate Device Twin Message: Version %d\n", iVersion);
        }
    }

    if (!processMessage)
    {
        Log_Debug("Duplicate Message - Bail\n");
        goto cleanup;
    }

    Log_Debug("Processing Device Twin Message\n");

    // Handle the Device Twin Desired Properties here.
    int compassHeading = -1;

    // get the heading from the Json - if there's no heading then don't change the direction properties.
    JSON_Value* Heading = json_object_get_value(desiredProperties, "DesiredHeading");
    if (Heading != NULL) {
        compassHeading = (int)json_value_get_number(Heading);
        TwinReportIntState("DesiredHeading", compassHeading, lastDeviceTwinVersion);

        struct TURN_ROBOT DirMsg = {
            .id = MSG_TURN_ROBOT,
            .heading = compassHeading,
            .enabled = true
        };

        EnqueueIntercoreMessage(&DirMsg, sizeof(DirMsg));
        Log_Debug("INFO: Setting 'TURN_NORTH' data: heading: %d\n", compassHeading);
    }

cleanup:
    // Release the allocated memory.
    json_value_free(rootProperties);
    free(nullTerminatedJsonString);
}

/// <summary>
///     Converts the IoT Hub connection status reason to a string.
/// </summary>
static const char* GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    static char* reasonString = "unknown reason";
    switch (reason) {
    case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
        reasonString = "IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN";
        break;
    case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
        reasonString = "IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED";
        break;
    case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
        reasonString = "IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL";
        break;
    case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
        reasonString = "IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED";
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
        reasonString = "IOTHUB_CLIENT_CONNECTION_NO_NETWORK";
        break;
    case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
        reasonString = "IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR";
        break;
    case IOTHUB_CLIENT_CONNECTION_OK:
        reasonString = "IOTHUB_CLIENT_CONNECTION_OK";
        break;
    }
    return reasonString;
}

/// <summary>
///     Converts AZURE_SPHERE_PROV_RETURN_VALUE to a string.
/// </summary>
static const char* getAzureSphereProvisioningResultString(
    AZURE_SPHERE_PROV_RETURN_VALUE provisioningResult)
{
    switch (provisioningResult.result) {
    case AZURE_SPHERE_PROV_RESULT_OK:
        return "AZURE_SPHERE_PROV_RESULT_OK";
    case AZURE_SPHERE_PROV_RESULT_INVALID_PARAM:
        return "AZURE_SPHERE_PROV_RESULT_INVALID_PARAM";
    case AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY:
        return "AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY";
    case AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY:
        return "AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY";
    case AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR:
        return "AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR";
    case AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR:
        return "AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR";
    default:
        return "UNKNOWN_RETURN_VALUE";
    }
}

/// <summary>
///     Callback confirming message delivered to IoT Hub.
/// </summary>
/// <param name="result">Message delivery status</param>
/// <param name="context">User specified context</param>
static void SendMessageCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* context)
{
    Log_Debug("INFO: Message received by IoT Hub. Result is: %d\n", result);
}

/// <summary>
///     Callback invoked when the Device Twin reported properties are accepted by IoT Hub.
/// </summary>
static void ReportStatusCallback(int result, void* context)
{
    Log_Debug("INFO: Device Twin reported properties update result: HTTP status code %d\n", result);
}

void UpdateDisplay(uint8_t batteryLevel, bool haveNetwork, bool haveIoTC, int AppUpdateIcon)
{
    // don't update the display if we're applying an update or waiting for IMU stability.
    if (updateApplied || WaitForIMU)
        return;
    // update the display.
    SSD1306_Clear();
    memset(BatteryIcon_Rot180, 0x00, 128);

    Log_Debug("Battery: %d, Network: %s, IoTC: %s, AppUpdateIcon %d\n", batteryLevel, haveNetwork ? "Yes" : "No", haveIoTC ? "Yes" : "No", AppUpdateIcon);

    if (batteryLevel == 255)
    {
        memcpy(BatteryIcon_Rot180, low_Batt, 128);
    }
    else
    {
        if (batteryLevel > 90 && batteryLevel <= 100)
        {
            SSD1306_RotateImage(Battery_Icon100, BatteryIcon_Rot180, 32, 32, 180);
        }

        if (batteryLevel > 80 && batteryLevel <= 90)
        {
            SSD1306_RotateImage(Battery_Icon90, BatteryIcon_Rot180, 32, 32, 180);
        }

        if (batteryLevel > 70 && batteryLevel <= 80)
        {
            SSD1306_RotateImage(Battery_Icon80, BatteryIcon_Rot180, 32, 32, 180);
        }

        if (batteryLevel > 60 && batteryLevel <= 70)
        {
            SSD1306_RotateImage(Battery_Icon70, BatteryIcon_Rot180, 32, 32, 180);
        }

        if (batteryLevel > 50 && batteryLevel <= 60)
        {
            SSD1306_RotateImage(Battery_Icon60, BatteryIcon_Rot180, 32, 32, 180);
        }

        if (batteryLevel > 40 && batteryLevel <= 50)
        {
            SSD1306_RotateImage(Battery_Icon50, BatteryIcon_Rot180, 32, 32, 180);
        }

        if (batteryLevel > 30 && batteryLevel <= 40)
        {
            SSD1306_RotateImage(Battery_Icon40, BatteryIcon_Rot180, 32, 32, 180);
        }

        if (batteryLevel > 20 && batteryLevel <= 30)
        {
            SSD1306_RotateImage(Battery_Icon30, BatteryIcon_Rot180, 32, 32, 180);
        }

        if (batteryLevel > 10 && batteryLevel <= 20)
        {
            SSD1306_RotateImage(Battery_Icon20, BatteryIcon_Rot180, 32, 32, 180);
        }

        if (batteryLevel > 0 && batteryLevel <= 10)
        {
            SSD1306_RotateImage(Battery_Icon10, BatteryIcon_Rot180, 32, 32, 180);
        }

        if (batteryLevel == 0)
        {
            SSD1306_RotateImage(Battery_Icon00, BatteryIcon_Rot180, 32, 32, 180);
        }
    }
    SSD1306_DrawImage(&BatteryIcon_Rot180[0], 32, 32, 96, 0);

    if (haveNetwork)
    {
        SSD1306_DrawImage(&WiFiIcon_Rot180[0], 32, 32, 64, 0);
    }
    else
    {
        SSD1306_DrawImage(&not_WiFi[0], 32, 32, 64, 0);
    }

    if (haveIoTC)
    {
        SSD1306_DrawImage(&IotcIcon_Rot180[0], 32, 32, 32, 0);
    }
    else
    {
        SSD1306_DrawImage(&not_IoTC[0], 32, 32, 32, 0);
    }

    if (updateDeferred)
    {
        SSD1306_DrawImage(&Update_Icon_Defer_Rejected_Rot180[0], 32, 32, 0, 0);
    }
    else
    {
        switch (AppUpdateIcon) {
        case UpdateApp:
            SSD1306_DrawImage(&UpdateIcon_Rot180[0], 32, 32, 0, 0);
            break;
        case AppA:
            SSD1306_DrawImage(&AppA_Rot180[0], 32, 32, 0, 0);
            break;
        case AppB:
            SSD1306_DrawImage(&AppB_Rot180[0], 32, 32, 0, 0);
            break;
        default:
            break;
        }
    }

    SSD1306_Display();
}

/// <summary>
///     Convert the supplied system event status to a human-readable string.
/// </summary>
/// <param name="status">The status.</param>
/// <returns>String representation of the supplied status.</param>
static const char* EventStatusToString(SysEvent_Status status)
{
    switch (status) {
    case SysEvent_Status_Invalid:
        return "Invalid";
    case SysEvent_Status_Pending:
        return "Pending";
    case SysEvent_Status_Final:
        return "Final";
    case SysEvent_Status_Deferred:
        return "Deferred";
    case SysEvent_Status_Complete:
        return "Completed";
    default:
        return "Unknown";
    }
}

/// <summary>
///     Convert the supplied update type to a human-readable string.
/// </summary>
/// <param name="updateType">The update type.</param>
/// <returns>String representation of the supplied update type.</param>
static const char* UpdateTypeToString(SysEvent_UpdateType updateType)
{
    switch (updateType) {
    case SysEvent_UpdateType_Invalid:
        return "Invalid";
    case SysEvent_UpdateType_App:
        return "Application";
    case SysEvent_UpdateType_System:
        return "System";
    default:
        return "Unknown";
    }
}

void SetupADC(void)
{
    adcControllerFd = ADC_Open(0);
    sampleBitCount = ADC_GetSampleBitCount(adcControllerFd, 0);
    Log_Debug("ADC - Sample Bits = %d\n", sampleBitCount);
    sampleMaxVoltage = 2.5f;
    ADC_SetReferenceVoltage(adcControllerFd, 0, sampleMaxVoltage);
}

static void IntercoreTimerEventHandler(EventLoopTimer* timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        return;
    }

    Log_Debug("Sending telemetry request to M4\n");
    Log_Debug("Sending telemetry request to M4\n");
    struct TELEMETRY_REQUEST msg = {
        .id = MSG_TELEMETRY_REQUEST
    };
    EnqueueIntercoreMessage(&msg, sizeof(msg));
}

/// <summary>
///     Handle socket event by reading incoming data from real-time capable application.
/// </summary>
void SocketEventHandler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context)
{
    // Read response from real-time capable application.
    char rxBuf[64];
    int bytesReceived = recv(fd, rxBuf, sizeof(rxBuf), 0);
    char spBuffer[10];  // used to write the current setpoint to isolated storage.

    if (bytesReceived == -1) {
        Log_Debug("ERROR: Unable to receive message: %d (%s)\n", errno, strerror(errno));
        return;
    }
    else if (bytesReceived == 0) {
        Log_Debug("ERROR: received 0 bytes from intercore message\n");
        return;
    }

    Log_Debug("Have Intercore Msg\n");
    Log_Debug("Have Intercore Msg\n");

    char CompassDirection[10];

    switch (rxBuf[0]) {
    case MSG_IMU_STABLE_RESULT:
        // stop timer icon if imuState = true;
        if (bytesReceived >= sizeof(struct IMU_STABLE_RESULT))
        {
            struct IMU_STABLE_RESULT* pImuResult = (struct IMU_STABLE_RESULT*)rxBuf;
            imuStable = pImuResult->imuStable;
        }
        break;
    case MSG_TURN_DETAILS:
        if (bytesReceived >= sizeof(struct TURN_DETAILS))
        {
            struct TURN_DETAILS* pTurnStatus = (struct TURN_DETAILS*)rxBuf;
            Log_Debug("INFO: Turn Complete: Start Heading %3.2f, End Heading %3.2f\n", pTurnStatus->startHeading, pTurnStatus->endHeading);
        }
        break;
    case MSG_DEVICE_STATUS:
        if (bytesReceived >= sizeof(struct DEVICE_STATUS))
        {
            struct DEVICE_STATUS* pDevStatus = (struct DEVICE_STATUS*)rxBuf;
            haveFirstDeviceData = true;
            // copy latest value to the device status folder.
            memcpy(&device_status, pDevStatus, sizeof(device_status));
            GetCompassDirection(pDevStatus->yaw, &CompassDirection[0],10);

            Log_Debug("%8u: Yaw: %3.2f | Roll: %3.2f | Setpoint: %3.2f (output: %3.2f) | Obstacles %u (active: %s) | Turn North: %s\r\n", pDevStatus->timestamp, pDevStatus->yaw, pDevStatus->roll, pDevStatus->setpoint, pDevStatus->output, pDevStatus->numObstaclesDetected, pDevStatus->avoidActive ? "yes" : "no", pDevStatus->turnNorth == true ? "yes" : "no");

            if (pDevStatus->setpoint > 80 && pDevStatus->setpoint < 100)
            {
                snprintf(spBuffer, 10, "%3.2f", pDevStatus->setpoint);
                WriteProfileString("Setpoint", spBuffer);
            }

            static unsigned long telemetryCount = 0;
            telemetryCount++;

            if (telemetryCount == 20)   // 20 seconds.
            {
                telemetryCount = 0;
                snprintf(telemetryMessage, 255, "{\"BatteryLevel\": %d, \"Heading\": %d, \"HeadingCompass\": \"%s\", \"ObstaclesAvoided\": %u, \"CurrentApp\": \"%s\" }", BatteryLevel, (int)pDevStatus->yaw, CompassDirection, pDevStatus->numObstaclesDetected, isAppA == true ? "A" : "B");
                Log_Debug(telemetryMessage);
                SendIoTMessageRaw(telemetryMessage);
            }

            if (updateDeferred == true && pDevStatus->roll <= 45)
            {
                SysEvent_ResumeEvent(SysEvent_Events_UpdateReadyForInstall);
                updateDeferred = false;
                currentIcon = UpdateApp;
                UpdateDisplay(BatteryLevel, HaveNetwork, HaveIoTC, currentIcon);
            }
        }
        break;
    default:
        Log_Debug("ERROR: Unexpected message id %d from bare-metal\n", rxBuf[0]);
        break;
    }
}

void SendIoTMessageRaw(const char* message)
{
    bool isNetworkingReady = false;
    if ((Networking_IsNetworkingReady(&isNetworkingReady) == -1) || !isNetworkingReady) {
        Log_Debug("WARNING: Cannot send IoTHubMessage because network is not up.\n");
        return;
    }

    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString(message);

    if (messageHandle == 0) {
        Log_Debug("WARNING: unable to create a new IoTHubMessage\n");
        return;
    }

    if (IoTHubDeviceClient_LL_SendEventAsync(iothubClientHandle, messageHandle, SendMessageCallback,
        /*&callback_param*/ 0) != IOTHUB_CLIENT_OK) {
        Log_Debug("WARNING: failed to hand over the message to IoTHubClient\n");
    }
    else {
        Log_Debug("INFO: IoTHubClient accepted the message for delivery\n");
    }

    IoTHubMessage_Destroy(messageHandle);
}

static char reportedPropertiesString[120];

static void TwinReportIntState(const char* propertyName, int propertyValue, int messageVersion)
{
    if (iothubClientHandle == NULL) {
        Log_Debug("ERROR: client not initialized\n");
    }
    else {
        memset(&reportedPropertiesString[0], 0x00, 120);
        int len = snprintf(reportedPropertiesString, 120, "\{ \"%s\": { \"value\": %d, \"statusCode\": 200, \"status\": \"completed\", \"desiredVersion\": %d }}", propertyName, propertyValue, messageVersion);
        if (len < 0)
            return;

        //#ifdef SHOW_DEBUG_MSGS
        Log_Debug("TwinReportIntState:\n%s", reportedPropertiesString);
        //#endif

        if (IoTHubDeviceClient_LL_SendReportedState(
            iothubClientHandle, (unsigned char*)reportedPropertiesString,
            strlen(reportedPropertiesString), ReportStatusCallback, 0) != IOTHUB_CLIENT_OK) {
            Log_Debug("ERROR: failed to set reported state for '%s'.\n", propertyName);
        }
        else {
            Log_Debug("INFO: Reported state for '%s' to value '%d'.\n", propertyName, propertyValue);
        }
    }
}

const char* CompassArray[16] = { "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW" };

void GetCompassDirection(float compassAngle, char* CompassString, size_t length)
{
    if (compassAngle < 0)
        compassAngle = 360 - compassAngle;
    int pos = (int)((compassAngle / 22.5) + .5);

    strncpy(CompassString,CompassArray[pos % 16],10);
}

static void ShowUpdatingIcon(void)
{
    SSD1306_Clear();
    SSD1306_DrawImage(Update_Final, 128, 32, 0, 0);
    SSD1306_Display();
}

static void ShowWaitIcon(void)
{
    WaitForIMU = true;
    SSD1306_Clear();
    SSD1306_DrawImage(wait_logo, 128, 32, 0, 0);
    SSD1306_Display();
}

void InitUDPThread(void)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&UDP_Thread, &attr, UDPReadThread, (void*)0);
}

static void* UDPReadThread(void* arg)
{
    int sockfd;     /* socket */
    int portno;     /* port to listen on */
    int clientlen;      /* byte size of client's address */
    struct sockaddr_in serveraddr;  /* server's addr */
    struct sockaddr_in clientaddr;  /* client addr */
    char* buf;      /* message buf */
    int optval;     /* flag value for setsockopt */
    int n;          /* message byte size */

    portno = 1825;
    int BUFSIZE = 10;

    struct REMOTE_CMD remoteCmd;
    remoteCmd.id = MSG_REMOTE_CMD;
    remoteCmd.cmd = 0x04;   //stop (default message).

    Log_Debug("UDP Rx Thread starting...\n");

    /*
     * socket: create the parent socket
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets
     * us rerun the server immediately after we kill it;
     * otherwise we have to wait about 20 secs.
     * Eliminates "ERROR on binding: Address already in use" error.
     */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
        (const void*)&optval, sizeof(int));

    /*
     * build the server's Internet address
     */
    bzero((char*)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    /*
     * bind: associate the parent socket with a port
     */
    if (bind(sockfd, (struct sockaddr*)&serveraddr,
        sizeof(serveraddr)) < 0)
        error("ERROR on binding");

    /*
     * main loop: wait for a datagram, then echo it
     */
    clientlen = sizeof(clientaddr);

    buf = malloc(BUFSIZE);

    while (1) {
        /*
         * recvfrom: receive a UDP datagram from a client
         */
        memset(buf, 0x00, BUFSIZE);

        n = recvfrom(sockfd, buf, BUFSIZE, 0,
            (struct sockaddr*)&clientaddr, &clientlen);
        if (n < 0)
            error("ERROR in recvfrom");

        if (n == 3 && buf[0] == 'R' && buf[1] == 'T' && buf[2] <= 6)
        {
            Log_Debug("UDP Command %d\n", buf[2]);
            // 0-4 = left, right, forward, back, and stop
            // 5 = reboot.
            if (buf[2] == 0x05 || buf[2] == 0x06)
            {
                switch (buf[2])
                {
                case 5:
                    Log_Debug("UDP Reboot\n");
                    PowerManagement_ForceSystemReboot();
                    break;
                case 6:
                    Log_Debug("Clear Device Twin version\n");
                    WriteProfileString("DeviceTwinVersion", "0");
                    lastDeviceTwinVersion = 0;
                    break;
                default:
                    break;
                }
            }
            else
            {
                remoteCmd.cmd = buf[2];
            }
            EnqueueIntercoreMessage(&remoteCmd, sizeof(remoteCmd));
        }
    }
    return (void*)0;
}

void error(char* msg)
{
    perror(msg);
    exit(1);
}
