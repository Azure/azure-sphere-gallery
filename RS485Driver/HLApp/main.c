/*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/

// This sample C application for Azure Sphere sends messages to, and receives
// responses from, a real-time capable application running an RS-485 driver.
// It sends a message every second and prints the message which was sent, 
// and the response which was received.
//
// It uses the following Azure Sphere libraries
// - log (displays messages in the Device Output window during debugging)
// - application (establish a connection with a real-time capable application).
// - eventloop (system invokes handlers for timer events)

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/socket.h>

#include <applibs/log.h>
#include <applibs/application.h>

#include "../common_defs.h"
#include "eventloop_timer_utilities.h"
#include "rs485_hl_driver.h"



/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
	ExitCode_Success = 0,
	ExitCode_TermHandler_SigTerm = 1,
	ExitCode_TimerHandler_Consume = 2,
	ExitCode_SendMsg_Send = 3,
	ExitCode_SocketHandler_Recv = 4,
	ExitCode_Init_EventLoop = 5,
	ExitCode_Init_SendTimer = 6,
	ExitCode_Init_Connection = 7,
	ExitCode_Init_Rs485 = 8,
	ExitCode_Main_EventLoopFail = 9
} ExitCode;

static EventLoop *eventLoop = NULL;
static EventLoopTimer *sendTimer = NULL;
static volatile sig_atomic_t exitCode = ExitCode_Success;
static uint8_t rs485RxBuffer[2000];

// Handy typedef, used in SendTimerEventHandler for processing Modbus commands
typedef struct {

	uint8_t *command;
	uint8_t length;

} message_t;

const char rtAppComponentId[] = "1CCE66F1-28E9-4DA4-AD25-D247FD362DE7";

static void TerminationHandler(int signalNumber);
static void SendTimerEventHandler(EventLoopTimer *timer);
static void Rs485ReceiveHandler(int bytesReceived);
static ExitCode InitHandlers(void);
static void CloseHandlers(void);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
	// Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
	exitCode = ExitCode_TermHandler_SigTerm;
}



/// <summary>
///     Handle send timer event by sending a command sequence to the RS-485 driver.
/// </summary>
static void SendTimerEventHandler(EventLoopTimer *timer)
{
	static int currCommand = 0;
	static const message_t commands[] = {

		{ "\xff\xff\xff\xff\x80\x25\x00\x00", 8},	// Change baud rate on the RS-485 driver (Note: invert endianness in the baudrate value!)
		{ "\x01\x04\x00\x01\x00\x01\x60\x0A", 8},	// Measure temperature
		{ "\x01\x04\x00\x02\x00\x01\x90\x0A", 8},	// Measure humidity
	};

	if (ConsumeEventLoopTimerEvent(timer) != 0) {
		exitCode = ExitCode_TimerHandler_Consume;
		return;
	}

	Rs485_Send(commands[currCommand].command, commands[currCommand].length);
	currCommand++;
	currCommand %= 3;
}

/// <summary>
///     Handle receive callback from the RS-485 driver.
///	    The received bytes will be available in the RX buffer passed to Rs485_Init().
/// </summary>
static void Rs485ReceiveHandler(int bytesReceived)
{
	// Just re-logging the received bytes.	
	if (bytesReceived > 0)
	{		
		Log_Debug("Rs485 Callback: received %d bytes: ", bytesReceived);
		for (int i = 0; i < bytesReceived; ++i) {
			Log_Debug("%02x", rs485RxBuffer[i]);
			if (i != bytesReceived - 1) {
				Log_Debug(":");
			}
		}
		Log_Debug("\n");
	}

	return;
}

/// <summary>
///     Set up SIGTERM termination handler and event handlers for send timer
///     and to receive data from real-time capable application.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode InitHandlers(void)
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = TerminationHandler;
	sigaction(SIGTERM, &action, NULL);

	eventLoop = EventLoop_Create();
	if (eventLoop == NULL) {
		Log_Debug("Could not create event loop.\n");
		return ExitCode_Init_EventLoop;
	}

	// Register a one-second timer to send a message to the real-time RS-485 driver (RTApp).
	static const struct timespec sendPeriod = { .tv_sec = 1, .tv_nsec = 0 };
	sendTimer = CreateEventLoopPeriodicTimer(eventLoop, &SendTimerEventHandler, &sendPeriod);
	if (sendTimer == NULL) {
		return ExitCode_Init_SendTimer;
	}
	
	// Initialize the real-time RS-485 driver (RTApp).
	if (Rs485_Init(eventLoop, rs485RxBuffer, sizeof(rs485RxBuffer), Rs485ReceiveHandler) == -1) {
		return ExitCode_Init_Rs485;
	}

	return ExitCode_Success;
}

/// <summary>
///     Clean up the resources previously allocated.
/// </summary>
static void CloseHandlers(void)
{
	DisposeEventLoopTimer(sendTimer);
	Rs485_Close();
	EventLoop_Close(eventLoop);
}

int main(void)
{
	Log_Debug("High-level RS-485 comms application\n");
	Log_Debug("Sends messages to, and receives messages from an RS-485 driver running on the RT-Core.\n");

	exitCode = InitHandlers();

	while (exitCode == ExitCode_Success) {
		EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
		// Continue if interrupted by signal, e.g. due to breakpoint being set.
		if (result == EventLoop_Run_Failed && errno != EINTR) {
			exitCode = ExitCode_Main_EventLoopFail;
		}
	}

	CloseHandlers();
	Log_Debug("Application exiting.\n");
	return exitCode;
}
