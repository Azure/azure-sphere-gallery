/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
  
#pragma once

#include <stdbool.h>
#include <applibs/eventloop.h>
#include <iot/azure_iot_utilities.h>

#define IOT_MESSAGE_TYPE_TELEMETRY "telemetry"
#define IOT_MESSAGE_TYPE_OSUPGRADE "os_upgrade"

#define IOT_MESSAGE_TYPE_DIAG_EVENTS "diag_events"
#define IOT_MESSAGE_TYPE_DIAG_DEBUG "diag_log"
#define IOT_MESSAGE_TYPE_DIAG_TELEMETRY "diag_telemetry"

#define IOT_MESSAGE_TYPE_CONTROL "control"
#define IOT_MESSAGE_TYPE_PROVISION "provision"

#define IOT_COMMAND_RESET "reset"
#define IOT_COMMAND_DEBUG "debug"
#define IOT_COMMAND_DUMPSYS "dumpsys"
#define IOT_COMMAND_PROVISION "provision"
#define IOT_COMMAND_REBOOT "reboot"
#define IOT_COMMAND_OTA_REBOOT "ota_reboot"

#define IOT_MESSAGE_CONTENT_TYPE "application%2fjson"
#define IOT_MESSAGE_CONTENT_ENCODING "utf-8"

/**
 * initialize iot module
 * 
 * iot module is be responsible for setup connection with iot hub, handle communication with iot hub
 * @param eloop event loop instance to schedule tasks in iot module
 */
int iot_init(EventLoop *eloop);

/**
 * deinitialize iot module
 */
void iot_deinit(void);

/**
 * send d2c message to iot hub
 * @param iot_message message to be sent
 * @param iot_message_type message type to be sent
 * @param callback callback function to indicate message deliver result
 * @param context context for callback function
 * @return 0 if message been successfully enqueue in SDK layer, not necessarialy mean message been deliver, actually deliver result
 * will be indicated in callback. negative if send attempt failed, e.g. no network, no connection to iot hub, exceed inflight quota etc.
 */
int iot_send_message_async(const char* iot_message, const char* iot_message_type,
                           message_delivery_confirmation_func_t callback, void *context);


/**
 * report device twin to iot hub
 * @param properties properties to be updated
 * @param callback callback function to indicate device twin update result
 * @param context context for callback function
 * @return 0 if update request been successfully enqueue in SDK layer, not necessarialy mean twin been updated, actually result
 * will be indicated in callback. negative if update attempt failed, e.g. no network, no connection to iot hub, exceed inflight quota etc.
 */                        
int iot_report_device_twin_async(const char* properties, device_twin_delivery_confirmation_func_t callback, void *context);

/**
 * check if adapter is connected to iot hub
 * @return whether adapter connected to iot hub
 */
bool iot_is_connected(void);

/**
 * get timestamp when adapter last online
 * @return boot timespec adapter last online
 */
struct timespec iot_last_online(void);

/**
 * get timestamp when adapter last offline
 * @return boot timespec adapter last offline
 */
struct timespec iot_last_offline(void);

