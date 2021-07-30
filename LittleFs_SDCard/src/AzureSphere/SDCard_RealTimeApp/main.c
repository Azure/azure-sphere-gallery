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
#include <unistd.h>
#include <string.h>

#include "lib/CPUFreq.h"
#include "lib/VectorTable.h"
#include "lib/NVIC.h"
#include "lib/UART.h"
#include "lib/Print.h"
#include "lib/SPIMaster.h"

#include "Socket.h"
#include "intercore_messages.h"
#include "SD.h"

static SPIMaster* driver = NULL;
static SDCard* card = NULL;

#define PAGE_SIZE 512
uint8_t dataBlock[PAGE_SIZE];

struct SD_CMD_WITH_DATA txStruct;

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static const Component_Id A7ID =
{
    .seg_0 = 0x25025d2c,
    .seg_1 = 0x66da,
    .seg_2 = 0x4448,
    .seg_3_4 = {0xba, 0xe1, 0xac, 0x26, 0xfc, 0xdd, 0x36, 0x27}
};

static uint8_t recvBuffer[720];

// Drivers
static UART   *debug              = NULL;
static Socket *socket             = NULL;

// Callbacks
typedef struct CallbackNode {
    bool enqueued;
    struct CallbackNode *next;
    void *data;
    void (*cb)(void*);
} CallbackNode;

static void EnqueueCallback(CallbackNode *node);

static void printSDBlock(uint8_t* buff, uintptr_t blocklen, unsigned blockID)
{
    uint8_t ascBuffer[17];
    int byteCount = 0;

    UART_Printf(debug, "SD Card Data (block %u):\r\n", blockID);
    uintptr_t i;

    __builtin_memset(ascBuffer, 0x00, sizeof(ascBuffer));

    for (i = 0; i < blocklen; i++) {
        UART_PrintHexWidth(debug, buff[i], 2);
        if (buff[i] > 0x20 && buff[i] < 0x80)
        {
            ascBuffer[byteCount] = buff[i];
        }
        else
        {
            ascBuffer[byteCount] = '.';
        }

        UART_Print(debug, " ");
        byteCount++;
        ascBuffer[byteCount] = 0x00;

        if (byteCount == sizeof(ascBuffer)-1)
        {
            UART_Printf(debug, "    %s\r\n", ascBuffer);
            byteCount = 0;
        }
    }
    if ((blocklen % (sizeof(ascBuffer) - 1)) != 0) {
        UART_Print(debug, "\r\n");
    }
    UART_Print(debug, "\r\n");
}

static void WriteDataToA7(uint8_t* data, ssize_t size)
{
#ifdef SHOW_DEBUG_INFO
    UART_Printf(debug, "Sending %d bytes to the A7\r\n", size);
    printSDBlock(&txStruct, sizeof(txStruct), txStruct.blockNumber);
#endif

    int32_t error = Socket_Write(socket, &A7ID, data, size);
    if (error != 0)
    {
        UART_Printf(debug, "Error Result: %d\r\n", error);
    }
}

static void handleRecvMsg(void *handle)
{
    Socket *socket = (Socket*)handle;

    Component_Id senderId;

    if (Socket_NegotiationPending(socket)) {
        UART_Printf(debug, "Negotiation pending, attempting renegotiation\r\n");
        // NB: this is blocking, if you want to protect against hanging,
        //     add a timeout
        if (Socket_Negotiate(socket) != ERROR_NONE) {
            UART_Printf(debug, "ERROR: renegotiating socket connection\r\n");
            return;
        }
    }

    uint32_t size = sizeof(recvBuffer);
    int32_t error = Socket_Read(socket, &senderId, &recvBuffer[0], &size);

    if (error != ERROR_NONE) {
        UART_Printf(debug, "ERROR: receiving msg - %ld\r\n", error);
        return;
    }

#ifdef SHOW_DEBUG_INFO
    UART_Printf(debug, "Received %d bytes\r\n", size);
#endif

    struct SD_CMD_WITH_DATA* pMsg = (struct SD_CMD_WITH_DATA*)&recvBuffer[0];

    struct SD_CMD cmd;
    cmd.blockNumber = pMsg->blockNumber;

#ifdef SHOW_DEBUG_INFO
    printSDBlock(pMsg->blockData, size, pMsg->blockNumber);
#endif

    switch (pMsg->id)
    {
    case MSG_BLOCK_READ:
#ifdef SHOW_DEBUG_INFO
        UART_Printf(debug, "READ Block %d - ", pMsg->blockNumber);
#endif

        if (!SD_ReadBlock(card, pMsg->blockNumber, &dataBlock[0])) {
            // If read block fails then return an error state to the A7
            UART_Printf(debug, "ERROR: reading block\r\n");
            cmd.id = MSG_BLOCK_READ_RESULT;
            cmd.read_write_result = -1;
            WriteDataToA7(&cmd, sizeof(cmd));
        }
        else
        {
            // read successful, return the block to the A7
            // printSDBlock(dataBlock, PAGE_SIZE, pMsg->blockNumber);
            txStruct.id = MSG_BLOCK_READ_RESULT;
            txStruct.blockNumber = pMsg->blockNumber;
            __builtin_memcpy(txStruct.blockData, dataBlock, PAGE_SIZE);
#ifdef SHOW_DEBUG_INFO
            UART_Printf(debug, "Returning block %d - ", pMsg->blockNumber);
#endif
            WriteDataToA7(&txStruct, sizeof(txStruct));
        }
        break;

    case MSG_BLOCK_WRITE:
#ifdef SHOW_DEBUG_INFO
        UART_Printf(debug, "WRITE Block %d - ", pMsg->blockNumber);
#endif
        cmd.id = MSG_BLOCK_WRITE_RESULT;

        if (!SD_WriteBlock(card, pMsg->blockNumber, pMsg->blockData)) {
            // write block failed, return error state to the A7
            cmd.read_write_result = -1;
            UART_Printf(debug, "ERROR: writing block %d\r\n",pMsg->blockNumber);
        }
        else
        {
            // write block success, confirm result to the A7
            cmd.id = MSG_BLOCK_WRITE_RESULT;
            cmd.read_write_result = 0;
        }
        WriteDataToA7(&cmd, sizeof(cmd));
        break;
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

/// <summary>
/// Function to read the fist 10 SD Card blocks.
/// </summary>
void SpiSDTest(void)
{
    for (int x = 0; x < 10; x++)
    {
        if (!SD_ReadBlock(card, x, &dataBlock[0])) {
            UART_Printf(debug, "ERROR: reading block %d\r\n",x);
            break;
        }
        else
        {
            printSDBlock(dataBlock, PAGE_SIZE, x);
        }
    }
}

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(197600000);

    txStruct.id = 0xfe;
    txStruct.blockNumber = 0;

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "\033[2J\033[0;0H--------------------------------\r\n");
    UART_Print(debug, "M4 SD Card Interface Application\r\n");
    UART_Print(debug, "App built on: " __DATE__ " " __TIME__ "\r\n");

    //volatile bool f = false;
    //while (!f) {
    //    // empty.
    //}

    // Setup socket
    socket = Socket_Open(handleRecvMsgWrapper);
    if (!socket) {
        UART_Printf(debug, "ERROR: socket initialisation failed\r\n");
    }

    // Open SPI 
    // ISU1 was chosen so the code could run on the Avnet board with a Mikroe SD Card Click board and Adafruit SD Card board on the Seeed RDB
    driver = SPIMaster_Open(MT3620_UNIT_ISU1);
    if (!driver) {
        UART_Print(debug,
            "ERROR: SPI initialisation failed\r\n");
    }
    SPIMaster_DMAEnable(driver, false);

    // Use CSA for chip select.
    SPIMaster_Select(driver, 0);

    card = SD_Open(driver);
    if (!card) {
        UART_Print(debug,
            "ERROR: Failed to open SD card.\r\n");
    }

    // SPI/SD test - read and display the first 10 SD Card blocks
    // SpiSDTest();

    for (;;) {
        __asm__("wfi");
        InvokeCallbacks();
    }
}
