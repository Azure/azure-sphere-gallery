/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <init/globals.h>
#include <init/device_hal.h>

#include "modbus_transport.h"
#include "modbus_transport_rtu.h"
#include "modbus_transport_tcp.h"


/// <summary>
/// factory method to create transportion layer for modbus
/// </summary>
/// <param name="connection">json value for connection information, format will depend on
/// transportation been used.</param>
/// <param name="ptransport>pointer to hold the transportation instance been created, no
/// change if failed.</param>
modbus_transport_t *modbus_create_transport(device_protocol_t protocol, const char *conn_str)
{
    ASSERT(conn_str);

    switch (protocol) {
    case DEVICE_PROTOCOL_MODBUS_TCP:
        return modbus_transport_tcp_create(conn_str);

    case DEVICE_PROTOCOL_MODBUS_RTU:
        return modbus_transport_rtu_create(conn_str);

    default:
        return NULL;
    }
}

void modbus_destroy_transport(device_protocol_t protocol, modbus_transport_t *transport)
{
    ASSERT(transport);

    switch (protocol) {
    case DEVICE_PROTOCOL_MODBUS_RTU:
        modbus_transport_rtu_destroy(transport);
        break;

    case DEVICE_PROTOCOL_MODBUS_TCP:
        modbus_transport_tcp_destroy(transport);
        break;

    default:
        break;
    }
}
