#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "../applibs_versions.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/wificonfig.h>
#include <applibs/storage.h>
#include <applibs/certstore.h>
#include <tlsutils/deviceauth_curl.h>

#include "../parson.h"
#include "../eventloop_timer_utilities.h"
#include "eap_tls_lib.h"
#include "web_api_client.h"

////////////////////////////////////////////////////////////////////////
// NOTE: search for '#NOTE' for incomplete implementations/highlights //
////////////////////////////////////////////////////////////////////////


/// <summary>
///     Globally accessible device configuration.
/// </summary>
DeviceConfiguration deviceConfiguration;

/// <summary>
///     State machine for the EapTls_RunConnectionManager API.
/// </summary>
typedef enum
{
	ConnectionManagerState_TBD = -1,						// Just a dead-end, to capture incomplete code branching
	ConnectionManagerState_Idle = 0,						// Just start-off: let's check if we have RootCA and Client certificates
	ConnectionManagerState_CheckCertsInstalled,				// Check if we have installed RootCA and Client certificates to use for connecting to the EAP_TLS network
	ConnectionManagerState_Installcerts,					// Install RootCA and Client certificates in the certificate store
	ConnectionManagerState_Installcerts_Dup,				// Install <new> RootCA and Client certificates in the certificate store, to be registered into the EAP_TLS's <TEMPORARY CLONED> network configuration
	ConnectionManagerState_AddEapTlsNetwork,				// Add the EAP_TLS network configuration from scratch (eventually removes existing)
	ConnectionManagerState_CloneEapTlsNetwork,				// Clone the EAP_TLS network configuration, for use in certificate renewal
	ConnectionManagerState_ConfigureEapTlsNetwork,			// Configure the EAP_TLS's network security (installing CA/Client certs)
	ConnectionManagerState_ConfigureEapTlsNetwork_Dup,		// Configure the EAP_TLS's <TEMPORARY CLONE> network security (installing CA/Client certs)
	ConnectionManagerState_ConnectToEapTlsNetwork,			// Attempt connecting to the EAP_TLS network
	ConnectionManagerState_ConnectToEapTlsNetwork_Dup,		// Attempt connecting to the EAP_TLS's <TEMPORARY CLONE> network
	ConnectionManagerState_RequestCertificates,				// Initiate a new RootCA and/or Client certificate request
	ConnectionManagerState_ConnectToBootstrapNetwork,		// Attempt connecting to a bootstrap network, in order to connect to the WebAPI and retrieve new certificates
	ConnectionManagerState_CallMdmWebApi,					// Request a new Client certificate to the WebAPI/CMS
	ConnectionManagerState_HandleMdmWebApiResponse,			// Handle the response from the WebAPI/CMS
	ConnectionManagerState_SwapEapTlsNetworks,				// Swap the original EAP_TLS network with the EAP_TLS's <TEMPORARY CLONE> network
	ConnectionManagerState_Connected_Exit,					// The device is connected to the EAP-TLS network -> the App can proceed its execution
	ConnectionManagerState_Error_Exit						// The device cannot connect to the EAP-TLS network -> let's return so the App can proceed its execution based on the return result

} ConnectionManagerState;

/// <summary>
///     Local structs & state-variables for the connection timer-handler
/// </summary>
static const struct timespec connectionPollSeconds = { .tv_sec = 10, .tv_nsec = 0 };
typedef struct
{
	char connectionNetworkName[WIFICONFIG_CONFIG_NAME_MAX_LENGTH + 1];
	EventLoop *eventLoop;
	EventLoopTimer *connectionTimer;
	volatile sig_atomic_t connectionRetries;
	volatile sig_atomic_t exitCode;
} ConnectionTimerHandlerContext;
static ConnectionTimerHandlerContext connectionTimerHandlerContext =
{
	"",
	NULL,
	NULL,
	MAX_CONNECTION_RETRIES,
	EapTlsResult_Error,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
// Logging utilities
//////////////////////////////////////////////////////////////
#define EAP_TLS_LIB_LOG_PREFIX "EAP-TLS lib: "
#define EapTls_Log(...) Log_Debug(EAP_TLS_LIB_LOG_PREFIX __VA_ARGS__)
//static void EapTls_Log(const char *fmt, ...)
//{
//	va_list myargs;
//	va_start(myargs, fmt);
//	
//	Log_Debug(EAP_TLS_LIB_LOG_PREFIX);
//	Log_Debug(fmt, myargs);
//
//	va_end(myargs);
//}


//////////////////////////////////////////////////////////////
// Timer handler for connecting to a network configuration
//////////////////////////////////////////////////////////////
static void ConnectionTimerEventHandler(EventLoopTimer *timer)
{
	if (ConsumeEventLoopTimerEvent(timer) != 0)
	{
		connectionTimerHandlerContext.exitCode = EapTlsResult_Connecting;
		return;
	}

	if (connectionTimerHandlerContext.connectionRetries--)
	{
		EapTlsResult iRes = EapTls_IsNetworkConnected(connectionTimerHandlerContext.connectionNetworkName);
		if (EapTlsResult_Disconnected == iRes)
		{
			connectionTimerHandlerContext.exitCode = EapTlsResult_Connecting;
		}
		else
		{
			connectionTimerHandlerContext.exitCode = EapTlsResult_Connected;
		}
	}
	else
	{
		connectionTimerHandlerContext.exitCode = EapTlsResult_ConnectionTimeout;
	}
}
static EapTlsResult InitConnectionTimerHandler(const char *networkName)
{
	EapTlsResult iRes = EapTlsResult_Error;

	// if not, what was the cause?
	int srcNetworkID = WifiConfig_GetNetworkIdByConfigName(networkName);
	if (-1 == srcNetworkID)
	{
		iRes = EapTlsResult_NetworkUnknown;
		EapTls_Log("Cannot find network configuration '%s': errno=%d (%s)\n", networkName, errno, strerror(errno));
	}
	else
	{
		// Ok, we are now connected: we should wait for a timeout for the OS to scan and connect...
		// Will make this blocking, as the App is expecting a deterministic result, and we don't have anything to do in the meanwhile.
		if (NULL != connectionTimerHandlerContext.eventLoop)
		{
			EventLoop_Close(connectionTimerHandlerContext.eventLoop);
			connectionTimerHandlerContext.eventLoop = NULL;
		}
		connectionTimerHandlerContext.eventLoop = EventLoop_Create();
		if (NULL != connectionTimerHandlerContext.eventLoop)
		{
			connectionTimerHandlerContext.connectionRetries = MAX_CONNECTION_RETRIES;
			strncpy(connectionTimerHandlerContext.connectionNetworkName, networkName, sizeof(connectionTimerHandlerContext.connectionNetworkName) - 1);
			connectionTimerHandlerContext.exitCode = EapTlsResult_Connecting;
			connectionTimerHandlerContext.connectionTimer = CreateEventLoopPeriodicTimer(connectionTimerHandlerContext.eventLoop, &ConnectionTimerEventHandler, &connectionPollSeconds);
			if (NULL != connectionTimerHandlerContext.connectionTimer)
			{
				iRes = EapTlsResult_Success;
			}
		}
		else
		{
			EapTls_Log("Could not create event loop.\n");
		}
	}

	return iRes;
}
static void DisposeConnectionTimerHandler(void)
{
	connectionTimerHandlerContext.exitCode = EapTlsResult_Error;
	if (NULL != connectionTimerHandlerContext.connectionTimer)
	{
		DisposeEventLoopTimer(connectionTimerHandlerContext.connectionTimer);
		connectionTimerHandlerContext.connectionTimer = NULL;
	}
	if (NULL != connectionTimerHandlerContext.eventLoop)
	{
		EventLoop_Close(connectionTimerHandlerContext.eventLoop);
		connectionTimerHandlerContext.eventLoop = NULL;
	}
}

//////////////////////////////////////////////////////////////
// Permanent device configuration helpers
//////////////////////////////////////////////////////////////
EapTlsResult EapTls_StoreDeviceConfiguration(const DeviceConfiguration *config)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != config)
	{
		// Note: if the DeviceConfiguration type is changed, the mutable file must be
		// first deleted to store the new object size:
		//
		int fd = Storage_DeleteMutableFile();
		if (-1 == fd)
		{
			EapTls_Log("No previous configuration mutable file to be deleted.\n");
		}
		else
		{
			EapTls_Log("Deleted previous configuration in mutable file.\n");
		}

		fd = Storage_OpenMutableFile();
		if (-1 == fd)
		{
			EapTls_Log("ERROR: could not open mutable file:  %s (%d).\n", strerror(errno), errno);
		}
		else
		{			
			size_t written = 0;
			while (written < sizeof(DeviceConfiguration))
			{
				ssize_t ret = write(fd, (unsigned char *)config + written, sizeof(DeviceConfiguration) - written);
				if (-1 == ret)
				{
					EapTls_Log("ERROR: could write to mutable file:  %s (%d).\n", strerror(errno), errno);
					break;
				}
				else
				{
					written += (size_t)ret;

					if (0 == ret)
					{
						EapTls_Log("FATAL: wrote 0 bytes to mutable file while still missing %d bytes -> aborting.\n", sizeof(DeviceConfiguration) - written);
						break;
					}
					else if (written < sizeof(DeviceConfiguration))
					{
						EapTls_Log("Warning: only wrote %d of %d bytes requested, continuing...\n", written, sizeof(DeviceConfiguration));
					}
					else
					{
						iRes = EapTlsResult_Success;
						EapTls_Log("Successfully persisted the device configuration in mutable file.\n");
						break;
					}
				}
			}
			close(fd);
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_ReadDeviceConfiguration(DeviceConfiguration *configOut)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != configOut)
	{
		int fd = Storage_OpenMutableFile();
		if (-1 != fd)
		{
			ssize_t ret = read(fd, configOut, sizeof(DeviceConfiguration));
			if (-1 == ret)
			{
				EapTls_Log("ERROR: reading from mutable file:  %s (%d).\n", strerror(errno), errno);
			}
			else
			{
				iRes = EapTlsResult_Success;
				EapTls_Log("Successfully read the device configuration from mutable file.\n");
			}
			close(fd);
		}
		else
		{
			EapTls_Log("ERROR: could not open mutable file:  %s (%d).\n", strerror(errno), errno);
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}

//////////////////////////////////////////////////////////////
// Network configuration helpers
//////////////////////////////////////////////////////////////
EapTlsResult EapTls_PersistNetworkConfig(void)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (WifiConfig_PersistConfig() == -1)
	{
		EapTls_Log("Cannot persist eapTlsConfig: errno=%d (%s)\n", errno, strerror(errno));
	}
	else
	{
		if (WifiConfig_ReloadConfig() == -1)
		{
			EapTls_Log("Cannot reload eapTlsConfig: errno=%d (%s)\n", errno, strerror(errno));
		}
		else
		{
			EapTls_Log("Successfully persisted & reloaded network configurations!\n");
			iRes = EapTlsResult_Success;
		}
	}

	return iRes;
}
EapTlsResult EapTls_DiagnoseNetwork(const char *networkName)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != networkName)
	{
		int networkID = WifiConfig_GetNetworkIdByConfigName(networkName);
		if (-1 == networkID)
		{
			iRes = EapTlsResult_NetworkUnknown;
			EapTls_Log("Cannot find network configuration '%s': errno=%d (%s)\n", networkName, errno, strerror(errno));
		}
		else
		{
			WifiConfig_NetworkDiagnostics networkDiagnostics;
			int res = WifiConfig_GetNetworkDiagnostics(networkID, &networkDiagnostics);
			if (-1 == res)
			{
				iRes = EapTlsResult_FailedDiagnosingNetwork;
				EapTls_Log("Failed getting diagnostics for network '%s' - Id[%d]: errno=%d (%s)\n", networkName, networkID, errno, strerror(errno));
			}
			else
			{
				// Check the connection state, and report eventual errors
				if (networkDiagnostics.isConnected)
				{
					iRes = EapTlsResult_Connected;
				}
				else
				{
					switch (networkDiagnostics.error)
					{
						case 2: // NetworkNotFound = 2 : Network was not found.
						{
							iRes = EapTlsResult_NetworkUnknown;
							EapTls_Log("Network '%s' - Id[%d] not found!\n", networkName, networkID);
						}
						break;


						case 5: // AuthenticationFailed = 5: Authentication failed. This error is thrown for EAP-TLS
						{
							// Let's attempt requesting new certificates (we already validated the certs at the first state)
							switch (networkDiagnostics.certError)
							{
								case 101: // InvalidRootCA
								{
									iRes = EapTlsResult_AuthenticationError_InvalidRootCaCert;
								}
								break;

								case 102: // InvalidClientAuth
								{
									iRes = EapTlsResult_AuthenticationError_InvalidClientCert;
								}
								break;
								
								case 103: // UnknownClientId
								{
									iRes = EapTlsResult_AuthenticationError_InvalidClientIdentity;
								}
								break;

								default:
								{
									iRes = EapTlsResult_AuthenticationError;
								}
								break;
							}

							EapTls_Log("Authentication error connecting to network '%s' - Id[%d]: error=%d, certError=%d\n", networkName, networkID, networkDiagnostics.error, networkDiagnostics.certError);
						}
						break;

						case 1:  // ConnectionFailed = 1 : Generic error message when connection fails.
						case 3:  // NoPskIncluded = 3: Network password is missing.
						case 4:  // WrongKey = 4: Network is using an incorrect password.
						case 6:  // SecurityTypeMismatch = 6: The stored network's security type does not match the available network.
						case 7:  // NetworkFrequencyNotAllowed = 7: Network frequency not allowed.
						case 8:  // NetworkNotEssPbssMbss = 8: Network is not supported because no ESS, PBSS or MBSS was detected.
						case 9:  // NetworkNotSupported = 9: Network is not supported.
						case 10: // NetworkNonWpa = 10: Network is not WPA2PSK, WPA2EAP or Open.
						{
							iRes = EapTlsResult_ConnectionError;
							EapTls_Log("FAILED connecting to network '%s' - Id[%d]: error=%d\n", networkName, networkID, networkDiagnostics.error);
						}
						break;

						default:
						{
							EapTls_Log("ERROR connecting to network '%s' - Id[%d]: error=%d\n", networkName, networkID, networkDiagnostics.error);
							if (!networkDiagnostics.isEnabled)
							{
								iRes = EapTlsResult_NetworkDisabled;
							}
							else
							{
								// This should never happen!
								iRes = EapTlsResult_BadParameters;
							}
						}
						break;
					}
				}
			}
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_SetTargetScanOnNetwork(const char *networkName, bool enabled)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != networkName)
	{
		int targetNetworkId = WifiConfig_GetNetworkIdByConfigName(networkName);
		if (-1 == targetNetworkId)
		{
			iRes = EapTlsResult_NetworkUnknown;
			EapTls_Log("Cannot find network configuration '%s': errno=%d (%s)\n", networkName, errno, strerror(errno));
		}
		else
		{
			// First, we want to leave the current connected network as not target-scanned
			int currentNetworkId = WifiConfig_GetConnectedNetworkId();
			if (-1 == currentNetworkId)
			{
				EapTls_Log("Failed retrieving current connected network Id: errno=%d (%s)\n", errno, strerror(errno));
			}
			else if (currentNetworkId != targetNetworkId)
			{
				EapTls_Log("Currently connected to network Id [%d]: disabling target scan on this network...\n", currentNetworkId);
				if (-1 == WifiConfig_SetTargetedScanEnabled(currentNetworkId, false))
				{
					EapTls_Log("Cannot reset targeted scan for network Id[%d]: errno=%d (%s)\n", currentNetworkId, errno, strerror(errno));
				}
			}

			EapTls_Log("Found target network configuration '%s' @ id=%d\n", networkName, targetNetworkId);
			if (-1 == WifiConfig_SetTargetedScanEnabled(targetNetworkId, enabled))
			{
				iRes = EapTlsResult_FailedScanningNetwork;
				EapTls_Log("Cannot set targeted scan for network configuration '%s': errno=%d (%s)\n", networkName, errno, strerror(errno));
			}
			else
			{
				// The setting is effective immediately but won't persist across device reboots unless the WifiConfig_PersistConfig function is called after this function.
				// NOTE: Targeted scanning causes the device to transmit probe requests that may reveal the SSID of the network to other devices.
				//       This should only be used in controlled environments, or on networks where this an acceptable risk.
				EapTls_Log("Successfully set target scanning for network configuration '%s' @ id=%d\n", networkName, targetNetworkId);

				int count = WifiConfig_TriggerScanAndGetScannedNetworkCount(); // blocking call
				if (-1 == count)
				{
					iRes = EapTlsResult_FailedScanningNetwork;
					EapTls_Log("Cannot trigger scan for network configuration '%s': errno=%d (%s)\n", networkName, errno, strerror(errno));
				}
				else
				{
					iRes = EapTlsResult_Success;
					EapTls_Log("Triggered scan and successfully found %d networks.\n", count);
				}
			}
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_IsNetworkIdConnected(int networkID)
{
	EapTlsResult iRes = EapTlsResult_Error;

	int connNetworkID = WifiConfig_GetConnectedNetworkId();
	if (-1 == connNetworkID)
	{
		//if (ENOTCONN == connNetworkID)
		{
			iRes = EapTlsResult_Disconnected;
		}

		EapTls_Log("Not connected to any network: errno=%d (%s)\n", errno, strerror(errno));
	}
	else
	{
		iRes = (connNetworkID == networkID) ? EapTlsResult_Connected : EapTlsResult_Disconnected;
	}

	return iRes;
}
EapTlsResult EapTls_IsNetworkConnected(const char *networkName)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != networkName)
	{
		int networkID = WifiConfig_GetNetworkIdByConfigName(networkName);
		if (-1 == networkID)
		{
			EapTls_Log("Cannot find network configuration '%s': errno=%d (%s)\n", networkName, errno, strerror(errno));
			iRes = EapTlsResult_NetworkUnknown;
		}
		else
		{
			iRes = EapTls_IsNetworkIdConnected(networkID);
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_DisableAllNetworksExcept(const char *networkName)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != networkName)
	{
		int targetNetworkId = WifiConfig_GetNetworkIdByConfigName(networkName);
		if (-1 == targetNetworkId)
		{
			EapTls_Log("Cannot find network configuration '%s': errno=%d (%s)\n", networkName, errno, strerror(errno));
			iRes = EapTlsResult_NetworkUnknown;
		}
		else
		{
			// #NOTE: At the moment, this is implemented with a workaround, since
			// the WifiConfig_StoredNetwork struct misses struct member to identify the related network.
			for (int networkId = 0; networkId < MAX_NETWORK_CONFIGURATIONS; networkId++)
			{
				int res = WifiConfig_SetNetworkEnabled(networkId, (networkId == targetNetworkId) ? true : false);
				if (-1 == res)
				{
					// #NOTE: disabled since it's a workaround
					//iRes = EapTlsResult_Error;
					EapTls_Log("Cannot %s network configuration ID[%d]: errno=%d (%s)\n", (networkId == targetNetworkId) ? "enable" : "disable", networkId, errno, strerror(errno));
				}
				else
				{
					iRes = EapTlsResult_Success;
					EapTls_Log("Successfully %s network configuration ID[%d]\n", (networkId == targetNetworkId) ? "enabled" : "disabled", networkId);
				}
			}
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_SetNetworkEnabledState(const char *networkName, bool enabled)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != networkName)
	{
		int targetNetworkId = WifiConfig_GetNetworkIdByConfigName(networkName);
		if (-1 == targetNetworkId)
		{
			EapTls_Log("Cannot find network configuration '%s': errno=%d (%s)\n", networkName, errno, strerror(errno));
			iRes = EapTlsResult_NetworkUnknown;
		}
		else
		{
			if (-1 == WifiConfig_SetNetworkEnabled(targetNetworkId, enabled))
			{
				EapTls_Log("Cannot %s network configuration '%s': errno=%d (%s)\n", enabled ? "enable" : "disable", errno, strerror(errno));
			}
			else
			{
				iRes = EapTlsResult_Success;
				EapTls_Log("Network configuration '%s' is now <%s>\n", networkName,  enabled ? "enabled" : "disabled");
			}
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_AddNetwork(const char *networkName, const char *networkSSID, WifiConfig_Security_Type securityType, const char *psk)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != networkName && NULL != networkSSID)
	{
		int networkID = WifiConfig_AddNetwork();
		if (-1 == networkID)
		{
			EapTls_Log("Cannot add a new network configuration: errno=%d (%s)\n", errno, strerror(errno));
		}
		else
		{
			EapTls_Log("Configuring a new '%s' network with security type [%d]...\n", networkName, securityType);
			if (-1 == WifiConfig_SetConfigName(networkID, networkName))
			{
				EapTls_Log("Cannot set eapTlsConfig name %s: errno=%d (%s)\n", networkSSID, errno, strerror(errno));
				return EapTlsResult_Error;
			}
			if (-1 == WifiConfig_SetSSID(networkID, networkSSID, strlen(networkSSID)))
			{
				EapTls_Log("Cannot set SSID %s: errno=%d (%s)\n", networkSSID, errno, strerror(errno));
				return EapTlsResult_Error;
			}
			if (-1 == WifiConfig_SetSecurityType(networkID, securityType))
			{
				EapTls_Log("Cannot set eapTlsConfig type to %d: errno=%d (%s)\n", securityType, errno, strerror(errno));
				return EapTlsResult_Error;
			}
			if (NULL != psk)
			{
				if (-1 == WifiConfig_SetPSK(networkID, psk, strnlen(psk, WIFICONFIG_WPA2_KEY_MAX_BUFFER_SIZE)))
				{
					EapTls_Log("Cannot set PSK: errno=%d (%s)\n", securityType, errno, strerror(errno));
					return EapTlsResult_Error;
				}
			}
			if (-1 == WifiConfig_SetNetworkEnabled(networkID, true))
			{
				EapTls_Log("Cannot enable network %s: errno=%d (%s)\n", networkSSID, errno, strerror(errno));
				return EapTlsResult_Error;
			}

			iRes = EapTls_PersistNetworkConfig();
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_RemoveNetwork(const char *networkName)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != networkName)
	{
		EapTls_Log("Looking for existing '%s' network configuration...\n", networkName);
		int srcNetworkID = WifiConfig_GetNetworkIdByConfigName(networkName);
		if (-1 == srcNetworkID)
		{
			EapTls_Log("Cannot find network configuration '%s': errno=%d (%s)\n", networkName, errno, strerror(errno));
		}
		else
		{
			EapTls_Log("Forgetting previous '%s' network...\n", networkName);
			if (-1 == WifiConfig_ForgetNetworkById(srcNetworkID))
			{
				EapTls_Log("Cannot forget eapTlsConfig: errno=%d (%s)\n", errno, strerror(errno));
			}
			else
			{
				iRes = EapTls_PersistNetworkConfig();
			}
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_CloneNetworkConfig(EapTlsConfig *srcNetwork, EapTlsConfig *dstNetwork)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != srcNetwork && NULL != dstNetwork)
	{
		int srcNetworkID = WifiConfig_GetNetworkIdByConfigName(srcNetwork->eapTlsNetworkName);
		if (-1 == srcNetworkID)
		{
			EapTls_Log("Cannot find network configuration '%s': errno=%d (%s)\n", srcNetwork->eapTlsNetworkName, errno, strerror(errno));
		}
		else
		{
			int newNetworkID = WifiConfig_AddDuplicateNetwork(srcNetworkID, dstNetwork->eapTlsNetworkName);
			if (-1 == newNetworkID)
			{
				EapTls_Log("Cannot duplicate eapTlsConfig name '%s': errno=%d (%s)\n", dstNetwork->eapTlsNetworkName, errno, strerror(errno));
			}
			else
			{
				if (-1 == WifiConfig_SetSSID(newNetworkID, dstNetwork->eapTlsNetworkSsid, strlen(dstNetwork->eapTlsNetworkSsid)))
				{
					EapTls_Log("Cannot set SSID %s: errno=%d (%s)\n", dstNetwork->eapTlsNetworkName, errno, strerror(errno));
				}
				else
				{
					return EapTlsResult_Success;
				}
			}
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_ConfigureNetworkSecurity(const char *networkName, const char *identity, const char *rootCACertificateId, const char *clientCertificateId)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != networkName && NULL != identity && NULL != rootCACertificateId && NULL != clientCertificateId)
	{
		int networkID = WifiConfig_GetNetworkIdByConfigName(networkName);
		if (-1 == networkID)
		{
			EapTls_Log("Cannot find network configuration '%s': errno=%d (%s)\n", networkName, errno, strerror(errno));
		}
		else
		{
			EapTls_Log("Configuring security for the '%s' EAP-TLS network...\n", networkName);

			if (WifiConfig_SetClientIdentity(networkID, identity) == -1)
			{
				EapTls_Log("Cannot set client identity %s: errno=%d (%s)\n", networkName, errno, strerror(errno));
			}
			else if (WifiConfig_SetRootCACertStoreIdentifier(networkID, rootCACertificateId) == -1)
			{
				EapTls_Log("Cannot set RootCA %s: errno=%d (%s)\n", rootCACertificateId, errno, strerror(errno));
			}
			else if (WifiConfig_SetClientCertStoreIdentifier(networkID, clientCertificateId) == -1)
			{
				EapTls_Log("Cannot set client certificate to network %s: errno=%d (%s)\n", clientCertificateId, errno, strerror(errno));
			}
			else if (EapTls_PersistNetworkConfig() == EapTlsResult_Error)
			{
				EapTls_Log("Cannot persist eapTlsConfig: errno=%d\n", iRes);
			}
			else
			{
				EapTls_Log("Successfully configured '%s' EAP-TLS network!\n", networkName);
				iRes = EapTlsResult_Success;
			}
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_WaitToConnectTo(const char *networkName)
{
	EapTlsResult iRes = EapTlsResult_Error, retries = MAX_CONNECTION_RETRIES;
	const struct timespec sleepTime = { .tv_sec = 10, .tv_nsec = 0 };

	if (NULL != networkName)
	{
		while (retries--)
		{
			EapTls_Log("Connection attempt #%d...\n", MAX_CONNECTION_RETRIES - retries);
			iRes = EapTls_IsNetworkConnected(networkName);
			if (EapTlsResult_Connected == iRes)
			{
				EapTls_Log("Successfully connected to network '%s'\n", networkName);
				return iRes;
			}
			else if (EapTlsResult_NetworkUnknown == iRes)
			{
				return iRes;
			}

			nanosleep(&sleepTime, NULL);
		}

		iRes = EapTlsResult_ConnectionTimeout;
		EapTls_Log("TIMEOUT connecting to network '%s'\n", networkName);
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_ConnectToNetwork(const char *networkName)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != networkName)
	{
		// Let's set the device to connect to the specific network that was asked
		{
			// Ideally these calls should be used (but both implementing workarounds!):
			iRes = EapTls_DisableAllNetworksExcept(networkName);
			// or
			//iRes = EapTls_SetTargetScanOnNetwork(networkName, true);
		}
		if (EapTlsResult_Success == iRes)
		{
			// Let's kick-off the connection handler
			iRes = InitConnectionTimerHandler(networkName);
			if (EapTlsResult_Success == iRes)
			{
				iRes = EapTlsResult_Connecting;
				EapTls_Log("Initialized connection handler to network '%s'\n", networkName);

				// Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
				while (connectionTimerHandlerContext.exitCode == EapTlsResult_Connecting)
				{
					EventLoop_Run_Result result = EventLoop_Run(connectionTimerHandlerContext.eventLoop, -1, true);

					// Continue if interrupted by signal, e.g. due to breakpoint being set.
					if (result == EventLoop_Run_Failed && errno != EINTR)
					{
						iRes = EapTlsResult_Error;
						goto exit_timer;
					}

					iRes = (EapTlsResult)connectionTimerHandlerContext.exitCode;
					switch (iRes)
					{
						case EapTlsResult_Connecting:
						{
							EapTls_Log("Attempt #%d connecting to network '%s'... \n", MAX_CONNECTION_RETRIES - connectionTimerHandlerContext.connectionRetries, networkName);
						}
						break;

						case EapTlsResult_Connected:
						{
							EapTls_Log("CONNECTED to network '%s'!\n", networkName);
							goto exit_timer;
						}
						break;

						case EapTlsResult_ConnectionTimeout:
						{
							EapTls_Log("Timeout connecting to network '%s'\n", networkName);

							iRes = EapTls_DiagnoseNetwork(connectionTimerHandlerContext.connectionNetworkName);
							if (EapTlsResult_FailedDiagnosingNetwork == iRes)
							{
								// Let's tweak this result translating it to a timeout error
								iRes = EapTlsResult_ConnectionTimeout;
							}
							goto exit_timer;
						}
						break;

						default:
							EapTls_Log("EapTls_ConnectToNetwork UNHANDLED CASE!!!!\n", networkName);
							break;
					}

				}
			}
			else
			{
				iRes = EapTlsResult_Error;
				EapTls_Log("FAILED to initialize connection handler to network '%s'\n", networkName);
			}
		}
		else
		{
			EapTls_Log("FAILED to targeting connection to network '%s'\n", networkName);
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;

exit_timer:
	DisposeConnectionTimerHandler();
	return iRes;
}

//////////////////////////////////////////////////////////////
// Certificate store helpers
//////////////////////////////////////////////////////////////
EapTlsResult EapTls_CompareCertificates(const char *certificateId, const uint8_t *certificatePem, size_t certSize)
{
	EapTlsResult iRes = 0;

	if (NULL != certificateId && *certificateId && NULL != certificatePem && *certificatePem)
	{
#if USE_ADDITIONAL_API_STUBS
		// MISSING APIs!!
		CertStore_Fingerprint fp1, fp2;
		int res = CertStore_GetCertificateFingerprint(certificateId, &fp1);
		if (-1 != res)
		{
			res = CertStore_GetBlobCertificateFingerprint(certificatePem, certSize, &fp2);
			if (-1 != iRes)
			{
				iRes = memcmp(&fp1, &fp2, sizeof(CertStore_Fingerprint)) == 0 ? EapTlsResult_SUCCESS : EapTlsResult_ERROR;
			}
		}
#else
		// There is no released API to retrieve the fingerprint on a stored certificate and neither on a PEM blob, so we just say they are different...
		// NOTE!! this uses flash write-cycles to (re)write certificate!!
		iRes = EapTlsResult_Error;
#endif
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
		EapTls_Log("ERROR - Invalid certificate Id!\n");
	}

	return iRes;
}
EapTlsResult EapTls_ValidateCertificates(const char *certificateId, const char *expectedSubject, const char *expectedIssuer)
{
	EapTlsResult iRes = EapTlsResult_Error;
	struct tm tmNotBefore = { 0 }, tmNotAfter = { 0 };
	CertStore_SubjectName outSubjectName;
	CertStore_IssuerName outIssuerName;

	if (NULL != certificateId && *certificateId)
	{
		if (CertStore_GetCertificateNotBefore(certificateId, &tmNotBefore) == 0)
		{
			if (CertStore_GetCertificateNotAfter(certificateId, &tmNotAfter) == 0)
			{
				if (CertStore_GetCertificateSubjectName(certificateId, &outSubjectName) == 0)
				{
					if (CertStore_GetCertificateIssuerName(certificateId, &outIssuerName) == 0)
					{
						time_t now = time(0);
						time_t tNotBefore = mktime(&tmNotBefore);
						time_t tNotAfter = mktime(&tmNotAfter);

						//
						// #NOTE: validate dates, etc. as per your custom requirements
						//		
						if (difftime(now, tNotAfter) > 0) // If positive, then now > t_not_after 
						{
							EapTls_Log("Certificate '%s' is expired!\n", certificateId);
						}
						else if (difftime(tNotBefore, now) > 0)
						{
							EapTls_Log("Certificate '%s' is not yet valid!\n", certificateId);
						}
						else if (NULL != expectedSubject && 0 != strcmp(outSubjectName.name, expectedSubject))
						{
							EapTls_Log("Certificate '%s' doesn't have the expected Subject (%s/%s)!\n", outSubjectName.name, expectedSubject);
						}
						else if (NULL != expectedIssuer && 0 != strcmp(outIssuerName.name, expectedIssuer))
						{
							EapTls_Log("Certificate '%s' doesn't have the expected Issuer (%s/%s)!\n", outIssuerName.name, expectedIssuer);
						}
						else
						{
							iRes = EapTlsResult_Success;
							EapTls_Log("Certificate '%s' is valid.\n", certificateId);
						}
					}
					else
					{
						EapTls_Log("ERROR retrieving the IssuerName for certificate Id '%s': errno=%d (%s)\n", certificateId, errno, strerror(errno));
					}
				}
				else
				{
					EapTls_Log("ERROR retrieving the SubjectName for certificate Id '%s': errno=%d (%s)\n", certificateId, errno, strerror(errno));
				}
			}
			else
			{
				EapTls_Log("ERROR retrieving the 'Not After' date for certificate Id '%s': errno=%d (%s)\n", certificateId, errno, strerror(errno));
			}
		}
		else
		{
			EapTls_Log("ERROR retrieving the 'Not Before' date for certificate Id '%s': errno=%d (%s)\n", certificateId, errno, strerror(errno));
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
		EapTls_Log("ERROR - Invalid certificate Id!\n");
	}

	return iRes;
}
EapTlsResult EapTls_IsCertificateInstalled(const char *certificateId)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != certificateId && *certificateId)
	{
		int certCount = CertStore_GetCertificateCount();
		if (-1 == certCount)
		{
			EapTls_Log("ERROR counting certificates in the CertStore: errno=%d (%s)\n", errno, strerror(errno));
		}
		else
		{
			for (int index = 0; index < certCount; index++)
			{
				CertStore_Identifier id;
				if (-1 == CertStore_GetCertificateIdentifierAt((size_t)index, &id))
				{
					EapTls_Log("ERROR finding certificate '%s' in the CertStore: errno=%d (%s)\n", certificateId, errno, strerror(errno));
				}
				else
				{
					if (0 == strncmp(id.identifier, certificateId, strlen(id.identifier)))
					{
						EapTls_Log("Certificate '%s' is installed in the CertStore\n", certificateId);

						return EapTlsResult_Success;
					}
				}
			}
			EapTls_Log("Certificate '%s' is NOT installed in the CertStore\n", certificateId);
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
		EapTls_Log("ERROR - Invalid certificate Id!\n");
	}

	return iRes;
}
EapTlsResult EapTls_CheckcertStoreFreeSpace(size_t certificateSize)
{
	EapTlsResult iRes = EapTlsResult_Success;

	ssize_t availableSpace = CertStore_GetAvailableSpace();
	if (-1 == availableSpace)
	{
		iRes = EapTlsResult_Error;
		EapTls_Log("ERROR: CertStore_GetAvailableSpace has failed: errno=%d (%s)\n", errno, strerror(errno));
	}
	else if (((size_t)availableSpace) < certificateSize)
	{
		iRes = EapTlsResult_Error;
		EapTls_Log("ERROR: Available space (%zu) is less than the required space: (%zu).\n", availableSpace, certificateSize);
	}

	return iRes;
}
EapTlsResult EapTls_InstallRootCaCertificatePem(const char *certificateId, const char *certificatePem)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != certificateId &&
		NULL != certificateId && *certificateId &&
		NULL != certificatePem && *certificatePem)
	{
		size_t cert_len = strlen(certificatePem);
		if (EapTls_CheckcertStoreFreeSpace(cert_len) == EapTlsResult_Success)
		{
			if (CertStore_InstallRootCACertificate(certificateId, certificatePem, cert_len) == 0)
			{
				iRes = EapTlsResult_Success;
				EapTls_Log("Successfully installed '%s' rootCA certificate\n", certificateId);
			}
			else
			{
				EapTls_Log("Error installing the CA certificate: errno=%d (%s)\n", errno, strerror(errno));
			}
		}
		else
		{
			iRes = EapTlsResult_CertStoreFull;
			EapTls_Log("Error: not enough space left in the CertStore\n");
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
		EapTls_Log("Invalid parameters!\n");
	}

	return iRes;
}
EapTlsResult EapTls_InstallRootCaCertificate(Certificate *certificate)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != certificate && *certificate->relativePath)
	{

		EapTls_Log("Looking for rootCA certificate @ '%s'...\n", certificate->relativePath);
		int fd = Storage_OpenFileInImagePackage(certificate->relativePath);
		if (-1 != fd)
		{
			char *certificateBuffer = malloc(CERTSTORE_MAX_CERT_SIZE);
			if (NULL != certificateBuffer)
			{
				if (read(fd, certificateBuffer, CERTSTORE_MAX_CERT_SIZE) >= 0)
				{
					// Install the RootCA certificate
					iRes = EapTls_InstallRootCaCertificatePem(certificate->id, certificateBuffer);
				}

				free(certificateBuffer);
			}
			else
			{
				iRes = EapTlsResult_OutOfMemory;
				Log_Debug("Out of memory in loading rootCA certificate @ '%s'...\n", certificate->relativePath);
			}

			close(fd);
		}
		else
		{
			EapTls_Log("The certificate file could not be read: errno=%d (%s)\n", errno, strerror(errno));
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
		EapTls_Log("Invalid parameters!\n");
	}

	return iRes;
}
EapTlsResult EapTls_InstallClientCertificatePem(const char *certificateId, const char *certificatePem, const char *privateKeyPem, const char *privateKeyPassword)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != certificateId && *certificateId &&
		NULL != certificatePem && *certificatePem)
	{
		size_t cert_len = strlen(certificatePem), pk_len = strlen(privateKeyPem);

		if (EapTls_CheckcertStoreFreeSpace(cert_len + pk_len) == EapTlsResult_Success)
		{
			// Install the Client certificate
			if (CertStore_InstallClientCertificate(certificateId, certificatePem, cert_len, privateKeyPem, pk_len, privateKeyPassword) == 0)
			{
				iRes = EapTlsResult_Success;
				EapTls_Log("Successfully installed '%s' client certificate\n", certificateId);
			}
			else
			{
				EapTls_Log("Error installing the client certificate: errno=%d (%s)\n", errno, strerror(errno));
			}
		}
		else
		{
			iRes = EapTlsResult_CertStoreFull;
			EapTls_Log("Error: not enough space left in the CertStore\n");
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
		EapTls_Log("Invalid parameters!\n");
	}

	return iRes;
}
EapTlsResult EapTls_InstallClientCertificate(Certificate *certificate)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != certificate &&
		*certificate->id &&
		*certificate->relativePath &&
		*certificate->privateKeyRelativePath)
	{
		EapTls_Log("Looking for Client certificate @ '%s'...\n", certificate->relativePath);
		int fd = Storage_OpenFileInImagePackage(certificate->relativePath);
		if (-1 != fd)
		{
			char *certificateBuffer = malloc(CERTSTORE_MAX_CERT_SIZE);
			if (NULL != certificateBuffer)
			{
				if (read(fd, certificateBuffer, CERTSTORE_MAX_CERT_SIZE) >= 0)
				{
					EapTls_Log("Looking for private key @ '%s'...\n", certificate->privateKeyRelativePath);
					int fdpk = Storage_OpenFileInImagePackage(certificate->privateKeyRelativePath);
					if (-1 != fd)
					{
						char *privateKeyBuffer = malloc(CERTSTORE_MAX_CERT_SIZE);
						if (NULL != privateKeyBuffer)
						{
							if (read(fdpk, privateKeyBuffer, CERTSTORE_MAX_CERT_SIZE) >= 0)
							{
								// Install the Client certificate
								iRes = EapTls_InstallClientCertificatePem(certificate->id, certificateBuffer, privateKeyBuffer, certificate->privateKeyPass);
							}
							else
							{
								EapTls_Log("The private key file could not be read: errno=%d (%s)\n", errno, strerror(errno));
							}

							free(privateKeyBuffer);
						}
						else
						{
							iRes = EapTlsResult_OutOfMemory;
							Log_Debug("Out of memory in loading client certificate @ '%s'...\n", certificate->privateKeyRelativePath);
						}

						close(fdpk);
					}
					else
					{
						EapTls_Log("The certificate's private key file could not be read: errno=%d (%s)\n", errno, strerror(errno));
					}

					close(fd);
				}
				else
				{
					EapTls_Log("The certificate file could not be read: errno=%d (%s)\n", errno, strerror(errno));
				}
				free(certificateBuffer);
			}
			else
			{
				iRes = EapTlsResult_OutOfMemory;
				Log_Debug("Out of memory in loading rooclienttCA certificate @ '%s'...\n", certificate->relativePath);
			}
		}
	}
	else
	{
		iRes = EapTlsResult_BadParameters;
		EapTls_Log("Invalid parameters!\n");
	}

	return iRes;
}

//////////////////////////////////////////////////////////////
// EAP-TLS network management
//////////////////////////////////////////////////////////////
EapTlsResult EapTls_SetBootstrapNetworkInterfaceType(EapTlsConfig *eapTlsConfig, NetworkInterfaceType networkInterfaceType)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != eapTlsConfig)
	{
		switch (networkInterfaceType)
		{
			case NetworkInterfaceType_Wifi:
			{
				eapTlsConfig->bootstrapNetworkInterfaceType = NetworkInterfaceType_Wifi;
				int res = Networking_SetInterfaceState(NET_INTERFACE_WLAN0, true);
				if (-1 == res)
				{
					EapTls_Log("Error setting interface state to '%s': %d(%s)\n", NET_INTERFACE_WLAN0, errno, strerror(errno));
				}
				else
				{
					iRes = EapTlsResult_Success;
					EapTls_Log("Bootstrap network is set to '%s'.\n", NET_INTERFACE_WLAN0);

					// If the bootstrap network is on Wi-Fi, then disable the Ethernet interface
					res = Networking_SetInterfaceState(NET_INTERFACE_ETHERNET0, false);
					if (-1 == res)
					{
						EapTls_Log("Error disabling '%s': %d(%s)\n", NET_INTERFACE_ETHERNET0, errno, strerror(errno));
					}
				}
			}
			break;

			case NetworkInterfaceType_Ethernet:
			{
				eapTlsConfig->bootstrapNetworkInterfaceType = NetworkInterfaceType_Ethernet;
				int res = Networking_SetInterfaceState(NET_INTERFACE_ETHERNET0, true);
				if (-1 == res)
				{
					EapTls_Log("Error setting interface state to '%s': %d(%s)\n", NET_INTERFACE_ETHERNET0, errno, strerror(errno));
				}
				else
				{
					iRes = EapTlsResult_Success;
					EapTls_Log("Bootstrap network is set to '%s'.\n", NET_INTERFACE_ETHERNET0);
				}
			}
			break;

			default:
			{
				eapTlsConfig->bootstrapNetworkInterfaceType = NetworkInterfaceType_Undefined;
				EapTls_Log("ERROR: unknown interface type [%d]!\n", networkInterfaceType);
			}
			break;
		}
	}
	else
	{
		EapTls_Log("ERROR: EAP-TLS configuration is NULL!");
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_SetBootstrapNetworkEnabledState(EapTlsConfig* eapTlsConfig, bool enabled)
{
	EapTlsResult iRes = EapTlsResult_Error;

	if (NULL != eapTlsConfig)
	{
		if (NetworkInterfaceType_Wifi == eapTlsConfig->bootstrapNetworkInterfaceType)
		{
			// On Wi-Fi, we just "disable" the bootstrap configuration, not the whole interface,
			// as that would prevent connecting to the RADIUS network over Wi-Fi
			iRes = EapTls_SetNetworkEnabledState(eapTlsConfig->bootstrapNetworkName, enabled);
			if (EapTlsResult_Success == iRes)
			{
				EapTls_Log("Bootstrap network on '%s' is now <%s>\n", NET_INTERFACE_WLAN0, enabled ? "enabled" : "disabled");
			}
		}
		else
		{
			// On Wi-Fi, we disable the whole interface, as this would not prevent connecting to the RADIUS network over Wi-Fi
			int err = Networking_SetInterfaceState(NET_INTERFACE_ETHERNET0, enabled);
			if (err == -1)
			{
				Log_Debug("Error setting interface state of '%s': %d(%s)\n", NET_INTERFACE_ETHERNET0, errno, strerror(errno));
			}
			else
			{
				iRes = EapTlsResult_Success;
				EapTls_Log("Bootstrap network on '%s' is now <%s>\n", NET_INTERFACE_ETHERNET0, enabled ? "enabled" : "disabled");
			}
		}
	}
	else
	{
		EapTls_Log("ERROR: EAP-TLS configuration is NULL!");
		iRes = EapTlsResult_BadParameters;
	}

	return iRes;
}
EapTlsResult EapTls_RunConnectionManager(EapTlsConfig *eapTlsConfig)
{
	EapTlsResult iRes = EapTlsResult_Error;

	// Global variables (to state-machine)
	EapTlsConfig radiusNetwork;
	EapTlsConfig radiusNetwork_dup;
	bool requestRootCaCertificate;
	bool requestClientCertificate;

	// Variable for handling the EAP-TLS network configuration, including its duplicate (used when renewing certificates with the WebAPI).
	bool duplicatingNetwork = false;

	// Temporary storage for the RootCA & Client certificates returned from the WebAPI (use Stack or static depending on SRAM/Flash constraints)
	MemoryBlock webApiResponseBlob = { .data = NULL, .size = 0 };
	WebApiResponse *webApiResponse = NULL;

	// Check basic parameter requirements
	{
		if (NULL == eapTlsConfig)
		{
			EapTls_Log("Invalid configuration pointer.\n");
			return EapTlsResult_BadParameters;
		}
		if (NetworkInterfaceType_Undefined == eapTlsConfig->bootstrapNetworkInterfaceType)
		{
			EapTls_Log("ERROR: EapTlsConfig::bootstrapNetworkInterfaceType is undefined!\n");
			return EapTlsResult_BadParameters;
		}
		if (NULL == eapTlsConfig->bootstrapNetworkName || 0 == eapTlsConfig->bootstrapNetworkName)
		{
			EapTls_Log("ERROR: EapTlsConfig::bootstrapNetworkName is NULL or empty!\n");
			return EapTlsResult_BadParameters;
		}
		if (NULL == eapTlsConfig->bootstrapNetworkSsid || 0 == eapTlsConfig->bootstrapNetworkSsid)
		{
			EapTls_Log("ERROR: EapTlsConfig::bootstrapNetworkSsid is NULL or empty!\n");
			return EapTlsResult_BadParameters;
		}
		if (NULL == eapTlsConfig->mdmWebApiInterfaceUrl || 0 == eapTlsConfig->mdmWebApiInterfaceUrl)
		{
			EapTls_Log("ERROR: EapTlsConfig::mdmWebApiInterfaceUrl is NULL or empty!\n");
			return EapTlsResult_BadParameters;
		}
		if (NULL == eapTlsConfig->eapTlsRootCertificate.id || 0 == eapTlsConfig->eapTlsRootCertificate.id)
		{
			EapTls_Log("ERROR: EapTlsConfig::eapTlsRootCertificate.id is NULL or empty!\n");
			return EapTlsResult_BadParameters;
		}
		if (NULL == eapTlsConfig->eapTlsClientCertificate.id || 0 == eapTlsConfig->eapTlsClientCertificate.id)
		{
			EapTls_Log("ERROR: EapTlsConfig::eapTlsClientCertificate.id is NULL or empty!\n");
			return EapTlsResult_BadParameters;
		}
		if (NULL == eapTlsConfig->eapTlsClientCertificate.privateKeyPass || 0 == eapTlsConfig->eapTlsClientCertificate.privateKeyPass)
		{
			EapTls_Log("ERROR: EapTlsConfig::eapTlsClientCertificate.privateKeyPass is NULL or empty!\n");
			return EapTlsResult_BadParameters;
		}
	}

	// Clone the configurations to the library's statics
	memcpy(&radiusNetwork, eapTlsConfig, sizeof(EapTlsConfig));
	memcpy(&radiusNetwork_dup, eapTlsConfig, sizeof(EapTlsConfig));

	bool exitStateMachine = false;
	ConnectionManagerState currentState = ConnectionManagerState_Idle;
	while (!exitStateMachine)
	{
		switch (currentState)
		{
			case ConnectionManagerState_TBD: // Just a dead-end, to capture incomplete branch coding
			{
				EapTls_Log("Stuck into EAP_TLS_TBD... see call-stack!!\n");
				while (1);
			}
			break;

			case ConnectionManagerState_Idle: // Just starting off: let's check if we have RootCA and Client certificates
			{
				EapTls_Log("EapTls_RunConnectionManager::ConnectionManagerState_Idle\n");
				currentState = ConnectionManagerState_CheckCertsInstalled;
			}
			break;

			case ConnectionManagerState_CheckCertsInstalled: // Check if we have installed RootCA and Client certificates
			{
				EapTls_Log("EapTls_RunConnectionManager::ConnectionManagerState_CheckCertsInstalled\n");

				requestRootCaCertificate = (EapTlsResult_Success != EapTls_IsCertificateInstalled(radiusNetwork.eapTlsRootCertificate.id));
				requestClientCertificate = (EapTlsResult_Success != EapTls_IsCertificateInstalled(radiusNetwork.eapTlsClientCertificate.id));

				currentState = (requestRootCaCertificate || requestClientCertificate) ? ConnectionManagerState_RequestCertificates : ConnectionManagerState_ConnectToEapTlsNetwork;
			}
			break;

			case ConnectionManagerState_AddEapTlsNetwork: // Add the EAP_TLS network from scratch (eventually removes existing)
			{
				EapTls_Log("EapTls_RunConnectionManager::ConnectionManagerState_AddEapTlsNetwork\n");

				// Always (eventually) remove the EAP_TLS network (no return/error check required, logging already within the API)
				EapTls_RemoveNetwork(radiusNetwork.eapTlsNetworkName);

				// Let's check if we can immediately connect to the EAP_TLS server
				iRes = EapTls_AddNetwork(radiusNetwork.eapTlsNetworkName, radiusNetwork.eapTlsNetworkSsid, WifiConfig_Security_Wpa2_EAP_TLS, NULL);
				if (EapTlsResult_Success == iRes)
				{
					EapTls_Log("Successfully added new EAP-TLS network '%s'!\n", radiusNetwork.eapTlsNetworkName);

					currentState = ConnectionManagerState_ConfigureEapTlsNetwork;
					continue;
				}

				// We cannot add the network --> let's give back control to the app to decide
				iRes = EapTlsResult_FailedAddingEapTlsNetwork;
				currentState = ConnectionManagerState_Error_Exit;
			}
			break;

			case ConnectionManagerState_CloneEapTlsNetwork: // Clone the EAP_TLS network, for use in certificate renewal
			{
				EapTls_Log("EapTls_RunConnectionManager::ConnectionManagerState_CloneEapTlsNetwork\n");

				// Let's duplicate the configuration structures, except for the network name and certificate IDs
				radiusNetwork_dup = radiusNetwork;
				snprintf(radiusNetwork_dup.eapTlsNetworkName, sizeof(radiusNetwork_dup.eapTlsNetworkName) - 1, "_%s", radiusNetwork.eapTlsNetworkName);

				// Always (eventually) remove the old attempts of duplication of the EAP_TLS network (no return/error check required, logging already within the API)
				EapTls_RemoveNetwork(radiusNetwork_dup.eapTlsNetworkName);

				iRes = EapTls_CloneNetworkConfig(&radiusNetwork, &radiusNetwork_dup);
				if (EapTlsResult_Success != iRes)
				{
					EapTls_Log("Cannot create temporary network configuration for '%s'\n", radiusNetwork.eapTlsNetworkName);

					// We cannot attempt duplicating the network to try a different authentication path --> let's give back control to the app to decide
					iRes = EapTlsResult_FailedCloningEapTlsNetwork;
					currentState = ConnectionManagerState_Error_Exit;
				}
				else
				{
					// Let's install the certs only once we are sure we'll have a network configuration to attach them to
					currentState = ConnectionManagerState_Installcerts_Dup;
				}
			}
			break;

			// Optimizing similar state handling
			case ConnectionManagerState_Installcerts: // Install RootCA and Client certificates
			case ConnectionManagerState_Installcerts_Dup: // Install new RootCA and Client certificates, to be registered into the EAP_TLS's <TEMPORARY CLONE> network
			{
				EapTls_Log("EapTls_RunConnectionManager::%s\n", duplicatingNetwork ? "ConnectionManagerState_Installcerts_Dup" : "ConnectionManagerState_Installcerts");

				if (NULL != webApiResponse)
				{
					EapTlsConfig *network = duplicatingNetwork ? &radiusNetwork_dup : &radiusNetwork;

					// Install the RootCA certificate, if we asked for one and if it's different
					if (requestRootCaCertificate)
					{
						if (0 != EapTls_CompareCertificates((const char*)&network->eapTlsRootCertificate, webApiResponse->rootCACertficate, strlen((const char*)&network->eapTlsRootCertificate)))
						{
							iRes = EapTls_InstallRootCaCertificatePem((const char*)&network->eapTlsRootCertificate.id, webApiResponse->rootCACertficate);
							if (EapTlsResult_Success != iRes)
							{
								iRes = EapTlsResult_FailedInstallingCertificates;
								currentState = ConnectionManagerState_Error_Exit;
								continue;
							}
						}						
					}					

					// Install the Client certificate, if we asked for one and if it's different
					if (requestClientCertificate)
					{
						if (0 != EapTls_CompareCertificates((const char*)&network->eapTlsClientCertificate, webApiResponse->clientPublicCertificate, strlen((const char*)&network->eapTlsClientCertificate)))
						{
							// NOTE: it left to the customer to decide wither to use the private key password that "may" be returned from the WebAPI
							// or a password that is hard-coded into the code (and that could be updated with future App updates)
							iRes = EapTls_InstallClientCertificatePem((const char*)&network->eapTlsClientCertificate.id, 
								webApiResponse->clientPublicCertificate,
								webApiResponse->clientPrivateKey,
#ifdef USE_CLIENT_CERT_PRIVATE_KEY_PASS_FROM_WEBAPI
								webApiResponse->clientPrivateKeyPass);
#else
								network->eapTlsClientCertificate.privateKeyPass);
#endif // USE_CLIENT_CERT_PRIVATE_KEY_PASS_FROM_WEBAPI
								

							if (EapTlsResult_Success != iRes)
							{
								iRes = EapTlsResult_FailedInstallingCertificates;
								currentState = ConnectionManagerState_Error_Exit;
								continue;
							}
						}
					}
				}
				else
				{
					iRes = EapTlsResult_FailedInstallingCertificates;
					currentState = ConnectionManagerState_Error_Exit;

					EapTls_Log("Error: null WebAPI response\n");
					continue;
				}

				// We now have the new certs installed, let's attempt to add the EAP-TLS network, or configure the duplicated one if we're coming from that path
				currentState = duplicatingNetwork ? ConnectionManagerState_ConfigureEapTlsNetwork_Dup : ConnectionManagerState_AddEapTlsNetwork;
			}
			break;

			// Optimizing similar state handling
			case ConnectionManagerState_ConfigureEapTlsNetwork: // Configure the EAP_TLS's network security (installing CA/Client certs)
			case ConnectionManagerState_ConfigureEapTlsNetwork_Dup: // Configure the EAP_TLS's <TEMPORARY CLONE> network security (installing CA/Client certs)
			{
				EapTls_Log("EapTls_RunConnectionManager::%s\n", duplicatingNetwork ? "ConnectionManagerState_ConfigureEapTlsNetwork_Dup" : "ConnectionManagerState_ConfigureEapTlsNetwork");

				EapTlsConfig *network = duplicatingNetwork ? &radiusNetwork_dup : &radiusNetwork;

				if (EapTls_ConfigureNetworkSecurity(network->eapTlsNetworkName, network->eapTlsClientIdentity, network->eapTlsRootCertificate.id, network->eapTlsClientCertificate.id) != EapTlsResult_Success)
				{
					EapTls_Log("Cannot configure network security for '%s'\n", network->eapTlsNetworkName);

					// Certificates from the WebAPI are not valid --> return to the App
					iRes = EapTlsResult_FailedConfiguringCertificates;
					currentState = ConnectionManagerState_Error_Exit;
				}
				else
				{
					// Network is configured: let's connect
					currentState = duplicatingNetwork ? ConnectionManagerState_ConnectToEapTlsNetwork_Dup : ConnectionManagerState_ConnectToEapTlsNetwork;			
				}
			}
			break;

			case ConnectionManagerState_ConnectToEapTlsNetwork: // Attempt connecting to the EAP_TLS network
			{
				EapTls_Log("EapTls_RunConnectionManager::ConnectionManagerState_ConnectToEapTlsNetwork\n");

				duplicatingNetwork = false; // reset the connection-attempt state
				switch ((iRes = EapTls_ConnectToNetwork(radiusNetwork.eapTlsNetworkName)))
				{
					case EapTlsResult_Connecting:
					{
						// We just stay in this state until something happens, ultimately a timeout
					}
					break;

					case EapTlsResult_Connected:
					{
						currentState = ConnectionManagerState_Connected_Exit;

						EapTls_Log("Successfully connected to EAP-TLS network '%s'\n", radiusNetwork.eapTlsNetworkName);
					}
					break;

					case EapTlsResult_FailedTargetingNetwork:
					case EapTlsResult_FailedScanningNetwork:
					{
						// Timeout forcing a connection to the EAP-TLS network --> let's give back control to the app to decide
						currentState = ConnectionManagerState_Error_Exit;

						EapTls_Log("Failed targeting EAP-TLS network '%s' --> exiting\n", radiusNetwork.eapTlsNetworkName);
					}
					break;

					case EapTlsResult_NetworkUnknown:
					{
						// The network is unknown --> this means that we might have a configuration issue (i.e. device relocated somewhere else)
						// Let's contact the Web API for a full re-provisioning						
						requestRootCaCertificate = true;
						requestClientCertificate = true;
						currentState = ConnectionManagerState_RequestCertificates;

						EapTls_Log("Unknown EAP-TLS network '%s' --> re-provisioning the device.\n", radiusNetwork.eapTlsNetworkName);
					}
					break;

					case EapTlsResult_ConnectionError:
					{
						// Generic error connecting to the network --> let's give back control to the app to decide
						currentState = ConnectionManagerState_Error_Exit;

						EapTls_Log("Connection error to EAP-TLS network '%s' --> exiting\n", radiusNetwork.eapTlsNetworkName);
					}
					break;

					case EapTlsResult_AuthenticationError:
					{
						requestRootCaCertificate = true;
						requestClientCertificate = true;

						// We cannot connect to the network because of authentication failure --> let's request new certificates and test them on a temporary duplicated network
						duplicatingNetwork = true;
						currentState = ConnectionManagerState_RequestCertificates;

						EapTls_Log("Error [%d] authenticating to EAP-TLS network '%s' --> requesting new certificates for a duplicated network configuration.\n", iRes, radiusNetwork.eapTlsNetworkName);
					}
					break;
					case EapTlsResult_AuthenticationError_InvalidRootCaCert:
					{
						requestRootCaCertificate = true;
						requestClientCertificate = false;

						// We cannot connect to the network for authentication failure --> let's request new certificates and test them on a temporary duplicated network
						duplicatingNetwork = true;
						currentState = ConnectionManagerState_RequestCertificates;

						EapTls_Log("Error [%d] authenticating to EAP-TLS network '%s' --> requesting new certificates for a duplicated network configuration.\n", iRes, radiusNetwork.eapTlsNetworkName);
					}
					break;
					case EapTlsResult_AuthenticationError_InvalidClientCert:
					case EapTlsResult_AuthenticationError_InvalidClientIdentity:
					{
						requestRootCaCertificate = false;
						requestClientCertificate = true;

						// We cannot connect to the network for authentication failure --> let's request new certificates and test them on a temporary duplicated network
						duplicatingNetwork = true;
						currentState = ConnectionManagerState_RequestCertificates;

						EapTls_Log("Error [%d] authenticating to EAP-TLS network '%s' --> requesting new certificates for a duplicated network configuration.\n", iRes, radiusNetwork.eapTlsNetworkName);
					}
					break;

					case EapTlsResult_NetworkDisabled:
					{
						// For some reason, the network is disabled despite the target-scan EapTls_ConnectToNetwork. This may be due to other user threads concurrency --> let's give back control to the app to decide
						//iRes = EapTlsResult_NetworkDisabled;
						currentState = ConnectionManagerState_Error_Exit;

						EapTls_Log("Error handling connection to EAP-TLS network '%s' --> exiting\n", radiusNetwork.eapTlsNetworkName);
					}
					break;

					case EapTlsResult_ConnectionTimeout:
					{
						// Timeout connecting to the network for technical reasons (as no diagnostics are available) --> let's give back control to the app to decide
						//iRes = EapTlsResult_ConnectionTimeout;
						currentState = ConnectionManagerState_Error_Exit;

						EapTls_Log("Timeout connecting to EAP-TLS network '%s' --> exiting\n", radiusNetwork.eapTlsNetworkName);
					}
					break;

					case EapTlsResult_FailedDiagnosingNetwork:
					{
						// For some reason, the network is disabled despite the target-scan EapTls_ConnectToNetwork. This may be due to other user threads concurrency --> let's give back control to the app to decide
						//iRes = EapTlsResult_FailedDiagnosingNetwork;
						currentState = ConnectionManagerState_Error_Exit;

						EapTls_Log("Error handling connection to EAP-TLS network '%s' --> exiting\n", radiusNetwork.eapTlsNetworkName);
					}
					break;

					case EapTlsResult_Error:
					{
						// Failed initializing connection handler --> let's give back control to the app to decide
						//iRes = EapTlsResult_Error;
						currentState = ConnectionManagerState_Error_Exit;

						EapTls_Log("Failed initializing connection handler to EAP-TLS network '%s' --> exiting\n", radiusNetwork.eapTlsNetworkName);
					}
					break;

					default:
					{
						// DEV ERROR: should never get here, block execution to check call-stack.
						currentState = ConnectionManagerState_TBD;
					}
					break;
				}
			}
			break;

			case ConnectionManagerState_ConnectToEapTlsNetwork_Dup: // Attempt connecting to the EAP_TLS's <TEMPORARY CLONE> network
			{
				EapTls_Log("EapTls_RunConnectionManager::ConnectionManagerState_ConnectToEapTlsNetwork_Dup\n");
				switch ((iRes = EapTls_ConnectToNetwork(radiusNetwork_dup.eapTlsNetworkName)))
				{
					case EapTlsResult_Connecting:
					{
						// We just stay in this state until something happens, ultimately a timeout
					}
					break;

					case EapTlsResult_Connected:
					{
						// Successfully connected to the new EAP_TLS network --> let's swap it with the original one
						currentState = ConnectionManagerState_SwapEapTlsNetworks;

						EapTls_Log("Successfully connected to the NEW EAP-TLS network '%s'\n", radiusNetwork_dup.eapTlsNetworkName);
					}
					break;

					default:
					{
						// Any other error on this second attempt (on the duplicated network), is a total fail. Therefore the state-machine is reset back to the App's control.
						// Failed connecting to the duplicated network --> let's give back control to the app to decide, with the last error in iRes
						currentState = ConnectionManagerState_Error_Exit;

						EapTls_Log("Failed connecting to NEW EAP-TLS network '%s' --> exiting\n", radiusNetwork_dup.eapTlsNetworkName);
					}
					break;
				}
			}
			break;

			case ConnectionManagerState_SwapEapTlsNetworks: // Swap the original EAP_TLS network with the EAP_TLS's <TEMPORARY CLONE> network
			{
				EapTls_Log("EapTls_RunConnectionManager::ConnectionManagerState_SwapEapTlsNetworks\n");

				// Delete the original network configuration
				int networkId = WifiConfig_GetNetworkIdByConfigName(radiusNetwork.eapTlsNetworkName);
				if (-1 == networkId)
				{
					iRes = EapTlsResult_FailedSwappingEapTlsNetworkConfig;
					currentState = ConnectionManagerState_Error_Exit;
					EapTls_Log("Error looking-up network '%s' - Id[%d]: errno=%d (%s) --> exiting\n", radiusNetwork.eapTlsNetworkName, networkId, errno, strerror(errno));
				}
				else
				{
					// Delete the original network configuration
					if (-1 == WifiConfig_ForgetNetworkById(networkId))
					{
						iRes = EapTlsResult_FailedSwappingEapTlsNetworkConfig;
						currentState = ConnectionManagerState_Error_Exit;
						EapTls_Log("Error forgetting network '%s' - Id[%d]: errno=%d (%s) --> exiting\n", radiusNetwork.eapTlsNetworkName, networkId, errno, strerror(errno));
					}
					else
					{
						// Rename the temporary network with the original network configuration name
						networkId = WifiConfig_GetNetworkIdByConfigName(radiusNetwork_dup.eapTlsNetworkName);
						if (-1 == networkId)
						{
							iRes = EapTlsResult_FailedSwappingEapTlsNetworkConfig;
							currentState = ConnectionManagerState_Error_Exit;
							EapTls_Log("Error looking-up network '%s' - Id[%d]: errno=%d (%s) --> exiting\n", radiusNetwork_dup.eapTlsNetworkName, networkId, errno, strerror(errno));
						}
						else
						{
							// Rename the duplicated network to become the first configuration					
							int res = WifiConfig_SetConfigName(networkId, radiusNetwork.eapTlsNetworkName);
							if (-1 == res)
							{
								iRes = EapTlsResult_FailedSwappingEapTlsNetworkConfig;
								currentState = ConnectionManagerState_Error_Exit;
								EapTls_Log("Error renaming network '%s' - Id[%d]: errno=%d (%s) --> exiting\n", radiusNetwork_dup.eapTlsNetworkName, networkId, errno, strerror(errno));
							}
							else
							{
								if (EapTls_PersistNetworkConfig() == EapTlsResult_Success)
								{
									// Rename the NEW root & client certificates
									res = CertStore_MoveCertificate(radiusNetwork_dup.eapTlsRootCertificate.id, radiusNetwork.eapTlsRootCertificate.id);
									if (-1 == res)
									{
										iRes = EapTlsResult_FailedSwappingEapTlsNetworkConfig;
										currentState = ConnectionManagerState_Error_Exit;
										EapTls_Log("Error renaming RootCA certificate '%s': errno=%d (%s) --> exiting\n", radiusNetwork_dup.eapTlsClientCertificate.id, errno, strerror(errno));
									}
									else
									{
										res = CertStore_MoveCertificate(radiusNetwork_dup.eapTlsClientCertificate.id, radiusNetwork.eapTlsClientCertificate.id);
										if (-1 == res)
										{
											iRes = EapTlsResult_FailedSwappingEapTlsNetworkConfig;
											currentState = ConnectionManagerState_Error_Exit;
											EapTls_Log("Error renaming Client certificate '%s': errno=%d (%s) --> exiting\n", radiusNetwork_dup.eapTlsClientCertificate.id, errno, strerror(errno));
										}
										else
										{
											iRes = EapTlsResult_Connected;
											currentState = ConnectionManagerState_Connected_Exit;
											EapTls_Log("Successfully configured new '%s' EAP-TLS network!\n", radiusNetwork.eapTlsNetworkName);
										}
									}
								}
								else
								{
									iRes = EapTlsResult_FailedSwappingEapTlsNetworkConfig;
									currentState = ConnectionManagerState_Error_Exit;
									EapTls_Log("Cannot persist network configuration: errno=%d (%s) --> exiting\n", errno, strerror(errno));
								}
							}
						}
					}
				}
			}
			break;

			case ConnectionManagerState_ConnectToBootstrapNetwork: // Attempt connecting to a bootstrap network, in order to connect to the WebAPI and retrieve new certificates
			{
				EapTls_Log("EapTls_RunConnectionManager::ConnectionManagerState_ConnectToBootstrapNetwork\n");

				iRes = EapTls_SetBootstrapNetworkEnabledState(eapTlsConfig, true);
				if (EapTlsResult_Success == iRes)
				{
					if (NetworkInterfaceType_Ethernet == radiusNetwork.bootstrapNetworkInterfaceType)
					{
						// We're on Ethernet: let's directly call the WebAPI
						EapTls_Log("Bootstrap network is on Ethernet: directly calling the WebAPI...\n");
						iRes = EapTlsResult_Connected;
						currentState = ConnectionManagerState_CallMdmWebApi;
					}
					else
					{
						EapTls_Log("Bootstrap network is on Wi-Fi: attempting to connect...\n");
						switch ((iRes = EapTls_ConnectToNetwork(radiusNetwork.bootstrapNetworkName)))
						{
							case EapTlsResult_Connecting:
							{
								// We just stay in this state
							}
							break;

							case EapTlsResult_Connected:
							{
								// Let's call the WebAPI
								currentState = ConnectionManagerState_CallMdmWebApi;
							}
							break;

							default:
							{
								// We cannot connect to the Bootstrap network --> let's give back control to the App to decide what to do next
								iRes = EapTlsResult_FailedConnectingToBootstrapNetwork;
								currentState = ConnectionManagerState_Error_Exit;
							}
							break;
						}
					}
				}
				else
				{
					// We cannot connect enable the Bootstrap network --> let's give back control to the App to decide what to do next
					iRes = EapTlsResult_FailedConnectingToBootstrapNetwork;
					currentState = ConnectionManagerState_Error_Exit;
				}

			}
			break;

			case ConnectionManagerState_RequestCertificates: // Initiate a new Client certificate request
			{
				EapTls_Log("EapTls_RunConnectionManager::ConnectionManagerState_RequestCertificates\n");

				currentState = ConnectionManagerState_ConnectToBootstrapNetwork;
			}
			break;

			case ConnectionManagerState_CallMdmWebApi: // Request a new Client certificate to the WebAPI/CMS
			{
				EapTls_Log("EapTls_RunConnectionManager::ConnectionManagerState_CallMdmWebApi\n");

				iRes = EapTls_CallMdmWebApi(&radiusNetwork, requestRootCaCertificate, requestClientCertificate, &webApiResponseBlob);
				if (EapTlsResult_Success == iRes)
				{
					// The WebAPI has successfully returned a response --> let's move to parsing
					currentState = ConnectionManagerState_HandleMdmWebApiResponse;
				}
				else
				{
					iRes = EapTlsResult_FailedConnectingToMdmWebApi;
					currentState = ConnectionManagerState_Error_Exit;
					EapTls_Log("Failed calling WebAPI '%s' --> exiting\n", radiusNetwork.mdmWebApiInterfaceUrl);
				}
			}
			break;

			case ConnectionManagerState_HandleMdmWebApiResponse: // Handle the response from the WebAPI/CMS
			{
				EapTls_Log("EapTls_RunConnectionManager::ConnectionManagerState_HandleMdmWebApiResponse\n");

				// The WebAPI "should" have returned new certificates in the webApiResponseBlock MemoryBlock
				// The response will be a JSON in the following format:
				// {
				//		"timestamp" : "2020-05-22T10:302:25.6828914+00:00", 
				//		"rootCACertficate" : "<certificate in PEM format>", 
				//		"eapTlsNetworkSsid" : "<the SId of the RADIUS network", 
				//		"clientIdentity" : "<the client user identity>"
				//		"clientPublicCertificate" : "<certificate in PEM format>"
				//		"clientPrivateKey" : "<client's private in PEM format>"
				//		"clientPrivateKeyPass" : "<private key password>"
				// }

				free(webApiResponse);
				webApiResponse = malloc(sizeof(WebApiResponse));
				if (NULL != webApiResponse)
				{
					// Parse the WebAPI response and extract the optional RootCA certificate and the Client certificate
					iRes = EapTls_ParseMdmWebApiResponse(&webApiResponseBlob, webApiResponse);
					if (EapTlsResult_Success == iRes)
					{
						// Check if the Web API has returned what we asked for
						if ((requestRootCaCertificate && 0 == *webApiResponse->rootCACertficate) || (requestClientCertificate && 0 == *webApiResponse->clientPublicCertificate))
						{
							iRes = EapTlsResult_FailedReceivingNewCertificates;

							EapTls_Log("Failed receiving the requested certificates from then WebAPI '%s' (RootCA-%s, ClientCert=%s) --> exiting\n",
								(requestRootCaCertificate && 0 == *webApiResponse->rootCACertficate) ? "OK" : "failed",
								(requestClientCertificate && 0 == *webApiResponse->clientPublicCertificate) ? "OK" : "failed",
								radiusNetwork.mdmWebApiInterfaceUrl);
						}
						else
						{
							// The WebAPI has returned new certificate(s) and stored them in the function-global webApiResponse variable				

							// Set and persist the assigned RADIUS network's SSID					
							strncpy(deviceConfiguration.eapTlsNetworkSsid, webApiResponse->eapTlsNetworkSsid, sizeof(deviceConfiguration.eapTlsNetworkSsid) - 1);
							if (EapTlsResult_Success == EapTls_StoreDeviceConfiguration(&deviceConfiguration))
							{
								strncpy(radiusNetwork.eapTlsNetworkSsid, deviceConfiguration.eapTlsNetworkSsid, sizeof(radiusNetwork.eapTlsNetworkSsid) - 1);
							}

							if (requestClientCertificate)
							{
								// The WebAPI has returned a new client certificate, so let's set & persist its related client's identity														
								strncpy(deviceConfiguration.eapTlsClientIdentity, webApiResponse->clientIdentity, sizeof(deviceConfiguration.eapTlsClientIdentity) - 1);
								if (EapTlsResult_Success == EapTls_StoreDeviceConfiguration(&deviceConfiguration))
								{
									strncpy(radiusNetwork.eapTlsClientIdentity, deviceConfiguration.eapTlsClientIdentity, sizeof(radiusNetwork.eapTlsClientIdentity) - 1);
								}
							}

							// Now split paths: differentiate if we're coming from the first (failed) attempt, or if this is just the first EAP_TLS installation
							if (duplicatingNetwork)
							{
								// We first clone the network, then we install the certificates from that state
								currentState = ConnectionManagerState_CloneEapTlsNetwork;
							}
							else
							{
								currentState = ConnectionManagerState_Installcerts;
							}
						}
					}
					else
					{
						iRes = EapTlsResult_FailedParsingMdmWebApiResponse;
						currentState = ConnectionManagerState_Error_Exit;
						EapTls_Log("Failed parsing response from WebAPI '%s' --> exiting\n", radiusNetwork.mdmWebApiInterfaceUrl);
					}
				}
				else
				{
					iRes = EapTlsResult_FailedParsingMdmWebApiResponse;
					currentState = ConnectionManagerState_Error_Exit;
					EapTls_Log("Out of memory parsing response from WebAPI '%s' --> exiting\n", radiusNetwork.mdmWebApiInterfaceUrl);
				}
				free(webApiResponseBlob.data);
				webApiResponseBlob.data = NULL;
			}
			break;

			case ConnectionManagerState_Connected_Exit: // Successfully connected to the EAP-TLS network -> the App can proceed its execution
			{
				EapTls_Log("EapTls_RunConnectionManager::EAP_TLS_CONNECTED_EXIT - result=%d\n", iRes);
				exitStateMachine = true;
			}
			break;

			case ConnectionManagerState_Error_Exit: // FAILED to connect to the EAP-TLS network -> let's return so the App can decide the next steps
			{
				EapTls_Log("EapTls_RunConnectionManager::EAP_TLS_ERROR_EXIT - result=%d\n", iRes);
				exitStateMachine = true;
			}
			break;

			default:
				EapTls_Log("EapTls_RunConnectionManager::UNDEFINED STATE '%d'!\n", currentState);
				currentState = ConnectionManagerState_TBD;
				break;
		}
	}

	if (NULL != webApiResponse)
	{
		free(webApiResponse);
	}

	return iRes;
}