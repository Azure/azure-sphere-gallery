#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "../lib/_environment_config.h"
#include "../lib/eap_tls_lib.h"
#include "../lib/web_api_client.h"
#include "tests.h"


// Specific error-testing constants
const char *const g_eapTlsInvalidRootCaCertificateRelativePath = "certs/bad-az-CA.pem";
const char *const g_eapTlsInvalidClientIdentity = "extuser@azsphere.com";
const char *const g_eapTlsInvalidClientCertificateRelativePath = "certs/extuser_public.pem";
const char *const g_eapTlsInvalidClientPrivateKeyRelativePath = "certs/extuser_private.pem";


void TestEapTlsLib_LogHeader(const char *title)
{
	const char *const header_decoration = "************************************************************************************************************";

	size_t len = strlen(title) + 4;
	Log_Debug("%.*s\n", len, header_decoration);
	Log_Debug("* %s *\n", title);
	Log_Debug("%.*s\n", len, header_decoration);
}
void TestEapTlsLib_TestResult(EapTlsResult testRes)
{
	Log_Debug((EapTlsResult_Success == testRes) ? "*** Test Case PASS ***\n" : "*** Test Case FAILED!!! ***\n");
}

EapTlsResult TestEapTlsLib_InitializeConfiguration(EapTlsConfig *eapTlsConfig)
{
	Log_Debug("Initializing test EAP-TLS configuration...\n");

	// Full-reset the configuration
	memset(eapTlsConfig, 0, sizeof(EapTlsConfig));

	// Define the BOOTSTRAP network	
	strncpy(eapTlsConfig->bootstrapNetworkName, g_bootstrapNetworkName, sizeof(eapTlsConfig->bootstrapNetworkName) - 1);
	strncpy(eapTlsConfig->bootstrapNetworkSsid, g_bootstrapNetworkSsid, sizeof(eapTlsConfig->bootstrapNetworkSsid) - 1);

	// Define the WebAPI Server certificate
	strncpy(eapTlsConfig->mdmWebApiInterfaceUrl, g_webApiInterfaceUrl, sizeof(eapTlsConfig->mdmWebApiInterfaceUrl) - 1);
	strncpy(eapTlsConfig->mdmWebApiRootCertificate.id, g_webApiRootCaCertificateId, sizeof(eapTlsConfig->mdmWebApiRootCertificate.id) - 1);
	strncpy(eapTlsConfig->mdmWebApiRootCertificate.relativePath, g_webApiRootCaCertificatePath, sizeof(eapTlsConfig->mdmWebApiRootCertificate.relativePath) - 1);

	// Read or define the RADIUS network
	strncpy(eapTlsConfig->eapTlsNetworkName, g_eapTlsNetworkName, sizeof(eapTlsConfig->eapTlsNetworkName) - 1);
	if (EapTlsResult_Success == EapTls_ReadDeviceConfiguration(&deviceConfiguration))
	{
		strncpy(eapTlsConfig->eapTlsNetworkSsid, deviceConfiguration.eapTlsNetworkSsid, sizeof(eapTlsConfig->eapTlsNetworkSsid) - 1);
		strncpy(eapTlsConfig->eapTlsClientIdentity, deviceConfiguration.eapTlsClientIdentity, sizeof(eapTlsConfig->eapTlsClientIdentity) - 1);
	}
	else
	{
		// Let's set default values and store them
		strncpy(deviceConfiguration.eapTlsNetworkSsid, g_eapTlsNetworkSsid, sizeof(deviceConfiguration.eapTlsNetworkSsid) - 1);
		strncpy(deviceConfiguration.eapTlsClientIdentity, g_eapTlsClientIdentity, sizeof(deviceConfiguration.eapTlsClientIdentity) - 1);
		if (EapTlsResult_Success == EapTls_StoreDeviceConfiguration(&deviceConfiguration))
		{
			strncpy(eapTlsConfig->eapTlsNetworkSsid, deviceConfiguration.eapTlsNetworkSsid, sizeof(eapTlsConfig->eapTlsNetworkSsid) - 1);
			strncpy(eapTlsConfig->eapTlsClientIdentity, deviceConfiguration.eapTlsClientIdentity, sizeof(eapTlsConfig->eapTlsClientIdentity) - 1);
		}
	}

	// Define the RADIUS RootCA certificate
	strncpy(eapTlsConfig->eapTlsRootCertificate.id, g_eapTlsRootCaCertificateId, sizeof(eapTlsConfig->eapTlsRootCertificate.id) - 1);
	strncpy(eapTlsConfig->eapTlsRootCertificate.relativePath, g_eapTlsRootCaCertificateRelativePath, sizeof(eapTlsConfig->eapTlsRootCertificate.relativePath) - 1);

	// Define the RADIUS Client certificate
	strncpy(eapTlsConfig->eapTlsClientCertificate.id, g_eapTlsClientCertificateId, sizeof(eapTlsConfig->eapTlsClientCertificate.id) - 1);
	strncpy(eapTlsConfig->eapTlsClientCertificate.relativePath, g_eapTlsClientCertificateRelativePath, sizeof(eapTlsConfig->eapTlsClientCertificate.relativePath) - 1);
	strncpy(eapTlsConfig->eapTlsClientCertificate.privateKeyRelativePath, g_eapTlsClientPrivateKeyRelativePath, sizeof(eapTlsConfig->eapTlsClientCertificate.privateKeyRelativePath) - 1);
	strncpy(eapTlsConfig->eapTlsClientCertificate.privateKeyPass, g_eapTlsClientPrivateKeyPassword, sizeof(eapTlsConfig->eapTlsClientCertificate.privateKeyPass) - 1);

	return EapTlsResult_Success;
}
EapTlsResult TestEapTlsLib_ResetEnvironment(EapTlsConfig *eapTlsConfig)
{
	EapTlsResult iRes = EapTlsResult_Error;

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

		// Provision the "must have" Bootstrap network
		if (NetworkInterfaceType_Wifi == eapTlsConfig->bootstrapNetworkInterfaceType)
		{
			iRes = EapTls_AddNetwork(eapTlsConfig->bootstrapNetworkName, eapTlsConfig->bootstrapNetworkSsid, WifiConfig_Security_Wpa2_Psk, g_bootstrapNetworkPassword);
			if (EapTlsResult_Success != iRes)
			{
				return iRes;
			}
		}

		// Let's explicitly enable the bootstrap network, so all test cases not using EapTls_RunConnectionManager will work as well
		iRes = EapTls_SetBootstrapNetworkEnabledState(eapTlsConfig, true);
		if (EapTlsResult_Success != iRes)
		{
			return iRes;
		}
	}

	// Let's wait for the network interface to connect to the internet
	if (NetworkInterfaceType_Undefined == eapTlsConfig->bootstrapNetworkInterfaceType)
	{
		Log_Debug("ERROR: EapTlsConfig::bootstrapNetworkInterfaceType is undefined!\n");
	}
	else
	{
		Log_Debug("Waiting for the network interface to connect to the internet...\n");
		for (int maxRetries = 10; maxRetries > 0; maxRetries--)
		{
			//bool isNetworkReady = false;
			Networking_InterfaceConnectionStatus interfaceStatus = 0;

			//if (-1 == Networking_IsNetworkingReady(&isNetworkReady))
			//{
			//	Log_Debug("FAILED Networking_IsNetworkingReady: errno=%d (%s)\n", errno, strerror(errno));
			//}

			if (-1 == Networking_GetInterfaceConnectionStatus((NetworkInterfaceType_Wifi == eapTlsConfig->bootstrapNetworkInterfaceType) ? NET_INTERFACE_WLAN0 : NET_INTERFACE_ETHERNET0, &interfaceStatus))
			{
				Log_Debug("FAILED Networking_GetInterfaceConnectionStatus: errno=%d (%s)\n", errno, strerror(errno));
			}
			else
			{
				if (Networking_InterfaceConnectionStatus_ConnectedToInternet & interfaceStatus)
				{
					iRes = EapTlsResult_Success;
					break;
				}
			}

			static const struct timespec twoSeconds = { .tv_sec = 2, .tv_nsec = 0 };
			nanosleep(&twoSeconds, NULL);
			if (0 == maxRetries)
			{
				Log_Debug("FAILED connecting to the internet!!\n");
			}
		}
	}

	return iRes;
}

///////////////////////
///// TEST CASES //////
///////////////////////
EapTlsResult TestEapTlsLib_ProvisionWithEmbeddedCerts(EapTlsConfig *eapTlsConfig)
{
	EapTlsResult testRes = EapTlsResult_Error, iRes;

	TestEapTlsLib_LogHeader("Testing basic connection to EAP-TLS network, with certs embedded in the App image...");
	iRes = TestEapTlsLib_ResetEnvironment(eapTlsConfig);
	if (EapTlsResult_Success == iRes)
	{
		// When using the Connection Manager, the EAP-TLs's SSID and Client Identity are returned by the WebAPI, so here we set these manually
		strncpy(eapTlsConfig->eapTlsNetworkSsid, g_eapTlsNetworkSsid, sizeof(eapTlsConfig->eapTlsNetworkSsid) - 1);
		strncpy(eapTlsConfig->eapTlsClientIdentity, g_eapTlsClientIdentity, sizeof(eapTlsConfig->eapTlsClientIdentity) - 1);

		iRes = EapTls_InstallRootCaCertificate(&eapTlsConfig->eapTlsRootCertificate);
		if (EapTlsResult_Success == iRes)
		{
			iRes = EapTls_InstallClientCertificate(&eapTlsConfig->eapTlsClientCertificate);
			if (EapTlsResult_Success == iRes)
			{
				iRes = EapTls_AddNetwork(eapTlsConfig->eapTlsNetworkName, eapTlsConfig->eapTlsNetworkSsid, WifiConfig_Security_Wpa2_EAP_TLS, NULL);
				if (EapTlsResult_Success == iRes)
				{
					iRes = EapTls_ConfigureNetworkSecurity(eapTlsConfig->eapTlsNetworkName, eapTlsConfig->eapTlsClientIdentity, eapTlsConfig->eapTlsRootCertificate.id, eapTlsConfig->eapTlsClientCertificate.id);
					if (EapTlsResult_Success == iRes)
					{
						iRes = EapTls_SetBootstrapNetworkEnabledState(eapTlsConfig, false);
						if (EapTlsResult_Success == iRes)
						{
							if (EapTlsResult_Success == iRes)
							{
								iRes = EapTls_WaitToConnectTo(eapTlsConfig->eapTlsNetworkName);
								if (EapTlsResult_Connected == iRes)
								{
									testRes = EapTlsResult_Success;
								}
							}
						}
					}
				}
			}
		}
	}

	TestEapTlsLib_TestResult(testRes);

	return testRes;
}
EapTlsResult TestEapTlsLib_ProvisionWithMdmWebApi(EapTlsConfig *eapTlsConfig)
{
	EapTlsResult testRes = EapTlsResult_Error, iRes;

	TestEapTlsLib_LogHeader("Testing automatic connection to EAP-TLS network with the webAPI (not using embedded certs)...");
	iRes = TestEapTlsLib_ResetEnvironment(eapTlsConfig);
	if (EapTlsResult_Success == iRes)
	{
		iRes = EapTls_SetBootstrapNetworkEnabledState(eapTlsConfig, true);
		if (EapTlsResult_Success == iRes)
		{
			if (EapTlsResult_Success == iRes)
			{
				MemoryBlock webApiResponseBlob = { .data = NULL, .size = 0 };
				iRes = EapTls_CallMdmWebApi(eapTlsConfig, true, true, &webApiResponseBlob);
				if (EapTlsResult_Success == iRes)
				{
					WebApiResponse response;
					iRes = EapTls_ParseMdmWebApiResponse(&webApiResponseBlob, &response);
					if (EapTlsResult_Success == iRes)
					{
						strncpy(eapTlsConfig->eapTlsNetworkSsid, response.eapTlsNetworkSsid, sizeof(eapTlsConfig->eapTlsNetworkSsid) - 1);
						iRes = EapTls_InstallRootCaCertificatePem(eapTlsConfig->eapTlsRootCertificate.id, response.rootCACertficate);
						if (EapTlsResult_Success == iRes)
						{
							iRes = EapTls_InstallClientCertificatePem(eapTlsConfig->eapTlsClientCertificate.id, response.clientPublicCertificate, response.clientPrivateKey, eapTlsConfig->eapTlsClientCertificate.privateKeyPass);
							if (EapTlsResult_Success == iRes)
							{
								iRes = EapTls_AddNetwork(eapTlsConfig->eapTlsNetworkName, eapTlsConfig->eapTlsNetworkSsid, WifiConfig_Security_Wpa2_EAP_TLS, NULL);
								if (EapTlsResult_Success == iRes)
								{
									iRes = EapTls_ConfigureNetworkSecurity(eapTlsConfig->eapTlsNetworkName, eapTlsConfig->eapTlsClientIdentity, eapTlsConfig->eapTlsRootCertificate.id, eapTlsConfig->eapTlsClientCertificate.id);
									if (EapTlsResult_Success == iRes)
									{
										iRes = EapTls_SetBootstrapNetworkEnabledState(eapTlsConfig, false);
										if (EapTlsResult_Success == iRes)
										{
											iRes = EapTls_WaitToConnectTo(eapTlsConfig->eapTlsNetworkName);
											if (EapTlsResult_Connected == iRes)
											{
												testRes = EapTlsResult_Success;
											}
										}
									}
								}
							}
						}
					}
				}
				free(webApiResponseBlob.data);
			}
		}
	}

	TestEapTlsLib_TestResult(testRes);

	return testRes;
}
EapTlsResult TestEapTlsLib_AutoProvision_ZeroTouch(EapTlsConfig *eapTlsConfig)
{
	EapTlsResult testRes = EapTlsResult_Error, iRes;

	TestEapTlsLib_LogHeader("Testing the full client connection state machine, with zero-touch provisioning...");

	iRes = TestEapTlsLib_ResetEnvironment(eapTlsConfig);
	if (EapTlsResult_Success == iRes)
	{
		iRes = EapTls_RunConnectionManager(eapTlsConfig);
		if (EapTlsResult_Connected == iRes)
		{
			testRes = EapTlsResult_Success;
		}
	}

	TestEapTlsLib_TestResult(testRes);

	return testRes;
}
EapTlsResult TestEapTlsLib_AutoProvision_InvalidRootCa(EapTlsConfig *eapTlsConfig)
{
	EapTlsResult testRes = EapTlsResult_Error, iRes;

	TestEapTlsLib_LogHeader("Testing the full client connection state machine with an invalid RootCA certificate...");

	// Set up an invalid rootCA certificate, which would cause an Authentication failure
	Log_Debug("Registering an invalid rootCA certificate...\n");
	TestEapTlsLib_InitializeConfiguration(eapTlsConfig);
	strncpy(eapTlsConfig->eapTlsRootCertificate.relativePath, g_eapTlsInvalidRootCaCertificateRelativePath, sizeof(eapTlsConfig->eapTlsRootCertificate.relativePath) - 1);

	iRes = EapTls_InstallRootCaCertificate(&eapTlsConfig->eapTlsRootCertificate);
	if (EapTlsResult_Success == iRes)
	{
		iRes = EapTls_InstallClientCertificate(&eapTlsConfig->eapTlsClientCertificate);
		if (EapTlsResult_Success == iRes)
		{
			iRes = EapTls_ConfigureNetworkSecurity(eapTlsConfig->eapTlsNetworkName, eapTlsConfig->eapTlsClientIdentity, eapTlsConfig->eapTlsRootCertificate.id, eapTlsConfig->eapTlsClientCertificate.id);
			if (EapTlsResult_Success == iRes)
			{
				iRes = EapTls_RunConnectionManager(eapTlsConfig);
				if (EapTlsResult_Connected == iRes)
				{
					testRes = EapTlsResult_Success;
				}
			}
		}
	}

	TestEapTlsLib_TestResult(testRes);

	return testRes;
}
EapTlsResult TestEapTlsLib_AutoProvision_InvalidClientId(EapTlsConfig *eapTlsConfig)
{
	EapTlsResult testRes = EapTlsResult_Error, iRes;

	TestEapTlsLib_LogHeader("Testing the full client connection state machine with an invalid client identity...");

	// Set up an invalid client identity, which would cause an Authentication failure
	Log_Debug("Assigning invalid client identity...\n");
	TestEapTlsLib_InitializeConfiguration(eapTlsConfig);
	strncpy(eapTlsConfig->eapTlsClientIdentity, g_eapTlsInvalidClientIdentity, sizeof(eapTlsConfig->eapTlsClientIdentity) - 1);

	iRes = EapTls_InstallClientCertificate(&eapTlsConfig->eapTlsClientCertificate);
	if (EapTlsResult_Success == iRes)
	{
		iRes = EapTls_ConfigureNetworkSecurity(eapTlsConfig->eapTlsNetworkName, eapTlsConfig->eapTlsClientIdentity, eapTlsConfig->eapTlsRootCertificate.id, eapTlsConfig->eapTlsClientCertificate.id);
		if (EapTlsResult_Success == iRes)
		{
			iRes = EapTls_RunConnectionManager(eapTlsConfig);
			if (EapTlsResult_Connected == iRes)
			{
				testRes = EapTlsResult_Success;
			}
		}
	}

	TestEapTlsLib_TestResult(testRes);

	return testRes;
}
EapTlsResult TestEapTlsLib_AutoProvision_InvalidClientCert(EapTlsConfig *eapTlsConfig)
{
	EapTlsResult testRes = EapTlsResult_Error, iRes;

	TestEapTlsLib_LogHeader("Testing the full client connection state machine with an invalid client certificate...");

	// Set up an invalid client certificate, which would cause an Authentication failure
	Log_Debug("Registering an invalid client certificate...\n");
	TestEapTlsLib_InitializeConfiguration(eapTlsConfig);
	strncpy(eapTlsConfig->eapTlsClientCertificate.relativePath, g_eapTlsInvalidClientCertificateRelativePath, sizeof(eapTlsConfig->eapTlsClientCertificate.relativePath) - 1);
	strncpy(eapTlsConfig->eapTlsClientCertificate.privateKeyRelativePath, g_eapTlsInvalidClientPrivateKeyRelativePath, sizeof(eapTlsConfig->eapTlsClientCertificate.privateKeyRelativePath) - 1);

	iRes = EapTls_InstallRootCaCertificate(&eapTlsConfig->eapTlsRootCertificate);
	if (EapTlsResult_Success == iRes)
	{
		iRes = EapTls_InstallClientCertificate(&eapTlsConfig->eapTlsClientCertificate);
		if (EapTlsResult_Success == iRes)
		{
			iRes = EapTls_ConfigureNetworkSecurity(eapTlsConfig->eapTlsNetworkName, eapTlsConfig->eapTlsClientIdentity, eapTlsConfig->eapTlsRootCertificate.id, eapTlsConfig->eapTlsClientCertificate.id);
			if (EapTlsResult_Success == iRes)
			{
				iRes = EapTls_RunConnectionManager(eapTlsConfig);
				if (EapTlsResult_Connected == iRes)
				{
					testRes = EapTlsResult_Success;
				}
			}
		}
	}

	TestEapTlsLib_TestResult(testRes);

	return testRes;
}

///////////////////////
///// TEST SUITE //////
///////////////////////
EapTlsResult TestEapTlsLib_All(EapTlsConfig *eapTlsConfig, NetworkInterfaceType bootstrapNetworkInterfaceType)
{
	EapTlsResult testRes = EapTlsResult_Error;

	TestEapTlsLib_LogHeader("Starting up TestEapTlsLib_All");

	// Initialize the configuration with the predefined test-data
	TestEapTlsLib_InitializeConfiguration(eapTlsConfig);

	// Setup the requested network interface for the bootstrap network
	EapTls_SetBootstrapNetworkInterfaceType(eapTlsConfig, bootstrapNetworkInterfaceType);

	// Let's list all the available networks, just as a status report
	Log_Debug("Listing all the visible/available Wi-Fi networks:");
	ssize_t total = WifiConfig_TriggerScanAndGetScannedNetworkCount();
	if (-1 == total)
	{
		Log_Debug("FAILED scanning for networks: errno=%d (%s)\n", errno, strerror(errno));
	}
	else
	{
		WifiConfig_ScannedNetwork scannedList[MAX_NETWORK_CONFIGURATIONS];
		memset(scannedList, 0, sizeof(scannedList));
		int total = WifiConfig_GetScannedNetworks(scannedList, MAX_NETWORK_CONFIGURATIONS);
		while (total >= 0)
		{
			Log_Debug("Found network [%d] ssid[%s] securityType[%s]\n", total, scannedList[total].ssid,
				scannedList[total].security == WifiConfig_Security_Open ?
				"Open" :
				scannedList[total].security == WifiConfig_Security_Wpa2_Psk ?
				"WPA2-PSK" :
				scannedList[total].security == WifiConfig_Security_Wpa2_EAP_TLS ?
				"WPA2-EAP-TLS" : "Unknown"
			);
			total--;
		}
	}

	// Test connecting to the EAP-TLS network with pre-provisioned certs
	testRes = TestEapTlsLib_ProvisionWithEmbeddedCerts(eapTlsConfig);
	if (EapTlsResult_Success == testRes)
	{
		// Temporarily using the internal cert for registering with the MDM WebApi
		testRes = EapTls_InstallRootCaCertificate(&eapTlsConfig->mdmWebApiRootCertificate);
		if (EapTlsResult_Success == testRes)
		{
			testRes = EapTls_WebApiRegisterDevice(eapTlsConfig);
		}
		if (EapTlsResult_Success == testRes)
		{
			// Test automatic connection to EAP - TLS network with the webAPI (not using embedded certs)...
			testRes = TestEapTlsLib_ProvisionWithMdmWebApi(eapTlsConfig);
		}
		if (EapTlsResult_Success == testRes)
		{
			// Test the full client connection state machine (includes zero-touch provisioning)
			testRes = TestEapTlsLib_AutoProvision_ZeroTouch(eapTlsConfig);
		}
		if (EapTlsResult_Success == testRes)
		{
			// Test the full client connection state machine with an invalid rootCA certificate (therefore creating a temp duplicate network)
			testRes = TestEapTlsLib_AutoProvision_InvalidRootCa(eapTlsConfig);
		}
		if (EapTlsResult_Success == testRes)
		{
			// Test the full client connection state machine with an wrong client identity (therefore creating a temp duplicate network)
			testRes = TestEapTlsLib_AutoProvision_InvalidClientId(eapTlsConfig);
		}
		if (EapTlsResult_Success == testRes)
		{
			// Test the full client connection state machine with an invalid client certificate (therefore creating a temp duplicate network)
			testRes = TestEapTlsLib_AutoProvision_InvalidClientCert(eapTlsConfig);
		}
		if (EapTlsResult_Success == testRes)
		{
			testRes = EapTlsResult_Success;
		}
	}

	Log_Debug((EapTlsResult_Success == testRes) ? "*** Test Suite PASS ***\n" : "*** Test Suite FAILED!!! ***\n");

	// Clean-up the environment, so after the tests the device stays connected 
	// to the internet and AS3 (for updates/DAA renewal), otherwise not happening
	// i.e. overnight when connected to the RADIUS network.
	EapTls_SetBootstrapNetworkInterfaceType(eapTlsConfig, NetworkInterfaceType_Wifi);
	TestEapTlsLib_ResetEnvironment(eapTlsConfig);

	return testRes;
}