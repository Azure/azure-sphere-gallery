#!/usr/bin/env python3

import os

# Files needed for the MQTT project
open("MQTT-C_Client/src/HighLevelApp/Certs/mosquitto.org.crt", "w").close()
open("MQTT-C_Client/src/HighLevelApp/Certs/client.key", "w").close()
open("MQTT-C_Client/src/HighLevelApp/Certs/client.crt", "w").close()

# Files needed for EAP-TLS project
certs = [ "eap-tls-webapi.pem", "az-CA.pem", "iotuser_public.pem", "iotuser_private.pem", "extuser_public.pem", "extuser_private.pem" ]

if not os.path.exists("EAP-TLS_Solution/EAP-TLS Client/certs"):
    os.mkdir("EAP-TLS_Solution/EAP-TLS Client/certs")

for c in certs:
    open(f"EAP-TLS_Solution/EAP-TLS Client/certs/{c}", "w").close()

environment_config_c = '''
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
'''

with open("EAP-TLS_Solution/EAP-TLS Client/lib/_environment_config.c", "w") as f:
    f.write(environment_config_c)