/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <applibs/log.h>
#include <signal.h>

#include <applibs/eventloop.h>
#include <applibs/wificonfig.h>
#include "eventloop_timer_utilities.h"

#include "utils.h"
#include "M24SR.h"

static void TerminationHandler(int signalNumber);
static void ClosePeripheralsAndHandlers(void);
static ExitCode InitPeripheralsAndHandlers(void);

volatile sig_atomic_t exitCode = ExitCode_Success;
static EventLoop* eventLoop = NULL;

static void TerminationHandler(int signalNumber)
{
	// Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
	exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
/// Callback from M24SR.c - called on NFC Tap event.
/// </summary>
/// 
void NDEF_Callback(const struct M24SR_WifiConfig* wificonfig)
{
	if (wificonfig != NULL)
	{
		Log_Debug("Adding network for SSID: %s, Key: %s\n", wificonfig->ssidString, wificonfig->networkKeyString);
		// If we have a valid WiFi NDEF Record - Configure WiFi.

		int networkId = WifiConfig_AddNetwork();
		WifiConfig_SetSSID(networkId, wificonfig->ssidString, strlen(wificonfig->ssidString));
		WifiConfig_SetSecurityType(networkId, wificonfig->wifiConfigSecurityType);
		WifiConfig_SetPSK(networkId, wificonfig->networkKeyString, strlen(wificonfig->networkKeyString));
		WifiConfig_SetNetworkEnabled(networkId, true);
		WifiConfig_PersistConfig();
	}
}

/// <summary>
/// Initialize EventLoop, M24SR
/// </summary>
static ExitCode InitPeripheralsAndHandlers(void)
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = TerminationHandler;
	sigaction(SIGTERM, &action, NULL);

	eventLoop = EventLoop_Create();
	if (eventLoop == NULL) {
		Log_Debug("Could not create event loop.\n");
		exitCode = ExitCode_Init_EventLoop;
		return exitCode;
	}

	M24SR_Init(eventLoop, NDEF_Callback);

	Log_Debug("InitPeripherals done.\n");
	return ExitCode_Success;
}

/// <summary>
/// Cleanup resources.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
	M24SR_ClosePeripheralsAndHandlers();
	EventLoop_Close(eventLoop);
}

int main(void)
{
	exitCode = InitPeripheralsAndHandlers();

	while (exitCode == ExitCode_Success)
	{
		EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
		// Continue if interrupted by signal, e.g. due to breakpoint being set.
		if (result == EventLoop_Run_Failed && errno != EINTR) {
			exitCode = ExitCode_Main_EventLoopFail;
		}
	}

	ClosePeripheralsAndHandlers();

    return 0;
}