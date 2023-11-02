/*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "lib/mt3620/gpt.h"
#include "lib/GPT.h"
#include "lib/CPUFreq.h"
#include "lib/VectorTable.h"
#include "lib/NVIC.h"
#include "lib/Print.h"

#include "../common_defs.h"
#include "Socket.h"
#include "ringBuffer.h"
#include "rs485_driver.h"

#define DEBUG_INFO
static UART *debug = NULL;
static Socket *socket = NULL;
static GPT *sendTimer = NULL;

static const Component_Id A7ID =
{
	.seg_0 = 0x96ACA524,
	.seg_1 = 0x8113,
	.seg_2 = 0x4171,
	.seg_3_4 = {0x9C, 0x76, 0x6F, 0xBD, 0xBB, 0x44, 0x11, 0x31}
};

typedef struct CallbackNode {
	bool enqueued;
	struct CallbackNode *next;
	void *data;
	void (*cb_void)(void);
	void (*cb_void_ptr)(void *);
} CallbackNode;
static CallbackNode *volatile callbacks = NULL;


// Function prototypes
static void HandleUartRxIrq(void);

// Callback handlers
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
	CallbackNode *node = NULL;

	do {
		uint32_t prevBasePri = NVIC_BlockIRQs();
		node = callbacks;
		if (node) {
			node->enqueued = false;
			callbacks = node->next;
		}
		NVIC_RestoreIRQs(prevBasePri);

		if (node) {
			if (NULL != node->cb_void)
			{
				(*node->cb_void)();
			}
			else if (NULL != node->cb_void_ptr)
			{
				(*node->cb_void_ptr)(node->data);
			}
		}
	} while (node);
}

// Handlers for messages received from the HLApp
static void handleRecvMsg(void *handle)
{
	Socket *socket = (Socket *)handle;

	Component_Id senderId;
	static uint8_t msg[MAX_HLAPP_MESSAGE_SIZE];

	if (Socket_NegotiationPending(socket)) {
		UART_Printf(debug, "Negotiation pending, attempting renegotiation\n");
		
		// NB: this is blocking, if you want to protect against hanging, add a timeout!
		if (Socket_Negotiate(socket) != ERROR_NONE) {
			UART_Printf(debug, "ERROR: renegotiating socket connection\n");
		}
	}

	// Read the message from the HLApp mailbox socket
	uint32_t bytesRead = sizeof(msg);
	int32_t error = Socket_Read(socket, &senderId, &msg, &bytesRead);
	if (error != ERROR_NONE) {
		UART_Printf(debug, "ERROR: receiving message from HLApp - %ld\r\n", error);
	}
	else if (bytesRead > sizeof(msg)) {
		UART_Printf(debug, "ERROR: message from HLApp too long - %ld\r\n", bytesRead);
	}


	// Is this a special command?
	if (*((uint32_t *)&msg[0]) == 0xffffffffUL)
	{
		bool bRes = Rs485_Init(*((uint16_t *)&msg[4]), NULL);

		if (bRes)
		{
			memcpy(msg, "\xff\xff\xff\xff\x00\x00\x00\x00", 8);			
		}
		else
		{
			memcpy(msg, "\xff\xff\xff\xff\xff\xff\xff\xff", 8);
		}

#ifdef DEBUG_INFO
		UART_Printf(debug, "Changing baud rate to %d --> %s\r\n", *((uint16_t *)&msg[4]), bRes ? "OK" : "FAILED!!");
#endif

		if (ring_buffer_push_bytes(&rs485_rxRingBuffer, msg, 8) == -1)
		{
			UART_Print(debug, "Message to HLApp LOST (rs485_rxRingBuffer overflow)!! ");
		}
	}
	else
	{
#ifdef DEBUG_INFO
		UART_Printf(debug, "Received %ld bytes from HLApp: ", bytesRead);
		for (uint32_t i = 0; i < bytesRead; ++i) {
			UART_Printf(debug, "%02x", msg[i]);
			if (i != bytesRead - 1) {
				UART_Print(debug, ":");
			}
		}
		UART_Print(debug, " --> sending to RS-485 field bus\r\n");
#endif
		if ((error = Rs485_Write(msg, bytesRead)) != ERROR_NONE)
		{
			UART_Printf(debug, "Message from HLApp LOST (error: %ld)!!\r\n", error);
		}
	}
}
static void handleRecvMsgWrapper(Socket *handle)
{
	static CallbackNode cbn = { .enqueued = false, .cb_void = NULL, .cb_void_ptr = handleRecvMsg, .data = NULL };

	if (!cbn.data) {
		cbn.data = handle;
	}

	EnqueueCallback(&cbn);
}

// Handler for messages to be sent to the HLApp
static void handleSendMsgTimer(void *data)
{
	// Dequeue the bytes to be sent to the HLApp
	int bytesBuffered = ring_buffer_count(&rs485_rxRingBuffer);
	uint8_t buffer[bytesBuffered];
	int32_t error;

	if ((error = ring_buffer_pop_bytes(&rs485_rxRingBuffer, buffer, bytesBuffered)) == -1)
	{
		UART_Printf(debug, "Message from HLApp LOST (error: %ld)!!\r\n", error);
	}
	else
	{
#ifdef DEBUG_INFO
		UART_Printf(debug, "Sending %d bytes to HLApp: ", bytesBuffered);
		for (uint32_t i = 0; i < bytesBuffered; ++i) {
			UART_Printf(debug, "%02x", buffer[i]);
			if (i != bytesBuffered - 1) {
				UART_Print(debug, ":");
			}
		}
		UART_Print(debug, "\r\n");
#endif
		error = Socket_Write(socket, &A7ID, buffer, bytesBuffered);
		if (error != ERROR_NONE) {
			UART_Printf(debug, "ERROR: sending message - %ld\r\n", error);
		}
	}

	Socket_Reset(socket); // Simulate reboot
}
static void handleSendMsgTimerWrapper(GPT *timer)
{
	if (NULL != timer)
		(void)(timer);

	static CallbackNode cbn = { .enqueued = false, .cb_void = NULL, .cb_void_ptr = handleSendMsgTimer, .data = NULL };
	EnqueueCallback(&cbn);
}

// IRQ Handlers for the RS-485 UART
static void HandleUartRxIrqDeferred(void)
{
	uintptr_t avail = Rs485_ReadAvailable();
	if (avail == 0) {
		UART_Print(debug, "ERROR: UART received interrupt for zero bytes.\r\n");
		return;
	}

	uint8_t buffer[avail];
	if (Rs485_Read(buffer, avail) != ERROR_NONE) {

		UART_Printf(debug, "ERROR: Failed to read %zu bytes from UART.\r\n", avail);
		return;
	}

#ifdef DEBUG_INFO
	UART_Printf(debug, "Received %zu bytes from RS-485 bus: ", avail);
	for (uint32_t i = 0; i < avail; ++i) {
		UART_Printf(debug, "%02x", buffer[i]);
		if (i != avail - 1) {
			UART_Print(debug, ":");
		}
	}
	UART_Printf(debug, "\r\n");
#endif

	// If the RX buffer overflows the desired limit, immediately send the bytes to the HLApp
	// so to lower chances of losing bytes from the serial port in between GPT interrupts.
	if (ring_buffer_count(&rs485_rxRingBuffer) > DRIVER_MAX_RX_BUFFER_FILL_SIZE)
	{
		handleSendMsgTimerWrapper(NULL);
	}

	// Enqueue the received bytes in the ring buffer, to be sent to the HLApp upon GPT interrupts.
	if (ring_buffer_push_bytes(&rs485_rxRingBuffer, buffer, avail) == -1)
	{
		UART_Print(debug, "Message from UART LOST (rs485_rxRingBuffer overflow)!! ");
	}
}
static void HandleUartRxIrq(void) {
	static CallbackNode cbn = { .enqueued = false, .cb_void = HandleUartRxIrqDeferred, .cb_void_ptr = NULL, .data = NULL };
	EnqueueCallback(&cbn);
}

_Noreturn void RTCoreMain(void)
{
	VectorTableInit();
	//CPUFreq_Set(26000000);

	// Initialize the debug UART
	debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
	UART_Print(debug, "RS-485 real-time driver\r\n");
	UART_Print(debug, "Built on: " __DATE__ " " __TIME__ "\r\n");

	// Initialize the RS-485 driver
	Rs485_Init(9600, HandleUartRxIrq);

	// Setup GPT1 as "Write to HLApp" timer
	sendTimer = GPT_Open(MT3620_UNIT_GPT0, MT3620_GPT_012_HIGH_SPEED, GPT_MODE_REPEAT);
	if (!sendTimer) {
		UART_Printf(debug, "ERROR: GPT_Open failed\r\n");
	}
	else
	{
		int32_t error;
		if ((error = GPT_SetMode(sendTimer, GPT_MODE_REPEAT)) != ERROR_NONE) {
			UART_Printf(debug, "ERROR: GPT_SetMode failed %ld\r\n", error);
		}
		if ((error = GPT_StartTimeout(
			sendTimer, RTDRV_SEND_DELAY_MSEC, GPT_UNITS_MILLISEC,
			handleSendMsgTimerWrapper)) != ERROR_NONE) {
			UART_Printf(debug, "ERROR: GPT_StartTimeout failed %ld\r\n", error);
		}
	}

	// Setup the receive socket for the HLApp
	socket = Socket_Open(handleRecvMsgWrapper);
	if (!socket) {
		UART_Printf(debug, "ERROR: Socket_Open failed\r\n");
	}

	for (;;) {
		__asm__("wfi");
		InvokeCallbacks();
	}
}

