#pragma once
#include <time.h>
#include "../applibs_versions.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/wificonfig.h>
#include <applibs/storage.h>
#include <applibs/certstore.h>

/// <summary>
///     Client certificate's Private Key decryption method.
/// 
///     Configure if the library will use a hard-coded password
///     (in the EapTlsConfig object) for decrypting the Client
///     certificate's private key, or if it should use the one that could
///     be provided through the WebAPI.
/// </summary>
//#define USE_CLIENT_CERT_PRIVATE_KEY_PASS_FROM_WEBAPI

/// <summary>
///     Operational constants.
/// </summary>
#define MAX_CONNECTION_RETRIES        4  // This is the number of retries the library will attempt in connecting to a network
#define MAX_NETWORK_CONFIGURATIONS   10  // Max number network configurations that can be stored
#define MAX_URL_LEN                 255

/// <summary>
///     Network interface definitions.
/// </summary>
typedef enum
{
    NetworkInterfaceType_Undefined = 0,
    NetworkInterfaceType_Wifi = 1,
    NetworkInterfaceType_Ethernet = 2
} NetworkInterfaceType;
#define NET_INTERFACE_WLAN0         "wlan0"
#define NET_INTERFACE_ETHERNET0     "eth0"

/// <summary>
///     Return error codes for all EapTls_* APIs.
/// </summary>
typedef enum
{
    EapTlsResult_Success = 0,
    EapTlsResult_Error,
    EapTlsResult_BadParameters,
    EapTlsResult_CertStoreFull,
    EapTlsResult_OutOfMemory,
    EapTlsResult_Connecting,
    EapTlsResult_Connected,
    EapTlsResult_Disconnected,
    EapTlsResult_ConnectionError,
    EapTlsResult_ConnectionTimeout,
    EapTlsResult_AuthenticationError,
    EapTlsResult_AuthenticationError_InvalidRootCaCert,
    EapTlsResult_AuthenticationError_InvalidClientCert,
    EapTlsResult_AuthenticationError_InvalidClientIdentity,
    EapTlsResult_NetworkUnknown,
    EapTlsResult_NetworkDisabled,
    EapTlsResult_FailedTargetingNetwork,
    EapTlsResult_FailedScanningNetwork,
    EapTlsResult_FailedDiagnosingNetwork,
    EapTlsResult_FailedAddingEapTlsNetwork,
    EapTlsResult_FailedCloningEapTlsNetwork,
    EapTlsResult_FailedConnectingToBootstrapNetwork,
    EapTlsResult_FailedConnectingToEapTlsNetwork,
    EapTlsResult_FailedConnectingToEapTlsTmpNetwork,
    EapTlsResult_FailedConnectingToMdmWebApi,
    EapTlsResult_FailedParsingMdmWebApiResponse,
    EapTlsResult_FailedReceivingNewCertificates,
    EapTlsResult_FailedSwappingEapTlsNetworkConfig,
    EapTlsResult_FailedInstallingCertificates,
    EapTlsResult_FailedConfiguringCertificates,
    EapTlsResult_FailedInstallingNewCertificates,
    EapTlsResult_FailedConfiguringNewCertificates,

} EapTlsResult;

/// <summary>
///     Generic data pointer and size of a block of memory allocated on the heap.
/// </summary>
typedef struct {
    char *data;
    size_t size;
} MemoryBlock;

/// <summary>
///     Data structure for handling the device's permanent configuration.
/// </summary>
typedef struct
{
    char eapTlsNetworkSsid[WIFICONFIG_SSID_MAX_LENGTH + 1];
    char eapTlsClientIdentity[WIFICONFIG_EAP_IDENTITY_MAX_LENGTH + 1];

    // Here you can store other entities that may be part of a local
    // configuration that needs to persist across device reboot and updates.
    // The maximum size is set in the app_manifest.json (currently it's 8Kb).

} DeviceConfiguration;
extern DeviceConfiguration deviceConfiguration;

/// <summary>
///     Data structure for handling certificates in mutable storage .
/// </summary>
typedef struct
{
    char id[CERTSTORE_MAX_IDENTIFIER_LENGTH + 1];
    char relativePath[255];
    char privateKeyRelativePath[255];
    char privateKeyPass[CERTSTORE_MAX_PRIVATE_KEY_PASSWORD_LENGTH + 1];

} Certificate;

/// <summary>
///     Data structure for the EAP-TLS network management.
/// </summary>
typedef struct
{
    // Bootstrap network parameters
    NetworkInterfaceType bootstrapNetworkInterfaceType;
    char bootstrapNetworkName[WIFICONFIG_CONFIG_NAME_MAX_LENGTH + 1];
    char bootstrapNetworkSsid[WIFICONFIG_SSID_MAX_LENGTH + 1];

    // MDM WebAPI parameters
    char mdmWebApiInterfaceUrl[MAX_URL_LEN + 1];
    Certificate mdmWebApiRootCertificate;

    // EAP-TLS network parameters
    char eapTlsNetworkName[WIFICONFIG_CONFIG_NAME_MAX_LENGTH + 1];
    char eapTlsNetworkSsid[WIFICONFIG_SSID_MAX_LENGTH + 1];
    char eapTlsClientIdentity[WIFICONFIG_EAP_IDENTITY_MAX_LENGTH + 1];    
    Certificate eapTlsRootCertificate;
    Certificate eapTlsClientCertificate;

} EapTlsConfig;

/// <summary>
///     Data structure of the WebAPI response.
/// </summary>

typedef struct
{
    char timestamp[50];
    char rootCACertficate[CERTSTORE_MAX_CERT_SIZE + 1];
    char eapTlsNetworkSsid[WIFICONFIG_SSID_MAX_LENGTH + 1];
    char clientIdentity[WIFICONFIG_EAP_IDENTITY_MAX_LENGTH + 1];
    char clientPublicCertificate[CERTSTORE_MAX_CERT_SIZE + 1];
    char clientPrivateKey[CERTSTORE_MAX_CERT_SIZE + 1];
    char clientPrivateKeyPass[CERTSTORE_MAX_PRIVATE_KEY_PASSWORD_LENGTH + 1]; // Optionally, it could be injected as a constant into the application, through a SW update
} WebApiResponse;

//////////////////////////////////////////////////////////////
// Permanent device configuration helpers
//////////////////////////////////////////////////////////////

/// <summary>
///     Stores the given DeviceConfiguration object in permanent Mutable Storage.
/// </summary>
/// <param name="config">A pointer to a DeviceConfiguration object to be permanently stored.</param>
///     <para>EapTlsResult_Success, for success.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="config"/> pointer is NULL.</para>
///     <para>EapTlsResult_Error, failed writing to mutable storage, in which case 'errno' is set to the error value.</para>
EapTlsResult EapTls_StoreDeviceConfiguration(const DeviceConfiguration *config);

/// <summary>
///     Reads a previously stored DeviceConfiguration object from permanent Mutable Storage.
/// </summary>
/// <param name="configOut">A valid pointer to a DeviceConfiguration object where the result will be stored in.</param>
///     <para>EapTlsResult_Success, for success.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="configOut"/> pointer is NULL.</para>
///     <para>EapTlsResult_Error, failed reading from mutable storage, in which case 'errno' is set to the error value.</para>
EapTlsResult EapTls_ReadDeviceConfiguration(DeviceConfiguration *configOut);

//////////////////////////////////////////////////////////////
// Network configuration helpers
//////////////////////////////////////////////////////////////

/// <summary>
///     Saves and persists the current device's network configurations in flash.
/// </summary>
/// <param name="">none</param>
/// <returns>
///     <para>EapTlsResult_Success, for success.</para>
///     <para>EapTlsResult_Error, failed setting the bootstrap network interface, in which case 'errno' is set to the error value.</para>
/// </returns>
EapTlsResult EapTls_PersistNetworkConfig(void);

/// <summary>
///     Enables/disables target-scanning on the given network configuration name.
/// </summary>
/// <param name="networkName">The network configuration name to target.</param>
/// <param name="enabled">Set to 'true' to enable 'target' scan, 'false' to disable</param>
/// <returns>
///     <para>EapTlsResult_Success, for success.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="networkName"/> pointer is NULL.</para>
///     <para>EapTlsResult_NetworkUnknown, failed looking up for "networkName", in which case 'errno' is set to the error value.</para>
///     <para>EapTlsResult_FailedScanningNetwork, failed target-scanning for "networkName", in which case 'errno' is set to the error value.</para>
///     <para>EapTlsResult_Error, failed setting target-scan on current/target network, in which case 'errno' is set to the error value.</para>
/// </returns>
EapTlsResult EapTls_SetTargetScanOnNetwork(const char *networkName, bool enabled);

/// <summary>
///     Get the latest diagnostic-state for the given network name.
/// </summary>
/// <param name="networkName">The network configuration name to target.</param>
/// <returns>
///     <para>EapTlsResult_Connected, the network is correctly connected.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="networkName"/> pointer is NULL.</para>
///     <para>EapTlsResult_FailedDiagnosingNetwork, failed diagnosing the network configuration, in which case 'errno' is set to the error value.</para>
///     <para>EapTlsResult_NetworkUnknown, failed finding the network configuration, in which case 'errno' is set to the error value.</para>
///     <para>EapTlsResult_NetworkDisabled, the network configuration is currently disabled.</para>
///     <para>EapTlsResult_AuthenticationError, generic failure authenticating to the network</para>
///     <para>EapTlsResult_AuthenticationError_InvalidRootCaCert, failed authenticating to the network</para>
///     <para>EapTlsResult_AuthenticationError_InvalidClientCert, failed authenticating to the network</para>
///     <para>EapTlsResult_AuthenticationError_InvalidClientIdentity, failed authenticating to the network</para>
///     <para>EapTlsResult_ConnectionError, failed connecting to the network.
///         This error groups the following results from the WifiConfig_GetNetworkDiagnostics(..) API:
///             ConnectionFailed = 1 : Generic error message when connection fails.
///             NoPskIncluded = 3: Network password is missing.
///             WrongKey = 4: Network is using an incorrect password.
///             SecurityTypeMismatch = 6: The stored network's security type does not match the available network.
///             NetworkFrequencyNotAllowed = 7: Network frequency not allowed.
///             NetworkNotEssPbssMbss = 8: Network is not supported because no ESS, PBSS or MBSS was detected.
///             NetworkNotSupported = 9: Network is not supported.
///             NetworkNonWpa = 10: Network is not WPA2PSK, WPA2EAP or Open.
///     </para>
/// </returns>
EapTlsResult EapTls_DiagnoseNetwork(const char *networkName);

/// <summary>
///     Returns if the given network configuration ID is connected (preferably, use the signature with the network name ad the IDs may change!).
/// </summary>
/// <param name="networkID">The network configuration ID to target.</param>
/// <returns>
///     <para>EapTlsResult_Connected, the network is connected.</para>
///     <para>EapTlsResult_Disconnected, the network is disconnected.</para>
/// </returns>
EapTlsResult EapTls_IsNetworkIdConnected(int networkID);

/// <summary>
///     Returns if the given network configuration name is connected.
/// </summary>
/// <param name="networkName">The network configuration name to target.</param>
/// <returns>
///     <para>EapTlsResult_Connected, the network interface is connected to the given configuration name's network.</para>
///     <para>EapTlsResult_Disconnected, the network interface is disconnected from the given configuration name's network.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="networkName"/> pointer is NULL.</para>
/// </returns>
EapTlsResult EapTls_IsNetworkConnected(const char *networkName);

/// <summary>
///     Disables all network configurations except for the given network name.
/// </summary>
/// <param name="networkName">The network configuration name to target.</param>
/// <returns>
///     <para>EapTlsResult_Success, if succeeded.</para>
///     <para>EapTlsResult_Error, if any network enable/disablement among the whole network configurations fails.</para>
///     <para>EapTlsResult_NetworkUnknown, failed looking up the given network configuration name, in which case 'errno' is set to the error value.</para>
/// </returns>
EapTlsResult EapTls_DisableAllNetworksExcept(const char *networkName);

/// <summary>
///     Enables/disables the given network configuration name.
/// </summary>
/// <param name="networkName">The network configuration name to target.</param>
/// <param name="enabled">Set to 'true' to enable 'target' scan, 'false' to disable</param>
/// <returns>
///     <para>EapTlsResult_Success, for success.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="networkName"/> pointer is NULL.</para>
///     <para>EapTlsResult_NetworkUnknown, failed looking up the given network configuration name, in which case 'errno' is set to the error value.</para>
///     <para>EapTlsResult_Error, failed enabling/disabling the given network configuration, in which case 'errno' is set to the error value.</para>
/// </returns>
EapTlsResult EapTls_SetNetworkEnabledState(const char *networkName, bool enabled);

/// <summary>
///     Add a new network configuration.
/// </summary>
/// <param name="networkName">The network configuration name to set.</param>
/// <param name="networkSSID">The network's SSID to be assigned.</param>
/// <param name="securityType">The network's security type.</param>
/// <param name="psk">The network's PSK password (set to NULL is any or EAP-TLS security type)</param>
/// <returns>
///     <para>EapTlsResult_Success, for success.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="networkName"/> and/or <paramref name="networkSSID"/> pointers are NULL.</para>
///     <para>EapTlsResult_Error, failed adding the new network configuration, in which case 'errno' is set to the error value.</para>
/// </returns>
EapTlsResult EapTls_AddNetwork(const char *networkName, const char *networkSSID, WifiConfig_Security_Type securityType, const char *psk);

/// <summary>
///     Deletes (forgets) the given network configuration name.
/// </summary>
/// <param name="networkName">The network configuration name to set.</param>
/// <returns>
///     <para>EapTlsResult_Success, for success.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="networkName"/> pointer is NULL.</para>
///     <para>EapTlsResult_Error, failed removing the given network configuration, in which case 'errno' is set to the error value.</para>
/// </returns>
EapTlsResult EapTls_RemoveNetwork(const char *networkName);

/// <summary>
///     Clones the given network configuration.
/// </summary>
/// <param name="srcNetwork">The network configuration name to be cloned.</param>
/// <param name="dstNetwork">The name to be assigned to the cloned network configuration.</param>
/// <returns>
///     <para>EapTlsResult_Success, for success.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="srcNetwork"/> and/or <paramref name="dstNetwork"/> pointers are NULL.</para>
///     <para>EapTlsResult_Error, failed cloning the given network configuration, in which case 'errno' is set to the error value.</para>
/// </returns>
EapTlsResult EapTls_CloneNetworkConfig(EapTlsConfig *srcNetwork, EapTlsConfig *dstNetwork);

/// <summary>
///     Configures the security for the given EAP-TLS network configuration.
/// </summary>
/// <param name="networkName">The network configuration name to be configured.</param>
/// <param name="identity">The client identity to be provided to the EAP-TLS network.</param>
/// <param name="rootCACertificateId">The Root certificate ID to be used for authenticating the EAP-TLS network Server.</param>
/// <param name="clientCertificateId">The Client certificate ID to be used for authenticating to the EAP-TLS network.</param>
/// <returns>
///     <para>EapTlsResult_Success, for success.</para>
///     <para>EapTlsResult_BadParameters, one or more of the provided parameters pointers are NULL.</para>
///     <para>EapTlsResult_Error, failed enabling/disabling the given network configuration, in which case 'errno' is set to the error value.</para>
/// </returns>
EapTlsResult EapTls_ConfigureNetworkSecurity(const char *networkName, const char *identity, const char *rootCACertificateId, const char *clientCertificateId);

/// <summary>
///     Checks for MAX_CONNECTION_RETRIES if the given configuration name is connected.
/// </summary>
/// <param name="networkName">The network configuration name to connect to.</param>
/// <returns>
///     <para>EapTlsResult_Connected, the network is correctly connected.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="networkName"/> pointer is NULL.</para>
///     <para>EapTlsResult_NetworkUnknown, failed finding the network configuration, in which case 'errno' is set to the error value.</para>
///     <para>EapTlsResult_ConnectionTimeout, timeout waiting to connect to the given network configuration.</para>
/// </returns>
EapTlsResult EapTls_WaitToConnectTo(const char *networkName);

/// <summary>
///     Connects to the given configuration name.
/// </summary>
/// <param name="networkName">The network configuration name to connect to.</param>
/// <returns>
///     <para>EapTlsResult_Connected, the network is correctly connected.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="networkName"/> pointer is NULL.</para>
///     <para>EapTlsResult_FailedDiagnosingNetwork, failed diagnosing the network configuration, in which case 'errno' is set to the error value.</para>
///     <para>EapTlsResult_FailedTargetingNetwork, failed targeting the connection to the given network configuration.</para>
///     <para>EapTlsResult_Connected, the network is correctly connected.</para>
///     <para>EapTlsResult_NetworkUnknown, failed finding the network configuration, in which case 'errno' is set to the error value.</para>
///     <para>EapTlsResult_NetworkDisabled, the network configuration is currently disabled.</para>
///     <para>EapTlsResult_AuthenticationError, failed authenticating to the network.
///         This error groups the following results from the WifiConfig_GetNetworkDiagnostics(..) API:
///             NoPskIncluded = 3: Network password is missing.
///             WrongKey = 4: Network is using an incorrect password.
///             SecurityTypeMismatch = 6: The stored network's security type does not match the available network.
///             AuthenticationFailed = 5: Authentication failed. This error is thrown for EAP-TLS
///     </para>
///     <para>EapTlsResult_ConnectionError, failed connecting to the network.
///         This error groups the following results from the WifiConfig_GetNetworkDiagnostics(..) API:
///             ConnectionFailed = 1 : Generic error message when connection fails.
///             NetworkFrequencyNotAllowed = 7: Network frequency not allowed.
///             NetworkNotEssPbssMbss = 8: Network is not supported because no ESS, PBSS or MBSS was detected.
///             NetworkNotSupported = 9: Network is not supported.
///             NetworkNonWpa = 10: Network is not WPA2PSK, WPA2EAP or Open.
///     </para>
/// </returns>
EapTlsResult EapTls_ConnectToNetwork(const char *networkName);

//////////////////////////////////////////////////////////////
// Certificate store helpers
//////////////////////////////////////////////////////////////

/// <summary>
///     (prototype) Currently, just uses some unavailable API stubs (to be enabled with the USE_ADDITIONAL_API_STUBS precompiler #define)
/// </summary>
/// <param name="certificateId">The certificate ID in the device's CertStore that needs to be compared with the PEM blob.</param>
/// <param name="certificatePem">The certificate, in PEM format, that needs to be compared with certificate ID in the device's CertStore.</param>
/// <param name="certSize">The size of the certificate PEM-blob.</param>
/// <returns>
///     <para>Note: currently ALWAYS returns EapTlsResult_Error.</para>
///     <para>Stable implementation with new APIs will return:</para>
///     <para>EapTlsResult_Success, the certificates are identical.</para>
///     <para>EapTlsResult_Error, the certificates are different.</para>
///     <para>EapTlsResult_BadParameters, one or more of the provided pointers are NULL or empty.</para>
/// </returns>
EapTlsResult EapTls_CompareCertificates(const char *certificateId, const uint8_t *certificatePem, size_t certSize);

/// <summary>
///     (To be customized!)
///     Current implementation validates the given certificate ID in the device's CertStore,
///     with its 'NotBefore' and 'NotAfter' dates, against the current date/time.
/// </summary>
/// <param name="certificateId">The certificate ID in the device's CertStore that needs to be validated.</param>
/// <param name="expectedSubject">The expected certificate Subject to be validated (can be NULL if not needed).</param>
/// <param name="expectedIssuer">The expected certificate Issuer to be validated (can be NULL if not needed).</param>
/// <returns>
///     <para>EapTlsResult_Success, the certificate is installed in the device's CertStore.</para>
///     <para>EapTlsResult_Error, the certificate is NOT installed in the device's CertStore. If OS APIs have given errors, 'errno' is set to the error value.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="certificateId"/> pointer is NULL or empty.</para>
/// </returns>
EapTlsResult EapTls_ValidateCertificates(const char *certificateId, const char *expectedSubject, const char *expectedIssuer);

/// <summary>
///     Checks for the requested space availability in the device's CertStore.
/// </summary>
/// <param name="certificateSize">The required space needed in the device's CertStore</param>
/// <returns>
///     <para>EapTlsResult_Success, the device's CertStore has availability for 'certificateSize' bytes.</para>
///     <para>EapTlsResult_Error, the device's CertStore does NOT have availability for 'certificateSize' bytes. If OS APIs have given errors, 'errno' is set to the error value.</para>
/// </returns>
EapTlsResult EapTls_CheckcertStoreFreeSpace(size_t certificateSize);

/// <summary>
///     Checks if the given certificate ID is installed in the device's CertStore.
/// </summary>
/// <param name="certificateId">The certificate ID that needs to be checked if installed in the device's CertStore.</param>
/// <returns>
///     <para>EapTlsResult_Success, the certificate is installed in the device's CertStore.</para>
///     <para>EapTlsResult_Error, the certificate is NOT installed in the device's CertStore. If OS APIs have given errors, 'errno' is set to the error value.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="certificateId"/> pointer is NULL or empty.</para>
/// </returns>
EapTlsResult EapTls_IsCertificateInstalled(const char *certificateId);

/// <summary>
///     Installs the provided certificate as a 'trusted' root certificate, within the device's CertStore.
/// </summary>
/// <param name="certificateId">The certificate ID to be assigned in the device's CertStore, corresponding to the given PEM blob.</param>
/// <param name="certificatePem">The certificate, in PEM format, that needs to be installed in the device's CertStore.</param>
/// <returns>
///     <para>EapTlsResult_Success, the certificate has been successfully installed in the device's CertStore.</para>
///     <para>EapTlsResult_Error, the certificate FAILED to be installed in the device's CertStore. If OS APIs have given errors, 'errno' is set to the error value.</para>
///     <para>EapTlsResult_CertStoreFull, not enough space for this certificate, in the device's CertStore. If OS APIs have given errors, 'errno' is set to the error value.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="certificateId"/> and/or <paramref name="certificatePEM"/> pointers are NULL.</para>
/// </returns>
EapTlsResult EapTls_InstallRootCaCertificatePem(const char *certificateId, const char *certificatePem);

/// <summary>
///     Installs the provided certificate, defined in the 'Certificate' structure, as a 'trusted' root certificate, within the device's CertStore.
/// </summary>
/// <param name="certificate">A pre-configured 'Certificate' structure, defining all the details of the certificate to be installed.</param>
/// <returns>
///     <para>EapTlsResult_Success, the certificate has been successfully installed in the device's CertStore.</para>
///     <para>EapTlsResult_Error, the certificate FAILED to be installed in the device's CertStore. If OS APIs have given errors, 'errno' is set to the error value.</para>
///     <para>EapTlsResult_OutOfMemory, not enough memory to accomplish loading this certificate from the file.</para>
///     <para>EapTlsResult_CertStoreFull, not enough space for this certificate, in the device's CertStore. If OS APIs have given errors, 'errno' is set to the error value.</para>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="certificate"/> and/or its 'id'/'relativePath' pointers are NULL.</para>
/// </returns>
EapTlsResult EapTls_InstallRootCaCertificate(Certificate *certificate);

/// <summary>
///     Installs the provided certificate, as a 'client' private certificate within the device's CertStore (to be used for authenticating to EAP-TLS networks).
/// </summary>
/// <param name="certificateId">The certificate ID to be assigned in the device's CertStore, corresponding to the given PEM blob.</param>
/// <param name="certificatePem">The certificate, in PEM format, that needs to be installed in the device's CertStore.</param>
/// <param name="privateKeyPem">The certificate's private key, in PEM format, that needs to be installed in the device's CertStore.</param>
/// <param name="privateKeyPassword">The password with which the certificate's private key was encrypted.</param>
/// <returns>
///     <para>EapTlsResult_Success, the certificate has been successfully installed in the device's CertStore.</para>
///     <para>EapTlsResult_Error, the certificate FAILED to be installed in the device's CertStore. If OS APIs have given errors, 'errno' is set to the error value.</para>
///     <para>EapTlsResult_CertStoreFull, not enough space for this certificate, in the device's CertStore. If OS APIs have given errors, 'errno' is set to the error value.</para>
///     <para>EapTlsResult_BadParameters, one or more if the provided parameter pointers are NULL or empty.</para>
/// </returns>
EapTlsResult EapTls_InstallClientCertificatePem(const char *certificateId, const char *certificatePem, const char *privateKeyPem, const char *privateKeyPassword);

/// <summary>
///     Installs the provided certificate, defined in the 'Certificate' structure, as a 'client' private certificate within the device's CertStore.
/// </summary>
/// <param name="certificate">A pre-configured 'Certificate' structure, defining all the details of the certificate to be installed.</param>
/// <returns>
///     <para>EapTlsResult_Success, the certificate has been successfully installed in the device's CertStore.</para>
///     <para>EapTlsResult_Error, the certificate FAILED to be installed in the device's CertStore. If OS APIs have given errors, 'errno' is set to the error value.</para>
///     <para>EapTlsResult_OutOfMemory, not enough memory to accomplish loading this certificate from the file.</para>
///     <para>EapTlsResult_CertStoreFull, not enough space for this certificate, in the device's CertStore. If OS APIs have given errors, 'errno' is set to the error value.</para>
///     <para>EapTlsResult_BadParameters, one or more of the provided <paramref name="certificate"/> and/or its 'id'/'relativePath'/'privateKeyRelativePath' pointers are NULL or empty.</para>
/// </returns>
EapTlsResult EapTls_InstallClientCertificate(Certificate *certificate);

//////////////////////////////////////////////////////////////
// EAP-TLS network management
//////////////////////////////////////////////////////////////

/// <summary>
///     Sets the bootstrap's network interface type.
/// </summary>
/// <param name="eapTlsConfig">A valid pointer to a pre-configured 'EapTlsConfig' struct, containing all the connection details</param>
/// <param name="networkInterfaceType">The desired network interface type</param>
///     <para>EapTlsResult_Success, succeeded setting the bootstrap's network interface type.</para>
///     <para>EapTlsResult_Error, error setting network interface type.</para>
///     <para>EapTlsResult_BadParameters, <paramref name="eapTlsConfig"/> is NULL.</para>
EapTlsResult EapTls_SetBootstrapNetworkInterfaceType(EapTlsConfig *eapTlsConfig, NetworkInterfaceType networkInterfaceType);

/// <summary>
///     Enables/disables the bootstrap's network connection.
/// </summary>
/// <param name="eapTlsConfig">A valid pointer to a pre-configured 'EapTlsConfig' struct, containing all the connection details</param>
/// <param name="enabled"></param>
///     <para>EapTlsResult_Success, succeeded setting the state of the bootstrap's network interface type.</para>
///     <para>EapTlsResult_Error, error enabling/disabling the bootstrap's network interface/configuration.</para>
///     <para>EapTlsResult_BadParameters, <paramref name="eapTlsConfig"/> is NULL.</para>
EapTlsResult EapTls_SetBootstrapNetworkEnabledState(EapTlsConfig* eapTlsConfig, bool enabled);

/// <summary>
///     Connects automatically to the given EAP-TLS network configuration.
/// </summary>
/// <param name="config">A valid pointer to a pre-configured 'EapTlsConfig' struct, containing all the connection details</param>
/// <returns>
///     <para>EapTlsResult_BadParameters, the provided <paramref name="config"/> pointer is NULL.</para>
///     <para>EapTlsResult_Success, succeeded connecting to the EAP-TLS network.</para>
///     <para>EapTlsResult_Error, failed initializing connection handler.</para>
///     <para>EapTlsResult_NetworkUnknown, the network is unknown.</para>
///     <para>EapTlsResult_NetworkDisabled, the network is disabled.</para>
///     <para>EapTlsResult_ConnectionTimeout, generic timeout connecting to the network for technical reasons (as no diagnostics are available).</para>
///     <para>EapTlsResult_FailedAddingEapTlsNetwork, adding the configured EAP-TLS network failed.</para>
///     <para>EapTlsResult_FailedCloningEapTlsNetwork, failed in attempting  connection recovery through a configuration duplicate.</para>
///     <para>EapTlsResult_FailedInstallingCertificates, failed installing root/client certificates for authenticating with the EAP-TLS network.</para>
///     <para>EapTlsResult_FailedConfiguringCertificates, failed configuring the root/client certificates, for authenticating with the EAP-TLS network.</para>
///     <para>EapTlsResult_FailedInstallingNewCertificates, failed installing the new root/client certificates returned by the WebAPI.</para>
///     <para>EapTlsResult_FailedConfiguringNewCertificates, failed configuring the new root/client certificates, for authenticating with the EAP-TLS network.</para>
///     <para>EapTlsResult_FailedTargetingNetwork, failed forcing a connection to a network.</para>
///     <para>EapTlsResult_FailedConnectingToEapTlsNetwork, failed connecting to the EAP-TLS network.</para>
///     <para>EapTlsResult_FailedConnectingToEapTlsTmpNetwork, failed connecting to the temporary recovery-EAP-TLS network.</para>
///     <para>EapTlsResult_FailedConnectingToBootstrapNetwork, failed connecting to the Bootstrap network.</para>
///     <para>EapTlsResult_FailedSwappingEapTlsNetworkConfig, failed to swap the temporary EAP-TLS network configuration with the original one.</para>
///     <para>EapTlsResult_FailedConnectingToMdmWebApi, failed calling WebAPI.</para>
///     <para>EapTlsResult_FailedParsingMdmWebApiResponse, failed parsing response from WebAPI.</para>
///     <para>EapTlsResult_FailedReceivingNewCertificates, the WebAPI did not return the requested certificates.</para>
/// </returns>
EapTlsResult EapTls_RunConnectionManager(EapTlsConfig *eapTlsConfig);
