/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <memory.h>
#include <stdlib.h>

#include <applibs/eventloop.h>
#include <applibs/log.h>

#include "parson.h"

#include "azure_iot.h"
#include "cloud.h"
#include "exitcodes.h"

// This file implements the interface described in cloud.h in terms of an Azure IoT Hub.
// Specifically, it translates Azure IoT Hub specific concepts (events, device twin messages, device
// methods, etc) into business domain concepts (telemetry, upload enabled, alarm raised)

static const char azureSphereModelId[] = "dtmi:com:example:azuresphere:thermometer;1";

// Azure IoT Hub callback handlers
static void DeviceTwinCallbackHandler(const char *nullTerminatedJsonString);
static int DeviceMethodCallbackHandler(const char *methodName, const unsigned char *payload,
                                       size_t payloadSize, unsigned char **response,
                                       size_t *responseSize);
static void ConnectionChangedCallbackHandler(bool connected);

// Default handlers for cloud events
// static void DefaultTelemetryUploadEnabledChangedHandler(bool uploadEnabled);
static void DefaultDisplayAlertHandler(const char *alertMessage);
static void DefaultConnectionChangedHandler(bool connected);

// Cloud event callback handlers
// static Cloud_TelemetryUploadEnabledChangedCallbackType
//     thermometerTelemetryUploadEnabledChangedCallbackFunction =
//         DefaultTelemetryUploadEnabledChangedHandler;
static Cloud_DesiredRecipeChangedCallbackType desiredRecipeChangedCallbackFunction = NULL;

static Cloud_DisplayAlertCallbackType displayAlertCallbackFunction = DefaultDisplayAlertHandler;
static Cloud_ConnectionChangedCallbackType connectionChangedCallbackFunction =
    DefaultConnectionChangedHandler;

// Utility functions
static Cloud_Result AzureIoTToCloudResult(AzureIoT_Result result);

// Constants
#define MAX_PAYLOAD_SIZE 512

unsigned int latestVersion = 1;

ExitCode Cloud_Initialize(EventLoop *el, void *backendContext,
                          ExitCode_CallbackType failureCallback,
                          Cloud_DesiredRecipeChangedCallbackType desiredRecipeChangedCallback,
                        //   Cloud_TelemetryUploadEnabledChangedCallbackType
                        //       thermometerTelemetryUploadEnabledChangedCallback,
                          Cloud_DisplayAlertCallbackType displayAlertCallback,
                          Cloud_ConnectionChangedCallbackType connectionChangedCallback)
{
    // if (thermometerTelemetryUploadEnabledChangedCallback != NULL) {
    //     thermometerTelemetryUploadEnabledChangedCallbackFunction =
    //         thermometerTelemetryUploadEnabledChangedCallback;
    // }

    if (displayAlertCallback != NULL) {
        displayAlertCallbackFunction = displayAlertCallback;
    }

    if (connectionChangedCallback != NULL) {
        connectionChangedCallbackFunction = connectionChangedCallback;
    }

    desiredRecipeChangedCallbackFunction = desiredRecipeChangedCallback;

    AzureIoT_Callbacks callbacks = {
        .connectionStatusCallbackFunction = ConnectionChangedCallbackHandler,
        .deviceTwinReceivedCallbackFunction = DeviceTwinCallbackHandler,
        .deviceTwinReportStateAckCallbackTypeFunction = NULL,
        .sendTelemetryCallbackFunction = NULL,
        .deviceMethodCallbackFunction = DeviceMethodCallbackHandler};

    return AzureIoT_Initialize(el, failureCallback, azureSphereModelId, backendContext, callbacks);
}

void Cloud_Cleanup(void)
{
    AzureIoT_Cleanup();
}

static Cloud_Result AzureIoTToCloudResult(AzureIoT_Result result)
{
    switch (result) {
    case AzureIoT_Result_OK:
        return Cloud_Result_OK;
    case AzureIoT_Result_NoNetwork:
        return Cloud_Result_NoNetwork;
    case AzureIoT_Result_OtherFailure:
    default:
        return Cloud_Result_OtherFailure;
    }
}

Cloud_Result Cloud_SendRecipeCampaignChangedEvent(DeviceUpdateRequest request, bool failed) {
    JSON_Value *uploadValue = json_value_init_object();
    JSON_Object *root = json_value_get_object(uploadValue);

    if (failed) {
        json_object_dotset_string(root, "recipe_campaign.failed.uuid", request.uuid);
        json_object_dotset_number(root, "failed.version", latestVersion);        
    } else {
        json_object_dotset_string(root, "recipe_campaign.uuid", request.uuid);
        json_object_dotset_number(root, "version", latestVersion++);
    }
    
    char *serializedTelemetryUpload = json_serialize_to_string(uploadValue);
    AzureIoT_Result aziotResult = AzureIoT_DeviceTwinReportState(serializedTelemetryUpload, NULL);
    Cloud_Result result = AzureIoTToCloudResult(aziotResult);

    json_free_serialized_string(serializedTelemetryUpload);
    json_value_free(uploadValue);

    return result;
}

Cloud_Result Cloud_SendDeviceDetails(const char *serialNumber)
{
    // Send static device twin properties when connection is established.
    JSON_Value *deviceDetailsValue = json_value_init_object();
    JSON_Object *deviceDetailsRoot = json_value_get_object(deviceDetailsValue);
    json_object_dotset_string(deviceDetailsRoot, "serialNumber", serialNumber);
    char *serializedDeviceDetails = json_serialize_to_string(deviceDetailsValue);
    AzureIoT_Result aziotResult = AzureIoT_DeviceTwinReportState(serializedDeviceDetails, NULL);
    Cloud_Result result = AzureIoTToCloudResult(aziotResult);

    json_free_serialized_string(serializedDeviceDetails);
    json_value_free(deviceDetailsValue);

    return result;
}

// static void DefaultTelemetryUploadEnabledChangedHandler(bool uploadEnabled)
// {
//     Log_Debug("WARNING: Cloud - no handler registered for TelemetryUploadEnabled - status %s\n",
//               uploadEnabled ? "true" : "false");
// }

static void DefaultDisplayAlertHandler(const char *alertMessage)
{
    Log_Debug("WARNING: Cloud - no handler registered for DisplayAlert - message %s\n",
              alertMessage);
}

static void DefaultConnectionChangedHandler(bool connected)
{
    Log_Debug("WARNING: Cloud - no handler registered for ConnectionChanged - status %s\n",
              connected ? "true" : "false");
}

static void ConnectionChangedCallbackHandler(bool connected)
{
    connectionChangedCallbackFunction(connected);
}

static void DeviceTwinCallbackHandler(const char *nullTerminatedJsonString)
{
    JSON_Value *rootProperties = NULL;
    rootProperties = json_parse_string(nullTerminatedJsonString);
    if (rootProperties == NULL) {
        Log_Debug("WARNING: Cannot parse the string as JSON content.\n");
        goto cleanup;
    }

    JSON_Object *rootObject = json_value_get_object(rootProperties);
    JSON_Object *desiredProperties = json_object_dotget_object(rootObject, "desired");
    JSON_Object *reportedProperties = json_object_dotget_object(rootObject, "reported");
    if (desiredProperties == NULL) {
        desiredProperties = rootObject;
    }
    JSON_Object *campaign = json_object_dotget_object(desiredProperties, "recipe_campaign");
    const char *currentUuid = json_object_dotget_string(reportedProperties, "recipe_campaign.uuid");
    
    if (campaign == NULL) {
        // this occurs when we delete a campaign:
        DeviceUpdateRequest request = { 0 };
        desiredRecipeChangedCallbackFunction(request);

        goto cleanup;
    }

    const char *uuid = json_object_dotget_string(campaign, "uuid");
    const char *filename = json_object_dotget_string(campaign, "filename");
    const char *url = json_object_dotget_string(campaign, "recipe_url");
    unsigned int size = (unsigned int)json_object_dotget_number(campaign, "size");

    unsigned int requestedVersion =
        (unsigned int)json_object_dotget_number(desiredProperties, "$version");
    DeviceUpdateRequest request = {
        .size = size
    };

    strcpy(request.filename, filename);
    strcpy(request.uuid, uuid);
    strcpy(request.url, url);
    
    if (requestedVersion > latestVersion) {
        latestVersion = requestedVersion;
    }
    
    if (currentUuid && (strcmp(uuid, currentUuid) == 0)) {
        Log_Debug("INFO: current campaign uuid is up to date, ignoring request...\n");
    } else {
        if (desiredRecipeChangedCallbackFunction != NULL) {
            desiredRecipeChangedCallbackFunction(request);
        }
    }


cleanup:
    // Release the allocated memory.
    json_value_free(rootProperties);
}

static int DeviceMethodCallbackHandler(const char *methodName, const unsigned char *payload,
                                       size_t payloadSize, unsigned char **response,
                                       size_t *responseSize)
{
    int result;
    char *responseString;
    static char nullTerminatedPayload[MAX_PAYLOAD_SIZE + 1];

    size_t actualPayloadSize = payloadSize > MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : payloadSize;

    strncpy(nullTerminatedPayload, payload, actualPayloadSize);
    nullTerminatedPayload[actualPayloadSize] = '\0';

    if (strcmp("displayAlert", methodName) == 0) {

        displayAlertCallbackFunction(nullTerminatedPayload);

        responseString =
            "\"Alert message displayed successfully.\""; // must be a JSON string (in quotes)
        result = 200;
    } else {
        // All other method names are ignored
        responseString = "{}";
        result = -1;
    }

    // if 'response' is non-NULL, the Azure IoT library frees it after use, so copy it to heap
    *responseSize = strlen(responseString);
    *response = malloc(*responseSize);
    memcpy(*response, responseString, *responseSize);

    return result;
}
