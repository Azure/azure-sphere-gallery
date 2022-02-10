/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
  
#pragma once

#define MB_RTU_MAX_ADU_SIZE 256
#define MB_RTU_HEADER_SIZE 3

/**
 * create RTU transportion layer for modbus protocol
 * @param conn_str uart config for transportation layer
 * @return rtu instance been created, the instance will implmente interface been defined in modbus_transport_t
 */
modbus_transport_t* modbus_transport_rtu_create(const char *conn_str);

/**
 * destroy rtu instance
 * @param instance rtu instance to be destroyed
 */
void modbus_transport_rtu_destroy(modbus_transport_t* instance);