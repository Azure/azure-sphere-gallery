/// \file wificonfig_internal.h
/// \brief This header contains internal functions for the WiFiConfig library, which manages Wi-Fi
/// configurations on a device.
///
/// These functions are not thread safe.
#pragma once

#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <applibs/certstore_structs.h>

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     The network attribute to configure.
/// </summary>
enum {
    /// <summary>Network SSID.</summary>
    z__WifiConfig_Attribute_SSID = 0,
    /// <summary>Network security type.</summary>
    z__WifiConfig_Attribute_Security_Type = 1,
    /// <summary>Enable/disable network.</summary>
    z__WifiConfig_Attribute_Enable = 2,
    /// <summary>Save network configuration.</summary>
    z__WifiConfig_Attribute_Save = 3,
    /// <summary>Reload network configuration.</summary>
    z__WifiConfig_Attribute_Reload = 4,
    /// <summary>WPA2 Pre-shared key.</summary>
    z__WifiConfig_Attribute_Preshared_Key = 5,
    /// <summary>Client identity.</summary>
    z__WifiConfig_Attribute_Client_Identity = 6,
    /// <summary>Client certificate identifier</summary>
    z__WifiConfig_Attribute_Client_CertStore_Identifier = 7,
    /// <summary>Root CA certificate identifier</summary>
    z__WifiConfig_Attribute_RootCA_CertStore_Identifier = 8,
    /// <summary>Enable/disable targeted scanning for the network.</summary>
    z__WifiConfig_Attribute_Targeted_Scan = 11,
    /// <summary>Custom name for the network configuration.</summary>
    z__WifiConfig_Attribute_Config_Name = 12,
    /// <summary>Wifi Network Diagnostics 32-bit time_t.</summary>
    z__WifiConfig_Attribute_Network_Diagnostics32 = 22,
    /// <summary>Wifi Network Diagnostics 64-bit time_t.</summary>
    z__WifiConfig_Attribute_Network_Diagnostics64 = 23,
    /// <summary>Wifi Network Diagnostics.</summary>
    z__WifiConfig_Attribute_Network_Diagnostics = z__WifiConfig_Attribute_Network_Diagnostics64,

};

typedef uint32_t z__WifiConfig_Attribute;

/// <summary>
///    Version support info for WifiConfig_StoredNetwork. Refer to z__WifiConfig_StoredNetwork_v[n]
///    in the appropriate wificonfig_structs_v[n].h header for actual struct fields.
/// </summary>
struct z__WifiConfig_StoredNetwork_Base {
    /// <summary>Internal version field.</summary>
    uint32_t z__magicAndVersion;
};

/// <summary>
///    A magic value that provides version support for the WifiConfig_StoredNetwork struct.
/// </summary>
#define z__WIFICONFIG_STORED_NETWORK_STRUCT_MAGIC 0xb22e0000

/// <summary>
///    Version support info for WifiConfig_ConnectedNetwork. Refer to
///    z__WifiConfig_ConnectedNetwork_v[n] in the appropriate wificonfig_structs_v[n].h header for
///    actual struct fields.
/// </summary>
struct z__WifiConfig_ConnectedNetwork_Base {
    /// <summary>Internal version field.</summary>
    uint32_t z__magicAndVersion;
};

/// <summary>
///    A magic value that provides version support for the WifiConfig_ConnectedNetwork struct.
/// </summary>
#define z__WIFICONFIG_CONNECTED_NETWORK_STRUCT_MAGIC 0x09ef0000

/// <summary>
///    Version support info for WifiConfig_ScannedNetwork. Refer to
///    z__WifiConfig_ScannedNetwork_v[n] in the appropriate wificonfig_structs_v[n].h header for
///    actual struct fields.
/// </summary>
struct z__WifiConfig_ScannedNetwork_Base {
    /// <summary>Internal version field.</summary>
    uint32_t z__magicAndVersion;
};

/// <summary>
///    A magic value that provides version support for WifiConfig_ScannedNetwork.
/// </summary>
#define z__WIFICONFIG_SCANNED_NETWORK_STRUCT_MAGIC 0x7eb10000

/// <summary>
///    Adds struct version support to WifiConfig_ForgetNetwork. Do not use this directly; use
///    WifiConfig_ForgetNetwork instead.
/// </summary>
/// <param name="storedNetwork">
///     A pointer to a WifiConfig_StoredNetwork struct that is type cast as
///     z__WifiConfig_StoredNetwork_Base.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int z__WifiConfig_ForgetNetwork(const struct z__WifiConfig_StoredNetwork_Base *storedNetwork);

/// <summary>
///     Removes a Wi-Fi network from the device. Disconnects the device from this network
///     if it's currently connected.
///     <para>The caller must provide a struct that describes the stored Wi-Fi network to
///     remove.</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="storedNetwork"/> is NULL.</para>
///     <para>ENOENT: the <paramref name="storedNetwork"/> does not match any of the stored
///     networks.</para>
///     <para>EINVAL: the <paramref name="storedNetwork"/> or its struct version is invalid.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="storedNetwork">
///     A pointer to a <see cref="WifiConfig_StoredNetwork"/> struct that describes the stored Wi-Fi
///     network to remove.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static inline int WifiConfig_ForgetNetwork(const WifiConfig_StoredNetwork *storedNetwork)
{
    return z__WifiConfig_ForgetNetwork(
        (const struct z__WifiConfig_StoredNetwork_Base *)storedNetwork);
}

/// <summary>
///    Adds version support to WifiConfig_GetStoredNetworks. Do not use this directly; use
///    WifiConfig_GetStoredNetworks instead.
/// </summary>
/// <param name="storedNetworkArray">
///     A pointer to an array that returns the stored Wi-Fi networks. The array contains
///     WifiConfig_StoredNetwork structs that are type cast as z__WifiConfig_StoredNetwork_Base
///     structs.
/// </param>
/// <param name="storedNetworkArrayCount">
///     The number of elements <paramref name="storedNetworkArray"/> can hold. The array should have
///     one element for each stored Wi-Fi network.
/// </param>
/// <param name="storedNetworkStructVersion">
///     The WifiConfig_StoredNetwork struct version.
/// </param>
/// <returns>
///     The number of elements in the WifiConfig_StoredNetwork array, or -1 for failure,
///     in which case errno is set to the error value.
/// </returns>
ssize_t z__WifiConfig_GetStoredNetworks(struct z__WifiConfig_StoredNetwork_Base *storedNetworkArray,
                                        size_t storedNetworkArrayCount,
                                        uint32_t storedNetworkStructVersion);

/// <summary>
///     Gets all stored Wi-Fi networks on the device.
///     <para>The caller must provide an array to return the Wi-Fi networks. If the array
///     is too small to hold all the stored Wi-Fi networks, the function fills the array
///     and returns the number of array elements.</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="storedNetworkArray"/> is NULL.</para>
///     <para>ERANGE: the <paramref name="storedNetworkArrayCount"/> is 0.</para>
///     <para>EINVAL: the <paramref name="storedNetworkArray"/> struct version is invalid.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="storedNetworkArray">
///     A pointer to an array that returns the stored Wi-Fi networks.
/// </param>
/// <param name="storedNetworkArrayCount">
///     The number of elements <paramref name="storedNetworkArray"/> can hold. The array should have
///     one element for each stored Wi-Fi network.
/// </param>
/// <returns>
///     The number of elements in the WifiConfig_StoredNetwork array, or -1 for failure,
///     in which case errno is set to the error value.
/// </returns>
static inline ssize_t WifiConfig_GetStoredNetworks(WifiConfig_StoredNetwork *storedNetworkArray,
                                                   size_t storedNetworkArrayCount)
{
    return z__WifiConfig_GetStoredNetworks(
        (struct z__WifiConfig_StoredNetwork_Base *)storedNetworkArray, storedNetworkArrayCount,
        WIFICONFIG_STRUCTS_VERSION);
}

/// <summary>
///    Adds version support to WifiConfig_GetCurrentNetwork. Do not use this directly; use
///    WifiConfig_GetCurrentNetwork instead.
/// </summary>
/// <param name="connectedNetwork">
///     A pointer to a struct that returns the connected Wi-Fi network. The array contains
///     WifiConfig_ConnectedNetwork structs that are type cast as
///     z__WifiConfig_ConnectedNetwork_Base structs.
/// </param>
/// <param name="ConnectedNetworkStructVersion">
///     The WifiConfig_ConnectedNetwork struct version.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int z__WifiConfig_GetCurrentNetwork(struct z__WifiConfig_ConnectedNetwork_Base *connectedNetwork,
                                    uint32_t ConnectedNetworkStructVersion);

/// <summary>
///     Gets the Wi-Fi network that is connected to the device.
///     <para>If the device is not currently connected to any network, -1 is returned, in which case
///     errno is set to ENOTCONN(107).
///     </para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="connectedNetwork"/> is NULL.</para>
///     <para>ENODATA: the device is not currently connected to any network.</para>
///     <para>EINVAL: the <paramref name="connectedNetwork"/> or its struct version is
///     invalid.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="connectedNetwork">
///     A pointer to a struct that returns the connected Wi-Fi network.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static inline int WifiConfig_GetCurrentNetwork(WifiConfig_ConnectedNetwork *connectedNetwork)
{
    return z__WifiConfig_GetCurrentNetwork(
        (struct z__WifiConfig_ConnectedNetwork_Base *)connectedNetwork, WIFICONFIG_STRUCTS_VERSION);
}

/// <summary>
///    Adds version support to WifiConfig_GetScannedNetworks. Do not use this directly; use
///    WifiConfig_GetScannedNetworks instead.
/// </summary>
/// <param name="scannedNetworkArray">
///     A pointer to an array that returns the retrieved Wi-Fi networks. The array contains
///     WifiConfig_ScannedNetwork structs that are type cast as z__WifiConfig_ScannedNetwork_Base
///     structs.
/// </param>
/// <param name="scannedNetworkArrayCount">
///     The number of elements the array can hold. The array should have one element for each Wi-Fi
///     network found by the last scan operation.
/// </param>
/// <param name="scannedNetworkStructVersion">
///     The WifiConfig_ScannedNetwork struct version.
/// </param>
/// <returns>
///     The number of elements in the WifiConfig_ScannedNetwork array, or -1 for failure, in
///     which case errno is set to the error value.
/// </returns>
ssize_t z__WifiConfig_GetScannedNetworks(
    struct z__WifiConfig_ScannedNetwork_Base *scannedNetworkArray, size_t scannedNetworkArrayCount,
    uint32_t scannedNetworkStructVersion);

/// <summary>
///     Gets the Wi-Fi networks found by the last scan operation.
///     <para>If scannedNetworkArray is too small to hold all the networks, this function fills all
///     the elements and returns the number of array elements.</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="scannedNetworkArray"/> is NULL.</para>
///     <para>ERANGE: the <paramref name="scannedNetworkArrayCount"/> is 0.</para>
///     <para>EINVAL: the <paramref name="scannedNetworkArray"/> or its struct version is
///     invalid.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     Any other errno may also be specified; such errors
///     aren't deterministic and there's no guarantee that the same behavior will be retained
///     through system updates.
/// </summary>
/// <param name="scannedNetworkArray">
///     A pointer to an array that returns the retrieved Wi-Fi networks.
/// </param>
/// <param name="scannedNetworkArrayCount">
///     The number of elements the array can hold. The array should have one element for each Wi-Fi
///     network found by the last scan operation.
/// </param>
/// <returns>
///     The number of elements in the WifiConfig_ScannedNetwork array, or -1 for failure, in which
///     case errno is set to the error value.
/// </returns>
static inline ssize_t WifiConfig_GetScannedNetworks(WifiConfig_ScannedNetwork *scannedNetworkArray,
                                                    size_t scannedNetworkArrayCount)
{
    return z__WifiConfig_GetScannedNetworks(
        (struct z__WifiConfig_ScannedNetwork_Base *)scannedNetworkArray, scannedNetworkArrayCount,
        WIFICONFIG_STRUCTS_VERSION);
}

/// <summary>
///     Set the attribute value for the specified network.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig or
///     EnterpriseWifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="attributeValue"/> is NULL.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     <para>EINVAL: the value for <paramref name="networkAttribute"/> or <paramref
///     name="attributeValue"/> is invalid.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID for the network to be configured. The network ID is returned by
///     WifiConfig_AddNetwork().
/// </param>
/// <param name="networkAttribute">
///     The network attribute to be configured.
/// </param>
/// <param name="attributeValue">
///     Pointer to appropriate data type containing the attribute value.
/// </param>
/// <param name="attributeLength">
///     0 for null-terminated strings. For other types, size of attributeValue.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int z__WifiConfig_SetNetworkOpt(int networkId, z__WifiConfig_Attribute networkAttribute,
                                const void *attributeValue, size_t attributeLength);

static inline int WifiConfig_SetSSID(int networkId, const uint8_t *ssid, size_t ssidLength)
{
    return z__WifiConfig_SetNetworkOpt(networkId, z__WifiConfig_Attribute_SSID, ssid, ssidLength);
}

static inline int WifiConfig_SetSecurityType(int networkId, WifiConfig_Security_Type securityType)
{
    return z__WifiConfig_SetNetworkOpt(networkId, z__WifiConfig_Attribute_Security_Type, &securityType,
                                       sizeof(securityType));
}

static inline int WifiConfig_SetNetworkEnabled(int networkId, bool enabled)
{
    return z__WifiConfig_SetNetworkOpt(networkId, z__WifiConfig_Attribute_Enable, &enabled,
                                       sizeof(enabled));
}

static inline int WifiConfig_PersistConfig(void)
{
    return z__WifiConfig_SetNetworkOpt(-1, z__WifiConfig_Attribute_Save, NULL, 0);
}

static inline int WifiConfig_ReloadConfig(void)
{
    return z__WifiConfig_SetNetworkOpt(-1, z__WifiConfig_Attribute_Reload, NULL, 0);
}

static inline int WifiConfig_SetPSK(int networkId, const char *psk, size_t pskLength)
{
    return z__WifiConfig_SetNetworkOpt(networkId, z__WifiConfig_Attribute_Preshared_Key, psk,
                                       pskLength);
}

static inline int WifiConfig_SetClientIdentity(int networkId, const char *identity)
{
    return z__WifiConfig_SetNetworkOpt(networkId, z__WifiConfig_Attribute_Client_Identity, identity,
                                       0);
}

static inline int WifiConfig_SetClientCertStoreIdentifier(int networkId,
                                                          const char *certStoreIdentifier)
{
    return z__WifiConfig_SetNetworkOpt(networkId, z__WifiConfig_Attribute_Client_CertStore_Identifier,
                                       certStoreIdentifier, 0);
}

static inline int WifiConfig_SetRootCACertStoreIdentifier(int networkId,
                                                          const char *certStoreIdentifier)
{
    return z__WifiConfig_SetNetworkOpt(networkId, z__WifiConfig_Attribute_RootCA_CertStore_Identifier,
                                       certStoreIdentifier, 0);
}

static inline int WifiConfig_SetConfigName(int networkId, const char *configName)
{
    return z__WifiConfig_SetNetworkOpt(networkId, z__WifiConfig_Attribute_Config_Name, configName, 0);
}

static inline int WifiConfig_SetTargetedScanEnabled(int networkId, bool enabled)
{
    return z__WifiConfig_SetNetworkOpt(networkId, z__WifiConfig_Attribute_Targeted_Scan, &enabled,
                                       sizeof(enabled));
}

/// <summary>
///     Get the attribute value for the specified network.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig or
///     EnterpriseWifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="buffer"/> is NULL while <paramref name="capacity"/> is
///     non-zero.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     <para>EINVAL: the value for <paramref name="networkAttribute"/> or <paramref
///     name="networkId"/> is invalid.</para>
///     <para>ERANGE: <paramref name="capacity"/> is not large enough to hold the given <paramref
///     name="networkAttribute"/>.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID for the network whose attribute to retrieve. The network ID is returned by
///     WifiConfig_AddNetwork().
/// </param>
/// <param name="networkAttribute">
///     The network attribute to be retrieved.
/// </param>
/// <param name="buffer">
///     Pointer to the buffer to receive the value.
///     If this pointer is NULL and <paramref name="capacity"/> is 0, the API will return the number
///     of bytes needed for copying the full data.
///     Otherwise the API will fail with ERANGE if the buffer is not NULL but not large enough to
///     receive the full data.
/// </param>
/// <param name="capacity">
///     The capacity of the <paramref name="buffer"/>.
/// </param>
/// <returns>
///     If <paramref name="buffer"/> is NULL and <paramref name="capacity"/> is 0, the number of
///     bytes needed to copy the data is returned.
///
///     If <paramref name="buffer"/> is not NULL and it's large enough to copy the data, the number
///     of bytes copied to the buffer is returned.
///
///     On failure, -1 is returned and errno is set to the error code
/// </returns>
ssize_t z__WifiConfig_GetNetworkOpt(int networkId, z__WifiConfig_Attribute networkAttribute,
                                    void *buffer, size_t capacity);

/// <summary>
///     Gets the identifier of the certificate of this network.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the EnterpriseWifiConfig
///     capability.</para>
///     <para>EFAULT: the <paramref name="outIdentifier"/> is NULL.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid or <paramref
///     name="certificate_type"/> does not equal a valid certificate type.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID to get information from. The network ID is returned by
///     <see cref="WifiConfig_AddNetwork"/> or <see cref="WifiConfig_AddDuplicateNetwork"/>.
/// </param>
/// <param name="certificate_type">
///     The type of certificate to retrieve.
/// </param>
/// <param name="outIdentifier">
///     The pointer to the struct <see cref="CertStore_Identifier"/> that receives the
///     identifier of the certificate associated with the given network. See <see
///     cref="CertStore_InstallClientCertificate"/> or <see
///     cref="CertStore_InstallRootCACertificate"/> for details.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static inline int z__WifiConfig_GetCertificateIdentifier(int networkId,
                                                         z__WifiConfig_Attribute certificate_type,
                                                         CertStore_Identifier *outIdentifier)
{
    if (outIdentifier == NULL) {
        errno = EFAULT;
        return -1;
    }

    if (certificate_type != z__WifiConfig_Attribute_Client_CertStore_Identifier &&
        certificate_type != z__WifiConfig_Attribute_RootCA_CertStore_Identifier) {
        errno = EINVAL;
        return -1;
    }

    char *buffer = outIdentifier->identifier;
    size_t bufferLength = sizeof(outIdentifier->identifier);

    ssize_t bytes_read =
        z__WifiConfig_GetNetworkOpt(networkId, certificate_type, buffer, bufferLength);

    if (bytes_read == -1) {
        return bytes_read;
    }

    return 0;
}

static inline int WifiConfig_GetNetworkDiagnostics(
    int networkId, WifiConfig_NetworkDiagnostics *networkDiagnostics)
{
    if (networkDiagnostics == NULL) {
        errno = EFAULT;
        return -1;
    }

    ssize_t res = z__WifiConfig_GetNetworkOpt(networkId, z__WifiConfig_Attribute_Network_Diagnostics64,
                                              networkDiagnostics, sizeof(*networkDiagnostics));
    if (res == -1) {
        return -1;
    }

    return 0;
}

static inline int WifiConfig_GetClientIdentity(int networkId,
                                               WifiConfig_ClientIdentity *outIdentity)
{
    if (outIdentity == NULL) {
        errno = EFAULT;
        return -1;
    }

    char *buffer = outIdentity->identity;
    size_t bufferLength = sizeof(outIdentity->identity);
    ssize_t bytes_read = z__WifiConfig_GetNetworkOpt(
        networkId, z__WifiConfig_Attribute_Client_Identity, buffer, bufferLength);

    if (bytes_read == -1) {
        return bytes_read;
    }

    return 0;
}

static inline int WifiConfig_GetClientCertStoreIdentifier(int networkId,
                                                          CertStore_Identifier *outIdentifier)
{
    return z__WifiConfig_GetCertificateIdentifier(
        networkId, z__WifiConfig_Attribute_Client_CertStore_Identifier, outIdentifier);
}

static inline int WifiConfig_GetRootCACertStoreIdentifier(int networkId,
                                                          CertStore_Identifier *outIdentifier)
{
    return z__WifiConfig_GetCertificateIdentifier(
        networkId, z__WifiConfig_Attribute_RootCA_CertStore_Identifier, outIdentifier);
}

#ifdef __cplusplus
}
#endif
