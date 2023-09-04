/*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/

#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <applibs/log.h>
#include <applibs/application.h>

#include "../common_defs.h"
#include "eventloop_timer_utilities.h"
#include "rs485_hl_driver.h"

static int rtAppSockFd = -1;
static EventLoop *rs485eventLoop;
static EventRegistration *socketEventReg = NULL;
static Rs485ReceiveCallback *userCallback = NULL;
static uint8_t *rs485rxBuffer = NULL;
static size_t rs485rxBufferSize = 0;

static void RTAppSocketEventHandler(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);

int Rs485_Init(EventLoop *eventLoop, uint8_t *rxBuffer, size_t rxBufferSize, Rs485ReceiveCallback *callback)
{
	if (rtAppSockFd >= 0) {

		Log_Debug("ERROR: RS-485 driver already initialized! Call Rs485_Close() before re-initializing.\n");
		return -1;
	}
	else
	{
		// Open a connection to the RS-485 driver RTApp.
		rtAppSockFd = Application_Connect(rtAppComponentId);
		if (rtAppSockFd == -1) {
			Log_Debug("ERROR: Unable to create socket: %d (%s)\n", errno, strerror(errno));
			return -1;
		}

		// Set the socket's timeout, to handle cases where real-time capable application does not respond.
		static const struct timeval recvTimeout = { .tv_sec = 5, .tv_usec = 0 };
		int result = setsockopt(rtAppSockFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));
		if (result == -1) {
			Log_Debug("ERROR: Unable to set socket timeout: %d (%s)\n", errno, strerror(errno));
			return -1;
		}

		// Register the handler for incoming messages from real-time RS-485 driver.
		socketEventReg = EventLoop_RegisterIo(eventLoop, rtAppSockFd, EventLoop_Input, RTAppSocketEventHandler, /* context */ NULL);
		if (socketEventReg == NULL) {
			Log_Debug("ERROR: Unable to register socket event: %d (%s)\n", errno, strerror(errno));
			return -1;
		}

		// Setup the user-callback
		userCallback = callback;

		// Setup the RX buffer
		if (NULL == rxBuffer || rxBufferSize < 32)
		{
			Log_Debug("ERROR: RX buffer not defined or too small: %p (%zu bytes)\n", rxBuffer, rxBufferSize);
			return -1;
		}
		else
		{
			rs485rxBuffer = rxBuffer;
			rs485rxBufferSize = rxBufferSize;
		}
	}

	return 0;
}

void Rs485_Close(void)
{
	EventLoop_UnregisterIo(rs485eventLoop, socketEventReg);
	if (rtAppSockFd >= 0) {
		int result = close(rtAppSockFd);
		if (result != 0) {
			Log_Debug("ERROR: Could not close rtAppSockFd %s: %s (%d).\n", "RTApp socket", strerror(errno), errno);
		}
	}

	rtAppSockFd = -1;
	userCallback = NULL;
	rs485rxBuffer = NULL;
	rs485rxBufferSize = 0;
}

int Rs485_Send(const void *data, size_t dataLen)
{
	// Prepare the block
	if (dataLen > MAX_HLAPP_MESSAGE_SIZE)
	{
		Log_Debug("ERROR: data buffer too big: %d (> %d)\n", dataLen, MAX_HLAPP_MESSAGE_SIZE);
		return -1;
	}

	// Log the bytes to be sent
	Log_Debug("Rs485_Driver: sending %ld bytes: ", dataLen);
	for (int i = 0; i < dataLen; ++i) {
		Log_Debug("%02x", ((char*)data)[i]);
		if (i != dataLen - 1) {
			Log_Debug(":");
		}
	}
	Log_Debug("\n");

	// Send the block to the RS-485 RTApp driver
	int bytesSent = send(rtAppSockFd, data, dataLen, 0);
	if (bytesSent == -1) {
		Log_Debug("ERROR: Unable to send message to the RS-485 driver: %d (%s)\n", errno, strerror(errno));
		return -1;
	}

	return bytesSent;
}

void RTAppSocketEventHandler(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
	if (NULL != rs485rxBuffer)
	{
		// Read response from real-time capable application.
		// If the RTApp has sent more than rs485rxBufferSize, then truncate.
		int bytesReceived = recv(fd, rs485rxBuffer, rs485rxBufferSize, 0);

		if (bytesReceived == -1) {
			Log_Debug("ERROR: Unable to receive message from the RS-485 driver: %d (%s)\n", errno, strerror(errno));
			return;
		}

		// Log the received bytes
		Log_Debug("Rs485_Driver: received %d bytes: ", bytesReceived);
		for (int i = 0; i < bytesReceived; ++i) {
			Log_Debug("%02x", rs485rxBuffer[i]);
			if (i != bytesReceived - 1) {
				Log_Debug(":");
			}
		}
		Log_Debug("\n");

		if (NULL != userCallback)
		{
			userCallback(bytesReceived);
		}
	}
}
