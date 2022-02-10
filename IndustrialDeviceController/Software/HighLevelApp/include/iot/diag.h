/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#include <applibs/eventloop.h>

typedef enum event_code_t event_code_t;
enum event_code_t {
    EVENT_RESTART = 0,
    EVENT_PROVISION,
    EVENT_WATCHDOG,
    EVENT_WATCHDOG_WARNING,
    EVENT_SYSTEM_REBOOT,
    EVENT_PROVISION_FAILED,
    EVENT_EVENT_MISSING,
    EVENT_RECOVER_REBOOT,
    EVENT_SAFE_MODE,
    EVENT_OTA,

    EVENT_CE_OPEN_FAILED = 100,
    EVENT_CE_POLL_FAILED,
    EVENT_CE_CONNECTED,
    EVENT_CE_DISCONNECTED,
    EVENT_CE_DISCONNECTED_BROKEN,

    EVENT_NETWORK_NO_INTERFACE = 150,
    EVENT_NETWORK_INTERFACE_UP,
    EVENT_NETWORK_LOCAL,
    EVENT_NETWORK_IP_AVAILABLE,
    EVENT_NETWORK_INTERNET,

    EVENT_TELEMETRY_FAILED = 200,
    EVENT_IOT_CONNECTED,
    EVENT_IOT_DISCONNECTED,
    EVENT_IOT_DISCONNECTED_NO_NETWORK,
    EVENT_IOT_SETUP_FAILED,
    EVENT_IOT_SETUP_FAILED_DEVICEAUTH,
    EVENT_IOT_SETUP_FAILED_NO_NETWORK,
    EVENT_IOT_SETUP_FAILED_DEVICE_ERROR,
    EVENT_IOT_CONNECTING,

    EVENT_IOT_EXPIRED_SAS_TOKEN = 250,
    EVENT_IOT_CONNECTION_DEVICE_DISABLED,
    EVENT_IOT_CONNECTION_BAD_CREDENTIAL,
    EVENT_IOT_CONNECTION_RETRY_EXPIRED,
    EVENT_IOT_CONNECTION_NO_NETWORK,
    EVENT_IOT_CONNECTION_COMMUNICATION_ERROR,
    EVENT_IOT_CONNECTION_OK,

    EVENT_SIGTERM = 315,

    // Use EVENT_NEXT_AVAILABLE_SIGNAL for new diag event code
    EVENT_NEXT_AVAILABLE_CODE = 400,
};


/**
 * initialized diagnostic module
 * @param eloop event loop to be used to schedule tasks in this module
 */
int diag_init(EventLoop *eloop);

/**
 * deinitialized diagnostic module
 */
void diag_deinit(void);

/**
 * log a diagnostic key/value pair which will be uploaded periodically
 * @param key key
 * @param value value
 */
void diag_log_value(const char *key, double value);

/**
 * retrive a logged diagnostic value
 * @param key key to be retrived
 * @return value retrived or NAN if key not exist
 */
double diag_get_value(const char *key);

/**
 * remove a logged diagnostic value
 * @param key key to be removed
 */
void diag_remove_value(const char* key);

/**
 * log a dianostic count value which will be uploaded periodically
 * @param key counter name
 * @return current count value
 */
int diag_log_count_value(const char *key);

/**
 * retrive a counter value
 * @param key counter name
 * @return count value or 0 if counter not exist
 */
int diag_get_count_value(const char *key);

/**
 * log a predefined diagnostic event, event is those major thing happened on adapter which enable
 * user to understand what happend on adapter over a long period of time by looking at event history
 * event will be uploaded periodically.
 * @param code event code
 */
void diag_log_event(event_code_t code);

/**
 * log a predefine diagnostic event and persist all current event in memory to file storage. This is useful when
 * adapter is going to reboot or app crashed to avoid lost of event. The event persisted will be loaded into memory
 * next time app start.
 * @param code - event code to be logged
 */
void diag_log_event_to_file(event_code_t code);
