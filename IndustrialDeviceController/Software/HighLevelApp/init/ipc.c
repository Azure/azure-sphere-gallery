/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>

#include <init/ipc.h>
#include <utils/llog.h>
#include <utils/memory.h>
#include <safeclib/safe_lib.h>

err_code ipc_execute_command(int socket_fd, ipc_command_type_t command, uint8_t* data, int32_t len)
{
    static uint32_t msg_seq_num = 1;
    
    int msg_length = sizeof(ipc_request_message_t) + len;
    uint8_t* msg = (uint8_t*)MALLOC(msg_length);
    serialize_uint32(msg, command);
    serialize_uint32(msg + 4, msg_seq_num++);
    serialize_uint32(msg + 8, len);
    if (len > 0) {
        memcpy_s(msg + 12, len, data, len);
    }

    LOGE("Send command to M4: %d command type %d", msg_seq_num - 1, command);
    int bytes_sent = send(socket_fd, msg, msg_length, 0);
    if (bytes_sent == -1) {
        LOGE("ERROR: Unable to send message to M4: %d (%s)", errno, strerror(errno));
        FREE(msg);
        return DEVICE_E_IO;
    }

    ipc_response_message_t resp_msg;
    int bytes_received = recv(socket_fd, &resp_msg, sizeof(ipc_response_message_t), 0);
    if (bytes_received != -1) {
        uint8_t* resp_bytes = (uint8_t*)&resp_msg;
        resp_msg.command = dserialize_uint32(resp_bytes);
        resp_msg.seq_num = dserialize_uint32(resp_bytes + 4);
        resp_msg.code = dserialize_uint32(resp_bytes + 8);
        LOGE("Receive message from M4: %d", resp_msg.seq_num);

        if (msg_seq_num - 1 != resp_msg.seq_num) {
            LOGE("ERROR: Sequence number does not match, exepct %d but get %d from M4", msg_seq_num - 1, resp_msg.seq_num);
            resp_msg.code = DEVICE_E_INTERNAL;
        }
    } else {
        LOGE("ERROR: Unable to receive message from M4: %d (%s)", errno, strerror(errno));
        resp_msg.code = DEVICE_E_IO;
    }

    FREE(msg);
    return resp_msg.code;
}

uint8_t* serialize_uint32(uint8_t* data, uint32_t value)
{
    data[0] = value;
    data[1] = value >> 8;
    data[2] = value >> 16;
    data[3] = value >> 24;
    return data + 4;
}

uint32_t dserialize_uint32(uint8_t* data)
{
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}