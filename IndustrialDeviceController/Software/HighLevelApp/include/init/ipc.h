/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "device_hal.h"

typedef enum ipc_command_type_t {
    IPC_OPEN_UART,
    IPC_CLOSE_UART,
    IPC_WRITE_UART
} ipc_command_type_t;

typedef struct ipc_request_message_t {
    ipc_command_type_t command;
    uint32_t seq_num;
    uint32_t length;
    uint8_t data[0];
} ipc_request_message_t;

typedef struct ipc_response_message_t {
    ipc_command_type_t command;
    uint32_t seq_num;
    err_code code;
} ipc_response_message_t;

/**
 * Execute command on the real-time core
 * @param socket_fd the socket file handle
 * @param command the command to be executed
 * @param data the data buffer to be sent 
 * @param len the length of the buffer 
 * @return protocol enum value
 */
err_code ipc_execute_command(int socket_fd, ipc_command_type_t command, uint8_t* data, int32_t len);

/**
 * Serialize uint32 into a byte array.
 * @param data the byte array
 * @param value the uint32 value
 * @return the next available position in the array
 */
uint8_t* serialize_uint32(uint8_t* data, uint32_t value);

/**
 * Deserialize 4 bytes to a uint32 value.
 * @param data the byte array
 * @return the uint32 value
 */
uint32_t dserialize_uint32(uint8_t* data);