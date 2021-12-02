/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

/**
 * create TCP transportion layer for modbus protocol
 * @param conn_str "ip:port", null terminated
 * @return tcp instance been created, the instance will implmente interface been defined in modbus_transport_t
 */
modbus_transport_t *modbus_transport_tcp_create(const char *conn_str);

/**
 * destroy tcp instance
 * @param instance tcp instance to be destroyed
 */
void modbus_transport_tcp_destroy(modbus_transport_t *instance);