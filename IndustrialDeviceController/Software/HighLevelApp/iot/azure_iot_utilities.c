/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <applibs/log.h>
#include <applibs/storage.h>
#include <azureiot/azure_sphere_provisioning.h>
#include <azureiot/iothub.h>
#include <azureiot/iothub_client_core_common.h>
#include <azureiot/iothub_client_options.h>
#include <azureiot/iothub_device_client_ll.h>
#include <azureiot/iothubtransportmqtt.h>

#include <init/globals.h>
#include <utils/llog.h>
#include <utils/utils.h>
#include <safeclib/safe_lib.h>
#include <iot/diag.h>
#include <iot/iot.h>
#include <iot/azure_iot_utilities.h>

// Refer to https://learn.microsoft.com/en-us/azure/iot-hub/iot-hub-device-sdk-c-intro for more
// information on Azure IoT SDK for C

typedef struct d2c_context_t d2c_context_t;
struct d2c_context_t {
    size_t payload_size;
    message_delivery_confirmation_func_t delivery_callback;
    void *context;
};

typedef struct twin_report_context_t twin_report_context_t;
struct twin_report_context_t {
    size_t payload_size;
    device_twin_delivery_confirmation_func_t delivery_callback;
    void *context;
};

// String containing the scope id of the Device Provisioning Service
// used to provision the app with the IoT hub hostname and the device id.
static const char scopeId[] = "xxxxxxxxxxx";

static device_twin_update_func_t device_twin_update_cb = 0;

static connection_status_func_t connection_status_cb = 0;

static message_received_func_t message_received_cb = 0;

static IOTHUB_DEVICE_CLIENT_LL_HANDLE iothub_client_handle = NULL;

static bool iothub_authenticated = false;

static int keepalive_period_seconds = 240;

static uint32_t inflight_message_quota = 0;

static uint32_t inflight_message_size = 0;


static const char *get_reason_string(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    static char *reason_string = "unknown reason";
    switch (reason) {
    case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
        reason_string = "IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN";
        break;
    case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
        reason_string = "IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED";
        break;
    case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
        reason_string = "IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL";
        break;
    case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
        reason_string = "IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED";
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
        reason_string = "IOTHUB_CLIENT_CONNECTION_NO_NETWORK";
        break;
    case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
        reason_string = "IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR";
        break;
    case IOTHUB_CLIENT_CONNECTION_OK:
        reason_string = "IOTHUB_CLIENT_CONNECTION_OK";
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE:
        reason_string = "IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE";
        break;
    }
    return reason_string;
}

static char *get_azure_sphere_provisioning_result_string(AZURE_SPHERE_PROV_RETURN_VALUE provisioning_result)
{
    switch (provisioning_result.result) {
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


static void send_reported_state_callback(int result, void *context)
{
    twin_report_context_t *ctx = (twin_report_context_t*)context;
    inflight_message_size -= ctx->payload_size;

    if (inflight_message_size < 0) {
        inflight_message_size = 0;
    }

    if (ctx->delivery_callback) {
        ctx->delivery_callback(result == 204, ctx->context);
    }

    FREE(ctx);
}


static void send_message_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *context)
{
    d2c_context_t *ctx = (d2c_context_t *)context;
    inflight_message_size -= ctx->payload_size;

    if (inflight_message_size < 0) {
        inflight_message_size = 0;
    }

    if (ctx->delivery_callback) {
        ctx->delivery_callback(result == IOTHUB_CLIENT_CONFIRMATION_OK, ctx->context);
    }

    FREE(ctx);
}


static IOTHUBMESSAGE_DISPOSITION_RESULT message_received_callback(IOTHUB_MESSAGE_HANDLE message, void *context)
{
    const unsigned char *buffer = NULL;
    size_t size = 0;

    if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK) {
        LOGW("failure performing IoTHubMessage_GetByteArray");
        return IOTHUBMESSAGE_REJECTED;
    }

    const char *message_type = IoTHubMessage_GetProperty(message, "message_type");

    if (message_received_cb) {
        message_received_cb(buffer, size, message_type, context);
    }

    return IOTHUBMESSAGE_ACCEPTED;
}


static void device_twin_update_callback(DEVICE_TWIN_UPDATE_STATE update_state,
           const unsigned char *properties, size_t properties_len, void *context)
{
    if (device_twin_update_cb) {
        device_twin_update_cb(properties, properties_len, context);
    }
}


static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result,
                                        IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void *context)
{
    iothub_authenticated = (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED);

    if (connection_status_cb) {
        connection_status_cb(iothub_authenticated, reason, context);
    }

    const char *reason_string = get_reason_string(reason);
    if (!iothub_authenticated) {
        LOGI("IoT Hub connection is down (%s), retrying connection...", reason_string);
    } else {
        LOGI("connection to the IoT Hub has been established (%s).", reason_string);
    }
}

// ---------------------------- public interface ------------------------------

bool azure_iot_initialize(void)
{
    if (IoTHub_Init() != 0) {
        LOGE("failed initializing platform.");
        return false;
    }
    return true;
}


void azure_iot_deinitialize(void)
{
    IoTHub_Deinit();
}

void azure_iot_destroy_client(void)
{
    if (iothub_client_handle != NULL) {
        IoTHubDeviceClient_LL_Destroy(iothub_client_handle);
        iothub_client_handle = NULL;
        iothub_authenticated = false;
    }
}

AZURE_SPHERE_PROV_RESULT azure_iot_setup_client(size_t quota, void *context)
{
    LOGI("Connecting to IoTHub...");

    azure_iot_destroy_client();

    AZURE_SPHERE_PROV_RETURN_VALUE prov_result =
        IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning(scopeId, 30000, &iothub_client_handle);

    LOGI("IoTHubDeviceClient_CreateWithAzureSphereDeviceAuthProvisioning returned '%s'.",
         get_azure_sphere_provisioning_result_string(prov_result));

    if (prov_result.result != AZURE_SPHERE_PROV_RESULT_OK) {
        return prov_result.result;
    }

    if (iothub_client_handle == NULL) {
        LOGE("failed to create handler");
        return AZURE_SPHERE_PROV_RESULT_IOTHUB_CLIENT_ERROR;
    }

    if (IoTHubDeviceClient_LL_SetRetryPolicy(iothub_client_handle, IOTHUB_CLIENT_RETRY_NONE, 0) != IOTHUB_CLIENT_OK) {
        LOGE("failed to set retry policy");
        azure_iot_destroy_client();
        return AZURE_SPHERE_PROV_RESULT_IOTHUB_CLIENT_ERROR;
    }

    if (IoTHubDeviceClient_LL_SetOption(iothub_client_handle, OPTION_KEEP_ALIVE, &keepalive_period_seconds) !=
        IOTHUB_CLIENT_OK) {
        LOGE("failed to set keep alive \"%s\"", OPTION_KEEP_ALIVE);
        azure_iot_destroy_client();
        return AZURE_SPHERE_PROV_RESULT_IOTHUB_CLIENT_ERROR;
    }


    if (IoTHubDeviceClient_LL_SetMessageCallback(iothub_client_handle, message_received_callback, context) !=
        IOTHUB_CLIENT_OK) {
        LOGE("failed to set receive message callback");
        azure_iot_destroy_client();
        return AZURE_SPHERE_PROV_RESULT_IOTHUB_CLIENT_ERROR;
    }

    if (IoTHubDeviceClient_LL_SetDeviceTwinCallback(iothub_client_handle, device_twin_update_callback, context) !=
        IOTHUB_CLIENT_OK) {
        LOGE("failed to set device twin callback");
        azure_iot_destroy_client();
        return AZURE_SPHERE_PROV_RESULT_IOTHUB_CLIENT_ERROR;
    }

    if (IoTHubDeviceClient_LL_SetConnectionStatusCallback(iothub_client_handle, connection_status_callback, context) !=
        IOTHUB_CLIENT_OK) {
        LOGE("failed to set connection status callback");
        azure_iot_destroy_client();
        return AZURE_SPHERE_PROV_RESULT_IOTHUB_CLIENT_ERROR;
    }

    inflight_message_size = 0;
    inflight_message_quota = quota;
    return AZURE_SPHERE_PROV_RESULT_OK;
}


void azure_iot_do_periodic_tasks(void)
{
    if (iothub_client_handle) {
        IoTHubDeviceClient_LL_DoWork(iothub_client_handle);
    }
}


void azure_iot_set_message_received_callback(message_received_func_t callback)
{
    message_received_cb = callback;
}


void azure_iot_set_device_twin_update_callback(device_twin_update_func_t callback)
{
    device_twin_update_cb = callback;
}


void azure_iot_set_connection_status_callback(connection_status_func_t callback)
{
    connection_status_cb = callback;
}


int azure_iot_send_message_async(const char *message, const char *message_type,
    message_delivery_confirmation_func_t callback, void *context)
{
    size_t message_len = strlen(message);

    if (inflight_message_quota && (inflight_message_size + message_len > inflight_message_quota)) {
        LOGE("Exceed inflight message quota");
        return -1;
    }

    IOTHUB_MESSAGE_HANDLE message_handle = IoTHubMessage_CreateFromString(message);

    if (message_handle == 0) {
        LOGE("unable to create a new IoTHubMessage");
        return -1;
    }

    // Set the system property of the message
    IoTHubMessage_SetContentTypeSystemProperty(message_handle, IOT_MESSAGE_CONTENT_TYPE);
    IoTHubMessage_SetContentEncodingSystemProperty(message_handle, IOT_MESSAGE_CONTENT_ENCODING);

    // Set the application property of the message
    IoTHubMessage_SetProperty(message_handle, "message_type", message_type);

    d2c_context_t *ctx = CALLOC(1, sizeof(d2c_context_t));
    ctx->payload_size = message_len;
    ctx->delivery_callback = callback;
    ctx->context = context;

    if (IoTHubDeviceClient_LL_SendEventAsync(iothub_client_handle, message_handle, send_message_callback,
                                             (void *)ctx) != IOTHUB_CLIENT_OK) {
        LOGE("failed to hand over the message to IoTHubClient");
        FREE(ctx);
        IoTHubMessage_Destroy(message_handle);
        return -1;
    }

    inflight_message_size += message_len;
    IoTHubMessage_Destroy(message_handle);
    return 0;
}


int azure_iot_twin_report_async(const char *properties, device_twin_delivery_confirmation_func_t callback, void *context)
{
    size_t properties_len = strlen(properties);

    if (inflight_message_quota && (inflight_message_size + properties_len > inflight_message_quota)) {
        LOGE("Exceed inflight message quota");
        return -1;
    }

    twin_report_context_t *ctx = CALLOC(1, sizeof(twin_report_context_t));
    ctx->payload_size = properties_len;
    ctx->delivery_callback = callback;
    ctx->context = context;

    if (IoTHubDeviceClient_LL_SendReportedState(iothub_client_handle, (unsigned char *)properties, properties_len,
                                                send_reported_state_callback, ctx) != IOTHUB_CLIENT_OK) {
        LOGE("failed to set reported property: %s", properties);
        FREE(ctx);
        return -1;
    }

    inflight_message_size += properties_len;
    return 0;
}


bool azure_iot_is_connected(void)
{
    return iothub_client_handle && iothub_authenticated;
}
