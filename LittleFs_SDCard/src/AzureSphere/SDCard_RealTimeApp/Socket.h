/* Copyright (c) Codethink Ltd. All rights reserved.
   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef AZURE_SPHERE_SOCKET_H_
#define AZURE_SPHERE_SOCKET_H_

#include "lib/Common.h"
#include "lib/Platform.h"

#include <stdbool.h>
#include <stdint.h>

// This interface is for communicating over a "socket" with a partner core.
// It supports connection with linux socket interface on the A7, which
// negotiates the connection by calling Application_Connect(Component_Id).
// Implementation depends on MBox.h.
// It is derivative of logical-intercore.h in
// https://github.com/Azure/azure-sphere-samples/tree/master/Samples/IntercoreComms


#ifdef __cplusplus
extern "C" {
#endif

/// Returned when there's a space issue.</summary>
#define ERROR_SOCKET_INSUFFICIENT_SPACE (ERROR_SPECIFIC - 1)

/// Returned when negotiation fails.</summary>
#define ERROR_SOCKET_NEGOTIATION        (ERROR_SPECIFIC - 2)

typedef struct Socket Socket;

/// When sending a message, this is the recipient HLApp's component ID.
/// When receiving a message, this is the sender HLApp's component ID.
typedef struct {
    /// 4-byte little-endian word
    uint32_t seg_0;
    /// 2-byte little-endian half
    uint16_t seg_1;
    /// 2-byte little-endian half
    uint16_t seg_2;
    /// 2-byte big-endian & 6-byte big-endian
    uint8_t  seg_3_4[8];
} Component_Id;

Socket* Socket_Open(void (*rx_cb)(Socket*));
int32_t Socket_Close(Socket *socket);

bool    Socket_NegotiationPending(Socket *socket);
int32_t Socket_Negotiate(Socket *socket);

void Socket_Reset(Socket *socket);

int32_t Socket_Write(
    Socket             *socket,
    const Component_Id *recipient,
    const void         *data,
    uint32_t            size);
int32_t Socket_Read(
    Socket       *socket,
    Component_Id *sender,
    void         *data,
    uint32_t     *size);

#ifdef __cplusplus
}
#endif

#endif // #ifndef AZURE_SPHERE_SOCKET_H_
