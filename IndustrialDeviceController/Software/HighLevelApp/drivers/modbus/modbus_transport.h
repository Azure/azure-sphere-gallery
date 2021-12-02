/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdint.h>
#include <init/device_hal.h>

typedef struct modbus_transport_t modbus_transport_t;
struct modbus_transport_t {
    // open the transport channel, return error code
    err_code (*transport_open)(modbus_transport_t *instance, int32_t timeout_ms);

    // close transport channel, return error code
    err_code (*transport_close)(modbus_transport_t *instance);

    // send pdu, return error code
    err_code (*send_request)(modbus_transport_t *instance, uint8_t id, const uint8_t *pdu, int32_t pdu_len,
                             int32_t timeout);

    // recv pdu for pervious request, return error code
    err_code (*recv_response)(modbus_transport_t *instance, uint8_t id, uint8_t *pdu, int32_t *ppdu_len,
                              int32_t timeout);
};


/**
 * factory method to create modbus transportation layer
 * @param protocol modbus rtu or tcp
 * @param conn_str protocol specific connection string, uart config for rtu, "ip:port" for tcp
 * @return transportation instance been created
 */
modbus_transport_t *modbus_create_transport(device_protocol_t protocol, const char *conn_str);

/**
 * factory method to destroy transportation layer instance
 * @param protocol modbus rtu or tcp
 * @param transport instance to be destroyed
 */
void modbus_destroy_transport(device_protocol_t protocol, modbus_transport_t *transport);