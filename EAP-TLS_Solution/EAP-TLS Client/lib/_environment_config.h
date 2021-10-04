#pragma once

//***************************************************************************************
// IMPORTANT!!!
// The "_environment_config.c" file is not included in the repo for security 
// policies (but is already pre-configured in CMakeLists.txt).
// Simply create an empty one int the project's root folder, containing all the below
// constant definitions. See the README.md for a full description.
//***************************************************************************************

/////////////////////////////////////
// WebAPI implementation selection //
/////////////////////////////////////
//#define WEBAPI_SERVER		// Use 3rd party MDM Server WebApi
#define WEBAPI_KESTREL	// Use the self-hosted WebApi on a Kestrel web-host
#if defined(WEBAPI_SERVER)
extern const char *const g_webApiInterfaceRegisterUrl;	// This is optional, leave blank if not present
#endif
extern const char *const g_webApiInterfaceUrl;
extern const char *const g_webApiRootCaCertificateId;
extern const char *const g_webApiRootCaCertificatePath;

/////////////////////////////////////
// Bootstrap network constants     //
/////////////////////////////////////
extern const char *const g_bootstrapNetworkName;
extern const char *const g_bootstrapNetworkSsid;
extern const char *const g_bootstrapNetworkPassword;

/////////////////////////////////////
// EAP-TLS network constants       //
/////////////////////////////////////
extern const char *const g_eapTlsNetworkName;
extern const char *const g_eapTlsNetworkSsid;
extern const char *const g_eapTlsRootCaCertificateId;
extern const char *const g_eapTlsRootCaCertificateRelativePath;
extern const char *const g_eapTlsClientIdentity;
extern const char *const g_eapTlsClientCertificateId;
extern const char *const g_eapTlsClientCertificateRelativePath;
extern const char *const g_eapTlsClientPrivateKeyRelativePath;
extern const char *const g_eapTlsClientPrivateKeyPassword;


/*
/////////////////////////////////////////////////
// TEMPLATE SAMPLE FOR "_environment_config.c" //
/////////////////////////////////////////////////


#include "_environment_config.h"

/////////////////////////////////////
// WebAPI implementation selection //
/////////////////////////////////////

#if defined(WEBAPI_SERVER)
const char *const g_webApiInterfaceRegisterUrl = "https://<your WebApi's registration endpoint>"; // This is optional, leave blank if not present
const char *const g_webApiInterfaceUrl = "https://<your WebApi's certificate request endpoint>";
#endif
#if defined(WEBAPI_KESTREL)
const char *const g_webApiInterfaceUrl = "https://<your local machine's hostname>:44378/api/certs?needRootCACertificate=true&needClientCertificate=true";
#endif

const char *const g_webApiRootCaCertificateId = "WebApiRootCA";
const char *const g_webApiRootCaCertificatePath = "certs/eap-tls-webapi.pem";;

/////////////////////////////////////
// Bootstrap network constants     //
/////////////////////////////////////
const char *const g_bootstrapNetworkName = "cfgBootstrap";
const char *const g_bootstrapNetworkSsid = "<your bootstrap network Wi-Fi SSID>";
const char* const g_bootstrapNetworkPassword = "<your bootstrap network Wi-Fi password>";

/////////////////////////////////////
// EAP-TLS network constants       //
/////////////////////////////////////
const char *const g_eapTlsNetworkName = "cfgRADIUS";
const char *const g_eapTlsNetworkSsid = "<your EAP-TLS network Wi-Fi SSID>"; // This is returned by the WebAPI and will be stored in Mutable storage, it is used for test initialization purposes only
const char *const g_eapTlsRootCaCertificateId = "RadiusRootCA";
const char *const g_eapTlsRootCaCertificateRelativePath = "certs/az-CA.pem";
const char *const g_eapTlsClientIdentity = "iotuser@azsphere.com";
const char *const g_eapTlsClientCertificateId = "RadiusClient";
const char *const g_eapTlsClientCertificateRelativePath = "certs/iotuser_public.pem";
const char *const g_eapTlsClientPrivateKeyRelativePath = "certs/iotuser_private.pem";
const char *const g_eapTlsClientPrivateKeyPassword = "<optional private key password";
*/