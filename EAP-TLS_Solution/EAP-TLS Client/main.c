/***************************************************************************************
	This is an EAP-TLS Client library & demo solution for Azure Sphere-based devices.
	The App will continuously attempt to establish and maintain the connection
	to the configured RADIUS network, by leveraging the eap_tls_lib client library.
***************************************************************************************/

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <curl/curl.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include "eventloop_timer_utilities.h"
#include "./lib/_environment_config.h"
#include "./lib/eap_tls_lib.h"
#include "./lib/web_api_client.h"


// Global conditional compiling sections
#define RUN_TESTS					// Run test cases in tests.c
#define START_FROM_CLEAN_DEVICE		// This is for testing zero-touch provisioning (when not running tests)

// Conditional includes
#ifdef RUN_TESTS
#	include "tests/tests.h"
#endif

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code.  They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum
{
	ExitCode_Success = 0,
	ExitCode_Error,
	ExitCode_TermHandler_SigTerm,
	ExitCode_TimerHandler_Consume,
	ExitCode_Init_EventLoop,
	ExitCode_Init_DownloadTimer,
	ExitCode_Main_EventLoopFail
} ExitCode;

// Prototypes
static void TerminationHandler(int signalNumber);
#ifndef RUN_TESTS
static ExitCode InitHandlers(void);
static void CloseHandlers(void);
static void TimerEventHandler(EventLoopTimer *timer);
#endif

// Global vars
static volatile sig_atomic_t exitCode = ExitCode_Success;
#ifndef RUN_TESTS
static EventLoop *eventLoop = NULL;
static EventLoopTimer *connectTimer = NULL;
#endif

// Global EAP-TLS network configuration
EapTlsConfig eapTlsConfig;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
	// Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
	exitCode = ExitCode_TermHandler_SigTerm;
}

#ifndef RUN_TESTS
/// <summary>
///     The connection timer event handler.
/// </summary>
static void TimerEventHandler(EventLoopTimer *timer)
{
	if (ConsumeEventLoopTimerEvent(timer) != 0)
	{
		exitCode = ExitCode_TimerHandler_Consume;
		return;
	}

	// Always make sure to be connected to the EAP-TLS network
	if (EapTls_IsNetworkConnected(eapTlsConfig.eapTlsNetworkName) != EapTlsResult_Connected)
	{
		EapTls_RunConnectionManager(&eapTlsConfig);
	}
}

/// <summary>
///     Set up SIGTERM termination handler and event handlers.
/// </summary>
/// <returns>
///		0 on success, or -1 on failure
/// </returns>
static ExitCode InitHandlers(void)
{
	eventLoop = EventLoop_Create();
	if (eventLoop == NULL)
	{
		Log_Debug("Could not create event loop.\n");
		return ExitCode_Init_EventLoop;
	}

	// Verify and maintain the RADIUS connection at the specified period.
	static const struct timespec tenSeconds = { .tv_sec = 10, .tv_nsec = 0 };
	connectTimer = CreateEventLoopPeriodicTimer(eventLoop, &TimerEventHandler, &tenSeconds);
	if (connectTimer == NULL)
	{
		return ExitCode_Init_DownloadTimer;
	}

	return ExitCode_Success;
}

/// <summary>
///     Clean up the resources previously allocated.
/// </summary>
static void CloseHandlers(void)
{
	DisposeEventLoopTimer(connectTimer);
	EventLoop_Close(eventLoop);
}
#endif


/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
	Log_Debug("EAP-TLS Client starting...\n");

	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = TerminationHandler;
	sigaction(SIGTERM, &action, NULL);


#ifdef RUN_TESTS

	ssize_t NetworkInterfaceCount = Networking_GetInterfaceCount();
	Networking_NetworkInterface *networkInterfaces = NULL;
	Networking_GetInterfaces(networkInterfaces, (size_t)NetworkInterfaceCount);

	exitCode = (sig_atomic_t)TestEapTlsLib_All(&eapTlsConfig, NetworkInterfaceType_Wifi);
	if (EapTlsResult_Success == exitCode)
	{
		exitCode = (sig_atomic_t)TestEapTlsLib_All(&eapTlsConfig, NetworkInterfaceType_Ethernet);
	}

#else // RUN_TESTS

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Initialize the library with minimal predefined test data (this must be customized with actual requirements).
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	{
		// Full-reset the configuration
		memset(&eapTlsConfig, 0, sizeof(EapTlsConfig));

		// Define the BOOTSTRAP network	
		EapTls_SetBootstrapNetworkInterfaceType(&eapTlsConfig, NetworkInterfaceType_Wifi);
		strncpy(eapTlsConfig.bootstrapNetworkName, g_bootstrapNetworkName, sizeof(eapTlsConfig.bootstrapNetworkName) - 1);
		strncpy(eapTlsConfig.bootstrapNetworkSsid, g_bootstrapNetworkSsid, sizeof(eapTlsConfig.bootstrapNetworkSsid) - 1);

		// Define the WebAPI Server certificate
		strncpy(eapTlsConfig.mdmWebApiInterfaceUrl, g_webApiInterfaceUrl, sizeof(eapTlsConfig.mdmWebApiInterfaceUrl) - 1);
		strncpy(eapTlsConfig.mdmWebApiRootCertificate.relativePath, g_webApiRootCaCertificatePath, sizeof(eapTlsConfig.mdmWebApiRootCertificate.relativePath) - 1);

		// Define the RADIUS network
		strncpy(eapTlsConfig.eapTlsNetworkName, g_eapTlsNetworkName, sizeof(eapTlsConfig.eapTlsNetworkName) - 1);

		// Define the RADIUS RootCA certificate
		strncpy(eapTlsConfig.eapTlsRootCertificate.id, g_eapTlsRootCaCertificateId, sizeof(eapTlsConfig.eapTlsRootCertificate.id) - 1);

		// Define the RADIUS Client certificate
		strncpy(eapTlsConfig.eapTlsClientCertificate.id, g_eapTlsClientCertificateId, sizeof(eapTlsConfig.eapTlsClientCertificate.id) - 1);
#	ifdef USE_CLIENT_CERT_PRIVATE_KEY_PASS_FROM_WEBAPI
		strncpy(eapTlsConfig.eapTlsClientCertificate.privateKeyPass, "", sizeof(eapTlsConfig.eapTlsClientCertificate.privateKeyPass) - 1);
#	else
		strncpy(eapTlsConfig.eapTlsClientCertificate.privateKeyPass, g_eapTlsClientPrivateKeyPassword, sizeof(eapTlsConfig.eapTlsClientCertificate.privateKeyPass) - 1);
#	endif // USE_CLIENT_CERT_PRIVATE_KEY_PASS_FROM_WEBAPI


#	ifdef START_FROM_CLEAN_DEVICE

		Log_Debug("Cleaning up all network configurations and certificate store...\n");
		{
			// Remove all networks
			int res = WifiConfig_ForgetAllNetworks();
			if (-1 == res)
			{
				Log_Debug("ERROR forgetting all network configurations: errno=%d (%s)\n", errno, strerror(errno));
			}

			// Remove all certificates from the CertStore
			int cnt;
			while ((cnt = CertStore_GetCertificateCount()) > 0)
			{
				CertStore_Identifier id;
				res = CertStore_GetCertificateIdentifierAt(0, &id);
				if (-1 == res)
				{
					Log_Debug("FATAL CERTSTORE ERROR finding certificate @ index[%d]/%d in the store: errno=%d (%s)\n", 0, cnt, errno, strerror(errno));

					break;
				}
				else
				{
					res = CertStore_DeleteCertificate(id.identifier);
					if (-1 == res)
					{
						Log_Debug("ERROR deleting certificate '%s' in the CertStore: errno=%d (%s)\n", id.identifier, errno, strerror(errno));
					}
					else
					{
						Log_Debug("Deleted certificate '%s' in the CertStore\n", id.identifier, errno, strerror(errno));
					}
				}
			}

			// Configure the Bootstrap network over WiFi
			if (NetworkInterfaceType_Wifi == eapTlsConfig.bootstrapNetworkInterfaceType)
			{
				EapTls_AddNetwork(eapTlsConfig.bootstrapNetworkName, g_bootstrapNetworkSsid, WifiConfig_Security_Wpa2_Psk, g_bootstrapNetworkPassword);
			}
		}
#	endif // START_FROM_CLEAN_DEVICE
	}

	InitHandlers();

	// Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
	while (exitCode == ExitCode_Success)
	{
		EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);

		// Continue if interrupted by signal, e.g. due to breakpoint being set.
		if (result == EventLoop_Run_Failed && errno != EINTR)
		{
			exitCode = ExitCode_Main_EventLoopFail;
		}
	}

	CloseHandlers();
#endif // RUN_TESTS

	Log_Debug("Application exiting.\n");
	return exitCode;
}
