/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef AZURE_SPHERE_IPC_H_
#define AZURE_SPHERE_IPC_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum err_code {
    DEVICE_OK,
    DEVICE_E_INVALID,  // invalid operation
    DEVICE_E_IO,       // IO error
    DEVICE_E_BROKEN,   // connection broken
    DEVICE_E_PROTOCOL, // protocol error, bad pdu, out of sync
    DEVICE_E_TIMEOUT,  // timeout
    DEVICE_E_INTERNAL, // internal logic error, ASSERT
    DEVICE_E_CONFIG,   // device configuration error
    DEVICE_E_BUSY,     // Garbage data on link
    DEVICE_E_NO_DATA,  // data not avaiable
    DEVICE_E_LAST
} err_code;

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

#endif // #ifndef AZURE_SPHERE_IPC_H_