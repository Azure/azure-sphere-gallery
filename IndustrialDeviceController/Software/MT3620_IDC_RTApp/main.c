/* Copyright (c) Microsoft Corporation. All rights reserved.
   Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for the real-time core demonstrates intercore communications by
// sending a message to a high-level application every second, and printing out any received
// messages.
//
// It demontrates the following hardware
// - UART (used to write a message via the built-in UART)
// - mailbox (used to report buffer sizes and send / receive events)
// - timer (used to send a message to the HLApp)

/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "lib/mt3620/gpt.h"
#include "lib/CPUFreq.h"
#include "lib/VectorTable.h"
#include "lib/NVIC.h"
#include "lib/GPIO.h"
#include "lib/UART.h"
#include "lib/Print.h"
#include "lib/GPT.h"

#include "Socket.h"
#include "ipc.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) > (b) ? (b) : (a))

#define MB_RTU_MAX_ADU_SIZE 256

// Application ID on A7
static const Component_Id A7ID =
{
    .seg_0 = 0x77c1568c,
    .seg_1 = 0xbae1,
    .seg_2 = 0x470d,
    .seg_3_4 = {0xab, 0xe7, 0xeb, 0x3f, 0xef, 0x9b, 0x6b, 0x00}
};

// Drivers
static UART *debug = NULL;
static UART *modbus = NULL;

static Socket *socket = NULL;
static uint8_t msg[sizeof(ipc_request_message_t) + MB_RTU_MAX_ADU_SIZE];

static uint8_t modbus_frame[MB_RTU_MAX_ADU_SIZE];
static uint16_t size = 0;

// Callbacks
typedef struct CallbackNode {
    bool enqueued;
    struct CallbackNode *next;
    void *data;
    void (*cb)(void*);
} CallbackNode;

static void EnqueueCallback(CallbackNode *node);
static void HandleUartIsu0RxIrq(void);

// Msg callbacks
// Prints an array of bytes
static void printBytes(const uint8_t *bytes, uintptr_t start, uintptr_t size)
{
    for (unsigned i = start; i < size; i++) {
        UART_Printf(debug, "%x", bytes[i]);
    }
}

static void printComponentId(const Component_Id *compId)
{
    UART_Printf(debug, "%lx-%x-%x", compId->seg_0, compId->seg_1, compId->seg_2);
    UART_Print(debug, "-");
    printBytes(compId->seg_3_4, 0, 2);
    UART_Print(debug, "-");
    printBytes(compId->seg_3_4, 2, 8);
    UART_Print(debug, "\r\n");
}

// Serialize uint32 into a byte array
static uint8_t* serialize_uint32(uint8_t* data, uint32_t value)
{
    data[0] = value;
    data[1] = value >> 8;
    data[2] = value >> 16;
    data[3] = value >> 24;
    return data + 4;
}

// Deserialize 4 bytes to a uint32 value.
static uint32_t dserialize_uint32(uint8_t* data)
{
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

// Send response message back to the application on A7.
static void ipcSendResponseMsg(ipc_command_type_t command, uint32_t seq_num, err_code code)
{
    serialize_uint32(msg, command);
    serialize_uint32(msg + 4, seq_num);
    serialize_uint32(msg + 8, code);

    int32_t error = Socket_Write(socket, &A7ID, msg, sizeof(ipc_response_message_t));
    if (error != ERROR_NONE) {
        UART_Printf(debug, "ERROR: sending code %d for command %ld with seq_num %d- %ld\r\n", command, seq_num, code, error);
    }
}

static void handleRecvMsg(void *handle)
{
    Socket *socket = (Socket*)handle;

    Component_Id senderId;
    ipc_request_message_t request;
    uint32_t msg_size = sizeof(msg);

    if (Socket_NegotiationPending(socket)) {
        UART_Printf(debug, "Negotiation pending, attempting renegotiation\n");
        // NB: this is blocking, if you want to protect against hanging,
        //     add a timeout
        if (Socket_Negotiate(socket) != ERROR_NONE) {
            UART_Printf(debug, "ERROR: renegotiating socket connection\n");
        }
    }

    int32_t error = Socket_Read(socket, &senderId, &msg, &msg_size);
    if (error != ERROR_NONE) {
        UART_Printf(debug, "ERROR: receiving msg %s - %ld\r\n", msg, error);
        return;
    }

    request.command = dserialize_uint32(msg);
    request.seq_num = dserialize_uint32(msg + 4);
    request.length = dserialize_uint32(msg + 8);

    UART_Printf(debug, "Message received: command %d seq_num %ld length %ld\r\nSender: ",
        request.command, request.seq_num, request.length);
    printComponentId(&senderId);

    int32_t result;
    uint32_t baudRate;
    uint8_t parity, stopBits;
    switch (request.command) {
        case IPC_OPEN_UART:
            baudRate = dserialize_uint32(msg + 12);
            parity = msg[16];
            stopBits = msg[17];
            if (modbus != NULL) {
                UART_Close(modbus);
                modbus = NULL;
            }

            modbus = UART_Open(MT3620_UNIT_ISU0, baudRate, parity, stopBits, HandleUartIsu0RxIrq);
            ipcSendResponseMsg(IPC_OPEN_UART, request.seq_num,
                modbus != NULL ? DEVICE_OK : DEVICE_E_IO);
            break;

        case IPC_CLOSE_UART:
            if (modbus != NULL) {
                UART_Close(modbus);
                modbus = NULL;
            }
            ipcSendResponseMsg(IPC_CLOSE_UART, request.seq_num, DEVICE_OK);
            break;

        case IPC_WRITE_UART:
            result = UART_Write(modbus, msg + 12, request.length);
            if (ERROR_NONE == result)
            {
                // Wait for the UART's hardware TX buffer to empty
                uint32_t retries = 0xFFFF;
                while (retries && !UART_IsWriteComplete(modbus)) retries--;

                if (retries == 0)
                {
                    result = ERROR_TIMEOUT;
                }
                else
                {
                    // This is fine-tuned with a scope to achieve a minimal delay,
                    // so to fully include the STOP bit after the TX of the last byte
                    for (int i = 300; i > 0; i--) __asm__("nop");
                }
                ipcSendResponseMsg(IPC_WRITE_UART, request.seq_num,
                    result == ERROR_NONE ? DEVICE_OK : DEVICE_E_TIMEOUT);
            } else {
                ipcSendResponseMsg(IPC_WRITE_UART, request.seq_num, DEVICE_E_IO);
            }
            break;

        default:
            UART_Printf(debug, "ERROR: receiving not supported command %d", request.command);
    }
}

static void handleRecvMsgWrapper(Socket *handle)
{
    static CallbackNode cbn = {.enqueued = false, .cb = handleRecvMsg, .data = NULL};

    if (!cbn.data) {
        cbn.data = handle;
    }

    EnqueueCallback(&cbn);
}

static void HandleUartIsu0RxIrqDeferred(void* data)
{
    uintptr_t avail = UART_ReadAvailable(modbus);
    if (avail == 0) {
        UART_Print(debug, "ERROR: UART received interrupt for zero bytes.\r\n");
        return;
    }

    avail = MIN(avail, MB_RTU_MAX_ADU_SIZE - size);
    if (UART_Read(modbus, modbus_frame + size, avail) != ERROR_NONE) {
        UART_Print(debug, "ERROR: Failed to read ");
        UART_PrintUInt(debug, avail);
        UART_Print(debug, " bytes from UART.\r\n");
        return;
    }

    UART_Print(debug, "UART received ");
    UART_PrintUInt(debug, avail);
    UART_Print(debug, " bytes: \'");
    //UART_Write(debug, modbus_frame + end, avail);
    printBytes(modbus_frame, size, avail);
    UART_Print(debug, "\'.\r\n");

    // Send bytes to A7 if in uard read mode
    int32_t error = Socket_Write(socket, &A7ID, modbus_frame, size + avail);
    if (error != ERROR_NONE) {
        UART_Printf(debug, "ERROR: sending bytes to A7 with error code %ld\r\n", error);
    } else {
        size = 0;
    }
}

static void HandleUartIsu0RxIrq(void) {
    static CallbackNode cbn = { .enqueued = false, .cb = HandleUartIsu0RxIrqDeferred };
    EnqueueCallback(&cbn);
}

static CallbackNode *volatile callbacks = NULL;

static void EnqueueCallback(CallbackNode *node)
{
    uint32_t prevBasePri = NVIC_BlockIRQs();
    if (!node->enqueued) {
        CallbackNode *prevHead = callbacks;
        node->enqueued = true;
        callbacks = node;
        node->next = prevHead;
    }
    NVIC_RestoreIRQs(prevBasePri);
}

static void InvokeCallbacks(void)
{
    CallbackNode *node;
    do {
        uint32_t prevBasePri = NVIC_BlockIRQs();
        node = callbacks;
        if (node) {
            node->enqueued = false;
            callbacks = node->next;
        }
        NVIC_RestoreIRQs(prevBasePri);

        if (node) {
            (node->cb)(node->data);
        }
    } while (node);
}

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(197600000);

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "MT3620_IDC_RTApp\r\n");
    UART_Print(debug, "App built on: " __DATE__ " " __TIME__ "\r\n");

    // Setup socket
    socket = Socket_Open(handleRecvMsgWrapper);
    if (!socket) {
        UART_Printf(debug, "ERROR: socket initialisation failed\r\n");
    }

    for (;;) {
        __asm__("wfi");
        InvokeCallbacks();
    }
}
