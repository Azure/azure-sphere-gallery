/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#include <applibs/eventloop.h>
#include <init/device_hal.h>

enum {
   DRIVER_STATE_INIT,
   DRIVER_STATE_OPENED,
   DRIVER_STATE_PARTIAL,
   DRIVER_STATE_NORMAL
};

/**
 * initialize adapter module
 * adapter module schedule query to each device according to provision and send telemetry message
 * @param eloop event loop to be used to schedule task in adapter module
 * @return 0 if succeed or negative value otherwise
 */
int adapter_init(EventLoop *eloop);

/**
 * deinitialize adapter module
 */
void adapter_deinit(void);

/**
 * parse and apply given provision
 * @param payload - provision json string
 * @param payload_size - len of provision
 * @param flush - true if need to synchronize with storage device
 */
void adapter_provision(const char *payload, size_t payload_size, bool flush);

/**
 * @return string name of adapter
 */
const char *adapter_get_name(void);

/**
 * @return string install location of adapter
 */
const char *adapter_get_location(void);

/**
 * @return source id of adapter which identify which profile
 * this adapter belongs to
 */
const char *adapter_get_source_id(void);

/**
 * @return device list this adapter connected to
 */
ce_device_t *adapter_get_devices(void);

/**
 * @return boot timestamp when this adapter been last provisioned
 */
struct timespec adapter_last_provisioned(void);

/**
 * @return current adapter state, refer to above DEVICE_STATE enum
 */
uint32_t adapter_get_driver_state(void);
