/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <init/applibs_versions.h>
#include <applibs/log.h>
#include <applibs/powermanagement.h>
#include <azureiot/azure_sphere_provisioning.h>
#include <applibs/networking.h>

#include <init/globals.h>
#include <init/adapter.h>
#include <iot/diag.h>
#include <iot/iot.h>
#include <iot/azure_iot_utilities.h>
#include <utils/llog.h>
#include <utils/timer.h>
#include <utils/utils.h>
#include <utils/network.h>
#include <utils/property.h>
#include <utils/event_loop_timer.h>
#include <frozen/frozen.h>

extern volatile bool g_app_running;

typedef struct iot_t iot_t;
struct iot_t {
    event_loop_timer_t *setup_timer;
    event_loop_timer_t *periodic_timer;
    event_loop_timer_t *reset_timer;
    struct timespec ts_last_online;
    struct timespec ts_last_offline;
    EventLoop *eloop;
};

static iot_t s_iot;


static event_code_t iot_connection_status2event(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    event_code_t code;

    switch (reason) {
    case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
        code = EVENT_IOT_EXPIRED_SAS_TOKEN;
        break;
    case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
        code = EVENT_IOT_CONNECTION_DEVICE_DISABLED;
        break;
    case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
        code = EVENT_IOT_CONNECTION_BAD_CREDENTIAL;
        break;
    case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
        code = EVENT_IOT_CONNECTION_RETRY_EXPIRED;
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
        code = EVENT_IOT_CONNECTION_NO_NETWORK;
        break;
    case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
        code = EVENT_IOT_CONNECTION_COMMUNICATION_ERROR;
        break;
    case IOTHUB_CLIENT_CONNECTION_OK:
        code = EVENT_IOT_CONNECTION_OK;
        break;
    default:
        code = EVENT_IOT_DISCONNECTED;
    }

    return code;
}


static void force_system_reboot(void *context)
{
    LOGI("System reboot ...");
    PowerManagement_ForceSystemReboot();
}


static void force_app_reset(void *context)
{
    LOGI("App reset...");
    g_app_running = false;
}

static void ota_system_reboot(void *context)
{
    LOGI("OTA System reboot ...");
    diag_log_event_to_file(EVENT_OTA);
    write_property("target_app_version", (char*)context);
    PowerManagement_ForceSystemReboot();
}

static void process_c2d_reset(void)
{
    // reboot after 10s to give app a chance to ack the C2D message
    LOGI("Force app reset in 10s");
    struct timespec ts = {10, 0};
    s_iot.reset_timer = event_loop_register_timer(s_iot.eloop, &ts, NULL, force_app_reset, NULL);
}

static void process_c2d_debug(const char *payload, size_t payload_size)
{
    int debug_level = 0;
    json_scanf(payload, payload_size, "{data:%d}", &debug_level);

    if (debug_level > 0) {
        llog_config(LOG_ENDPOINT_IOTHUB, debug_level);
        LOGI("Remote debug on");
    } else {
        llog_config(LOG_ENDPOINT_CONSOLE, LOG_LEVEL);
        LOGI("Remote debug off");
    }
}

static void process_c2d_reboot(void)
{
    LOGI("Force system reboot in 10s");
    struct timespec ts = {10, 0};
    s_iot.reset_timer = event_loop_register_timer(s_iot.eloop, &ts, NULL, force_system_reboot, NULL);
}

static void process_c2d_ota_reboot(const char *payload, size_t payload_size)
{
    char *target_app_version = NULL;
    json_scanf(payload, payload_size, "{target_app_version:%Q}", &target_app_version);

    if (target_app_version && strcmp(target_app_version, app_version()) != 0) {
        LOGI("Schedule App update: %s", target_app_version);
        struct timespec ts = { 1, 0 };
        s_iot.reset_timer = event_loop_register_timer(s_iot.eloop, &ts, NULL, ota_system_reboot, target_app_version);
    }
}


static void handle_c2d(const char *payload, size_t payload_size)
{
    char *command = NULL;
    json_scanf(payload, payload_size, "{command:%Q}", &command);

    if (!command) {
        LOGE("Invalid c2d message");
        return;
    }

    if (strcmp(command, IOT_COMMAND_RESET) == 0) {
        process_c2d_reset();
    } else if (strcmp(command, IOT_COMMAND_DUMPSYS) == 0) {
        // not supported for now
    } else if (strcmp(command, IOT_COMMAND_DEBUG) == 0) {
        process_c2d_debug(payload, payload_size);
    } else if (strcmp(command, IOT_COMMAND_REBOOT) == 0) {
        process_c2d_reboot();
    } else if (strcmp(command, IOT_COMMAND_OTA_REBOOT) == 0) {
        process_c2d_ota_reboot(payload, payload_size);
    } else {
        LOGE("Invalid C2D command:%s", command);
    }

    FREE(command);
}

static void message_received(const char *payload, size_t payload_size, const char *message_type, void *context)
{
    if (!payload || !payload_size || !message_type) {
        LOGW("empty C2D message received");
        return;
    }

    LOGI("Received %s message [size=%ld]", message_type, payload_size);

    if (strcmp(message_type, IOT_MESSAGE_TYPE_PROVISION) == 0) {
        adapter_provision(payload, payload_size, true);
    } else if (strcmp(message_type, IOT_MESSAGE_TYPE_CONTROL) == 0) {
        handle_c2d(payload, payload_size);
    } else {
        LOGW("Unsupport message type: %s", message_type);
    }
}


static void scan_desired_twin(const char *str, int len, void *user_data)
{
    char *target_app_version = NULL;
    json_scanf(str, len, "{target_app_version:%Q}", &target_app_version);

    if (target_app_version && (strcmp(app_version(), target_app_version) != 0)) {
        LOGI("Schedule APP update: %s", target_app_version);
        ota_system_reboot(target_app_version);
    }
}


static void scan_reported_twin(const char *str, int len, void *user_data)
{
    // do nothing
}

static void device_twin_update(const char *payload, size_t payload_size, void *context)
{
    LOGI("DEVICE_TWIN_UPDATE: %.*s", payload_size, payload);
    int result = json_scanf(payload, payload_size, "{desired:%M, reported:%M}",
                            scan_desired_twin, NULL,
                            scan_reported_twin, NULL);
    // device twin update contain desired and reported for the first time
    // following update will only contain desired section
    if (result <= 0) {
        scan_desired_twin(payload, payload_size, context);
    }
}

static void connection_status_changed(bool connected, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void *context)
{
    static bool last_connected = false;
    LOGI("iot hub %s", (connected ? "connected" : "disconnected"));

    if (!last_connected && connected) {
        clock_gettime(CLOCK_BOOTTIME, &s_iot.ts_last_online);
        diag_log_event(EVENT_IOT_CONNECTED);
    } else if (last_connected && !connected) {
        clock_gettime(CLOCK_BOOTTIME, &s_iot.ts_last_offline);
        diag_log_event(iot_connection_status2event(reason));
    }
    last_connected = connected;
}


static void log_setup_error_event(AZURE_SPHERE_PROV_RESULT result)
{
    switch (result) {
    case AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY:
        diag_log_event(EVENT_IOT_SETUP_FAILED_DEVICEAUTH);
        break;
    case AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY:
        diag_log_event(EVENT_IOT_SETUP_FAILED_NO_NETWORK);
        break;
    case AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR:
        diag_log_event(EVENT_IOT_SETUP_FAILED_DEVICE_ERROR);
        break;
    default:
        diag_log_event(EVENT_IOT_SETUP_FAILED);
    }
}


static void iot_setup_task(void *context)
{
    if (azure_iot_is_connected()) return;

    if (!network_is_connected()) {
        LOGI("iot setup client failed due to no network");
        return;
    }

    // we seen scenario that callback not been invoked even azure_iot_setup_client return ok
    AZURE_SPHERE_PROV_RESULT result = azure_iot_setup_client(IOT_MAX_INFLIGHT_MESSAGE_SIZE, NULL);

    if (result == AZURE_SPHERE_PROV_RESULT_OK) {
        LOGI("iot setup client ok");
    } else {
        log_setup_error_event(result);
    }
}


static void iot_periodic_task(void *context)
{
    azure_iot_do_periodic_tasks();
}

static int init_sdk(void)
{
    if (!azure_iot_initialize()) return -1;

    // Set the Azure IoT hub related callbacks
    azure_iot_set_message_received_callback(&message_received);
    azure_iot_set_device_twin_update_callback(&device_twin_update);
    azure_iot_set_connection_status_callback(&connection_status_changed);
    return 0;
}

static int schedule_setup_task(void)
{
    // can't use 0 as that means disarm timer
    struct timespec init_setup = MS2SPEC(30*1000);
    struct timespec ts_setup = MS2SPEC(IOT_SETUP_RETRY_MS);
    s_iot.setup_timer = event_loop_register_timer(s_iot.eloop, &init_setup, &ts_setup, iot_setup_task, NULL);
    return s_iot.setup_timer ? 0 : -1;
}

static int schedule_periodic_task(void)
{
    struct timespec init_periodic = MS2SPEC(1000);
    struct timespec ts_periodic = MS2SPEC(IOT_PERIODIC_TASK_MS);
    s_iot.periodic_timer = event_loop_register_timer(s_iot.eloop, &init_periodic, &ts_periodic, iot_periodic_task, NULL);
    return s_iot.periodic_timer ? 0 : -1;
}


// ------------------------------ public interface ----------------------------

int iot_init(EventLoop *eloop)
{
    LOGI("iot init");

    s_iot.eloop = eloop;

    if (init_sdk() != 0) {
        LOGE("Failed to initialize Azure IoT Hub SDK");
        return -1;
    }

    if (schedule_setup_task() != 0) {
        LOGE("Failed to schedule setup task");
        return -1;
    }

    if (schedule_periodic_task() != 0) {
        LOGE("Failed to schedule periodic task");
        return -1;
    }

    clock_gettime(CLOCK_BOOTTIME, &s_iot.ts_last_offline);
    return 0;
}


void iot_deinit()
{
    event_loop_unregister_timer(s_iot.eloop, s_iot.setup_timer);
    event_loop_unregister_timer(s_iot.eloop, s_iot.periodic_timer);
    if (s_iot.reset_timer) {
        event_loop_unregister_timer(s_iot.eloop, s_iot.reset_timer);
    }

    azure_iot_destroy_client();
    azure_iot_deinitialize();
}


int iot_send_message_async(const char *iot_message, const char *iot_message_type,
    message_delivery_confirmation_func_t callback, void *context)
{
    ASSERT(iot_message);
    ASSERT(iot_message_type);

    if (!network_is_connected()) {
        LOGW("Can't send message as network not connected");
        return -1;
    }

    if (!azure_iot_is_connected()) {
        LOGW("Can't send message as iot hub not connected");
        return -1;
    }

    return azure_iot_send_message_async(iot_message, iot_message_type, callback, context);
}



int iot_report_device_twin_async(const char *properties,
    device_twin_delivery_confirmation_func_t callback, void *context)
{
    ASSERT(properties);

    if (!network_is_connected()) {
        LOGW("Can't report device twin as network not connected");
        return -1;
    }

    if (!azure_iot_is_connected()) {
        LOGW("Can't report device twin as iot hub not connected");
        return -1;
    }

    return azure_iot_twin_report_async(properties, callback, context);
}


bool iot_is_connected(void)
{
    return azure_iot_is_connected();
}


struct timespec iot_last_online(void)
{
    return s_iot.ts_last_online;
}


struct timespec iot_last_offline(void)
{
    return s_iot.ts_last_offline;
}