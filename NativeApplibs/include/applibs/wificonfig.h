/// \file wificonfig.h
/// \brief This header contains functions that manage Wi-Fi network configurations on a device.
/// These functions are only permitted if the application has the WifiConfig capability
/// in its application manifest.
///
/// To use these functions, define WIFICONFIG_STRUCTS_VERSION in your source code with the structure
/// version you're using. Currently, the only valid version is 1.
///    \c \#define WIFICONFIG_STRUCTS_VERSION 1
/// Thereafter, you can use the friendly names of the WifiConfig_ structures, which start with
/// WifiConfig_.
///
/// These functions are not thread safe.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#include <applibs/wificonfig_structs_v1.h>
#include <applibs/certstore_structs.h>

// Default the WIFICONFIG_STRUCTS_VERSION version to 1
#ifndef WIFICONFIG_STRUCTS_VERSION
#define WIFICONFIG_STRUCTS_VERSION 1
#endif

/// <summary>
///     The maximum length for a network configuration name (not including the null terminator)
/// </summary>
#define WIFICONFIG_CONFIG_NAME_MAX_LENGTH 16

/// <summary>
///     The maximum length for the EAP identity (not including the null terminator)
/// </summary>
#define WIFICONFIG_EAP_IDENTITY_MAX_LENGTH 254

#if WIFICONFIG_STRUCTS_VERSION == 1
/// <summary>
///     Alias to the z__WifiConfig_StoredNetwork_v1 structure. After you define
///     WIFICONFIG_STRUCTS_VERSION, you can refer to z__WifiConfig_StoredNetwork_v1
///     structures with this alias.
/// </summary>
typedef struct z__WifiConfig_StoredNetwork_v1 WifiConfig_StoredNetwork;
/// <summary>
///     Alias to the z__WifiConfig_ConnectedNetwork_v1 structure. After you define
///     WIFICONFIG_STRUCTS_VERSION, you can refer to z__WifiConfig_ConnectedNetwork_v1
///     structures with this alias.
/// </summary>
typedef struct z__WifiConfig_ConnectedNetwork_v1 WifiConfig_ConnectedNetwork;
/// <summary>
///     Alias to the z__WifiConfig_ScannedNetwork_v1 structure. After you define
///     WIFICONFIG_STRUCTS_VERSION, you can refer to z__WifiConfig_ScannedNetwork_v1
///     structures with this alias.
/// </summary>
typedef struct z__WifiConfig_ScannedNetwork_v1 WifiConfig_ScannedNetwork;
#else
#error To use applibs/wificonfig.h you must first #define a supported WIFICONFIG_STRUCTS_VERSION
#endif

/// <summary>
///    The client identity associated with a network.
/// </summary>
typedef struct WifiConfig_ClientIdentity {
    // A null-terminated byte array with an unspecified character encoding.
    char identity[WIFICONFIG_EAP_IDENTITY_MAX_LENGTH + 1];
} WifiConfig_ClientIdentity;

typedef struct WifiConfig_NetworkDiagnostics32 {
    /// <summary>Indicates whether the network is enabled.</summary>
    uint8_t isEnabled;
    /// <summary>Indicates whether the network is connected.</summary>
    uint8_t isConnected;
    /// <summary>
    /// The last reason to fail to connect to this network:
    ///     ConnectionFailed =      1: Generic error message if network connection failed.
    ///                                For EAP-TLS networks, this error is potentially caused by
    ///                                not being able to reach the RADIUS server.
    ///     NetworkNotFound =       2: Network was not found.
    ///     NoPskIncluded =         3: Network password is missing.
    ///     WrongKey =              4: Network is using an incorrect password.
    ///     AuthenticationFailed =  5: Authentication failed during an EAP-TLS network connection
    ///                                attempt.
    ///     SecurityTypeMismatch =  6: The stored network's security type does not match
    ///                                the available network.
    ///     NetworkFrequencyNotAllowed = 7: Network frequency not allowed.
    ///     NetworkNotEssPbssMbss = 8: Network is not supported because no ESS, PBSS or MBSS was
    ///                                detected.
    ///                                ESS: Extended Service Set
    ///                                PBSS: Personal Basic Service Set
    ///                                MBSS: Minimum Baseline Security Standard
    ///     NetworkNotSupported =   9: Network is not supported.
    ///     NetworkNonWpa =        10: Network is not WPA2PSK, WPA2EAP or Open.
    /// </summary>
    int32_t error;
    /// <summary>POSIX time when error was recorded in 32-bit time_t</summary>
    int32_t timestamp;
    /// <summary>
    /// The certificate error, meaningful only when 'error' indicates AuthenticationFailed:
    ///     CertificateNotFound = 100: The certificate was not found while connecting to the
    ///                                network. The user may need to call
    ///                                <see cref="WifiConfig_ReloadConfig"/> to load the
    ///                                certificates or confirm that the correct
    ///                                <see cref="CertStore_Identifier"/> is being used.
    ///     InvalidRootCA =       101: An error occurred during server authentication.
    ///     InvalidClientAuth =   102: An error occurred during client authentication. Potential
    ///                                causes are an invalid client certificate or a client
    ///                                identity the RADIUS server recognizes, but is not associated
    ///                                to the client certificate.
    ///     UnknownClientId =     103: The RADIUS server rejected the client identity during initial
    ///                                communication.
    /// </summary>
    int32_t certError;
    /// <summary>The certificate's depth in the certification chain. It's meaningful only when
    /// 'error' indicates AuthenticationFailed and certDepth is set to a non-negative
    /// number</summary>
    int32_t certDepth;
    /// <summary>The certificate's subject. It's meaningful only when 'error' indicates
    /// AuthenticationFailed</summary>
    CertStore_SubjectName certSubject;
} WifiConfig_NetworkDiagnostics32;

typedef struct WifiConfig_NetworkDiagnostics64 {
    /// <summary>Indicates whether the network is enabled.</summary>
    uint8_t isEnabled;
    /// <summary>Indicates whether the network is connected.</summary>
    uint8_t isConnected;
    /// <summary>
    /// The last reason to fail to connect to this network:
    ///     ConnectionFailed =      1: Generic error message if network connection failed.
    ///                                For EAP-TLS networks, this error is potentially caused by
    ///                                not being able to reach the RADIUS server or using a client
    ///                                identity the RADIUS server does not recognize.
    ///     NetworkNotFound =       2: Network was not found.
    ///     NoPskIncluded =         3: Network password is missing.
    ///     WrongKey =              4: Network is using an incorrect password.
    ///     AuthenticationFailed =  5: Authentication failed during an EAP-TLS network connection
    ///                                attempt.
    ///     SecurityTypeMismatch =  6: The stored network's security type does not match
    ///                                the available network.
    ///     NetworkFrequencyNotAllowed = 7: Network frequency not allowed.
    ///     NetworkNotEssPbssMbss = 8: Network is not supported because no ESS, PBSS or MBSS was
    ///                                detected.
    ///                                ESS: Extended Service Set
    ///                                PBSS: Personal Basic Service Set
    ///                                MBSS: Minimum Baseline Security Standard
    ///     NetworkNotSupported =   9: Network is not supported.
    ///     NetworkNonWpa =        10: Network is not WPA2PSK, WPA2EAP or Open.
    /// </summary>
    int32_t error;
    /// <summary>POSIX time when error was recorded in 64-bit representation</summary>
    time_t timestamp;
    /// <summary>
    /// The certificate error, meaningful only when 'error' indicates AuthenticationFailed:
    ///     CertificateNotFound = 100: The certificate was not found while connecting to the
    ///                                network. The user may need to call
    ///                                <see cref="WifiConfig_ReloadConfig"/> to load the
    ///                                certificates or confirm that the correct
    ///                                <see cref="CertStore_Identifier"/> is being used.
    ///     InvalidRootCA =       101: An error occurred during server authentication.
    ///     InvalidClientAuth =   102: An error occurred during client authentication. Potential
    ///                                causes are an invalid client certificate or used a client
    ///                                identity the RADIUS server recognizes, but is not associated
    ///                                to the client certificate.
    /// </summary>
    int32_t certError;
    /// <summary>The certificate's depth in the certification chain. It's meaningful only when
    /// 'error' indicates AuthenticationFailed and certDepth is set to a non-negative
    /// number</summary>
    int32_t certDepth;
    /// <summary>The certificate's subject. It's meaningful only when 'error' indicates
    /// AuthenticationFailed</summary>
    CertStore_SubjectName certSubject;
} WifiConfig_NetworkDiagnostics64;

typedef struct WifiConfig_NetworkDiagnostics64 WifiConfig_NetworkDiagnostics;

#include <applibs/wificonfig_internal.h>
#include <applibs/applibs_internal_api_traits.h>

/// <summary>
///     Gets the identifier of the stored client certificate of this network.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the EnterpriseWifiConfig
///     capability.</para>
///     <para>EFAULT: the <paramref name="outIdentifier"/> is NULL.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID to get information from. The network ID is returned by
///     <see cref="WifiConfig_AddNetwork"/> or <see cref="WifiConfig_AddDuplicateNetwork"/>.
/// </param>
/// <param name="outIdentifier">
///     The pointer to the <see cref="CertStore_Identifier"/> struct that receives the
///     identifier of the certificate associated with the given network. See <see
///     cref="CertStore_InstallClientCertificate"/> for details.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_GetClientCertStoreIdentifier(int networkId,
                                                   CertStore_Identifier *outIdentifier);

/// <summary>
///     Sets the identifier of the stored certificate to use as the client certificate for this
///     network. The setting is effective immediately but will be lost across a configuration reload
///     or a reboot unless the <see cref="WifiConfig_PersistConfig"/> function is called after this
///     function.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the EnterpriseWifiConfig
///     capability.</para>
///     <para>EFAULT: the <paramref name="certStoreIdentifier"/> is NULL.</para>
///     <para>ERANGE: the <paramref name="certStoreIdentifier"/> is greater than <see
///     cref="CERTSTORE_MAX_IDENTIFIER_LENGTH"/>.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID for the network to be configured. The network ID is returned by <see
///     cref="WifiConfig_AddNetwork"/>.
/// </param>
/// <param name="certStoreIdentifier">
///     A pointer to a constant null-terminated character string with the name of the certificate.
///     The name must be a unique string from one to <see cref="CERTSTORE_MAX_IDENTIFIER_LENGTH"/>
///     characters in length. See <see cref="CertStore_InstallClientCertificate"/> for details.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_SetClientCertStoreIdentifier(int networkId, const char *certStoreIdentifier);

/// <summary>
///     Gets the identifier of the stored root certificate authority of this network.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the EnterpriseWifiConfig
///     capability.</para>
///     <para>EFAULT: the <paramref name="outIdentifier"/> is NULL.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID to get information from. The network ID is returned by
///     <see cref="WifiConfig_AddNetwork"/> or <see cref="WifiConfig_AddDuplicateNetwork"/>.
/// </param>
/// <param name="outIdentifier">
///     The pointer to the <see cref="CertStore_Identifier"/> struct that receives the
///     identifier of the certificate associated with the given network. See <see
///     cref="CertStore_InstallRootCACertificate"/> for details.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_GetRootCACertStoreIdentifier(int networkId,
                                                   CertStore_Identifier *outIdentifier);

/// <summary>
///     Sets the identifier of the stored certificate to use as the root certificate authority for
///     this network. If this attribute is not set, the device will not authenticate the server that
///     it connects to. The setting is effective immediately but will be lost across a configuration
///     reload or a reboot unless the <see cref="WifiConfig_PersistConfig"/> function is called
///     after this function.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets
///     errno to the error value.
///     <para>EACCES: the application manifest does not include the
///     EnterpriseWifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="certStoreIdentifier"/> is NULL.</para>
///     <para>ERANGE: the <paramref name="certStoreIdentifier"/> greater than
///     <see cref="CERTSTORE_MAX_IDENTIFIER_LENGTH"/>.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID for the network to be configured. The network ID is returned by <see
///     cref="WifiConfig_AddNetwork"/>.
/// </param>
/// <param name="certStoreIdentifier">
///     A pointer to a constant null-terminated character string with the name of the certificate.
///     The name must be a unique string from one to <see cref="CERTSTORE_MAX_IDENTIFIER_LENGTH"/>
///     characters in length. See <see cref="CertStore_InstallRootCACertificate"/> for details.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_SetRootCACertStoreIdentifier(int networkId, const char *certStoreIdentifier);

/// <summary>
///     Stores an open Wi-Fi network without a key.
///     <para>This function will fail if an identical network is already stored on the device
///     without a key. See the error section (EEXIST). However, if a stored network includes a
///     key along with the same SSID, this function will succeed and store the network.</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EEXIST: a stored Wi-Fi network that has the same SSID and no key already
///     exists.</para>
///     <para>EFAULT: the <paramref name="ssid"/> is NULL.</para>
///     <para>ERANGE: the <paramref name="ssidLength"/> is 0 or greater than <see
///     cref="WIFICONFIG_SSID_MAX_LENGTH"/>.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>ENOSPC: there is no space left to store another Wi-Fi network.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="ssid">
///     A pointer to an SSID byte array with unspecified character encoding that identifies the
///     Wi-Fi network.
/// </param>
/// <param name="ssidLength">
///     The number of bytes in the SSID of the Wi-Fi network.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
_AZSPHERE_DEPRECATED_BY(WifiConfig_AddNetwork)
int WifiConfig_StoreOpenNetwork(const uint8_t *ssid, size_t ssidLength);

/// <summary>
///     Stores a WPA2 Wi-Fi network that uses a pre-shared key.
///     <para>This function will fail if a network with the same SSID and pre-shared key is already
///     stored. See the error section(EEXIST).</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EEXIST: a stored Wi-Fi network already exists that has the same SSID and uses
///     WPA2.</para>
///     <para>EFAULT: the <paramref name="ssid"/> or <paramref name="psk"/> is NULL.</para>
///     <para>ERANGE: the <paramref name="ssidLength"/> or <paramref name="pskLength"/> is 0 or
///     greater than <see cref="WIFICONFIG_WPA2_KEY_MAX_BUFFER_SIZE"/> and <see
///     cref="WIFICONFIG_SSID_MAX_LENGTH"/>.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>ENOSPC: there is no space left to store another Wi-Fi network.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="ssid">
///     A pointer to an SSID byte array with unspecified character encoding that identifies the
///     Wi-Fi network.
/// </param>
/// <param name="ssidLength">
///     The number of bytes in the <paramref name="ssid"/> of the Wi-Fi network.
/// </param>
/// <param name="psk">
///     A pointer to a buffer that contains the pre-shared key for the Wi-Fi network.
/// </param>
/// <param name="pskLength">
///     The length of the pre-shared key for the Wi-Fi network.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
_AZSPHERE_DEPRECATED_BY(WifiConfig_AddNetwork)
int WifiConfig_StoreWpa2Network(const uint8_t *ssid, size_t ssidLength, const char *psk,
                                size_t pskLength);

/// <summary>
///     Removes a Wi-Fi network from the device. Disconnects the device from the
///     network if it's currently connected.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="storedNetwork"/> is NULL.</para>
///     <para>ENOENT: the <paramref name="storedNetwork"/> does not match any of the stored
///     networks.</para>
///     <para>EINVAL: the <paramref name="storedNetwork"/> or its struct version is invalid.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
///     <para>ENOSPC: there are too many Wi-Fi networks to persist the config; remove one
///     and try again.</para>
/// </summary>
/// <param name="storedNetwork">
///     Pointer to a <see cref="WifiConfig_StoredNetwork"/> struct that describes the stored Wi-Fi
///     network to remove.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
_AZSPHERE_DEPRECATED_BY(WifiConfig_ForgetNetworkById)
static int WifiConfig_ForgetNetwork(const WifiConfig_StoredNetwork *storedNetwork);

/// <summary>
///     Removes a Wi-Fi network from the device. Disconnects the device from the
///     network if it's currently connected.
///     The change is effective immediately but will be lost across a configuration reload or a
///     reboot unless the <see cref="WifiConfig_PersistConfig"/> function is called after this
///     function.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID for the network to be removed.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int WifiConfig_ForgetNetworkById(int networkId);

/// <summary>
///     Removes all stored Wi-Fi networks from the device. Disconnects the device from
///     any connected network.
///     The removal is effective immediately and persists across a device reboot, unlike
///     <see cref="WifiConfig_ForgetNetworkById"/>.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int WifiConfig_ForgetAllNetworks(void);

/// <summary>
///     Gets the number of stored Wi-Fi networks on the device.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <returns>
///     The number of Wi-Fi networks stored on the device, or -1 for failure, in which case errno
///     is set to the error value.
/// </returns>
ssize_t WifiConfig_GetStoredNetworkCount(void);

/// <summary>
///     Retrieves all stored Wi-Fi networks on the device.
///     Before you call <see cref="WifiConfig_GetStoredNetworks"/>, you must call
///     <see cref="WifiConfig_GetStoredNetworkCount"/> and use the result as the array size for
///     the WifiConfig_StoredNetwork array that is passed in as the
///     <paramref name="storedNetworkArray"/> parameter.
///     <para>If <paramref name="storedNetworkArray"/> is too small to hold all the stored Wi-Fi
///     networks, this function fills the array and returns the number of array elements.</para>
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
static ssize_t WifiConfig_GetStoredNetworks(WifiConfig_StoredNetwork *storedNetworkArray,
                                            size_t storedNetworkArrayCount);

/// <summary>
///     Gets the Wi-Fi network that is connected to the device.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="connectedNetwork"/> is NULL.</para>
///     <para>ENODATA: the device is not currently connected to any network.</para>
///     <para>EINVAL: the <paramref name="connectedNetwork"/> or its struct version is
///     invalid.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="connectedNetwork">
///     A pointer to a struct that returns the connected Wi-Fi network.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_GetCurrentNetwork(WifiConfig_ConnectedNetwork *connectedNetwork);

/// <summary>
///     Gets the network ID of the currently connected network.
///     The network ID may change as network configurations are added and removed, so it is
///     recommended that the network ID is retrieved again before changing stored network
///     configurations if the device has been rebooted or network configurations have been added or
///     removed.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig
///     capability.</para>
///     <para>ENOTCONN: the device is not currently connected to any network.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <returns>
///     The network ID of the connected network (non-negative value), or -1 for failure, in
///     which case errno is set to the error value.
/// </returns>
int WifiConfig_GetConnectedNetworkId();

/// <summary>
///     Starts a scan to find all available Wi-Fi networks.
///     <para>This is a blocking call.</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
/// </summary>
/// <returns>
///     The number of networks found, or -1 for failure, in which case errno is set to the error
///     value.
/// </returns>
ssize_t WifiConfig_TriggerScanAndGetScannedNetworkCount(void);

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
///     invalid. </para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="scannedNetworkArray">
///     A pointer to an array that returns the retrieved Wi-Fi networks.
/// </param>
/// <param name="scannedNetworkArrayCount">
///     The number of elements scannedNetworkArray can hold. The array should have one element for
///     each Wi-Fi network found by the last scan operation.
/// </param>
/// <returns>
///     The number of elements returned by scannedNetworkArray, or -1 for failure, in
///     which case errno is set to the error value.
/// </returns>
static ssize_t WifiConfig_GetScannedNetworks(WifiConfig_ScannedNetwork *scannedNetworkArray,
                                             size_t scannedNetworkArrayCount);

/// <summary>
///     Adds an unconfigured new network to be configured using the WifiConfig_Setxxx APIs such as
///     <see cref="WifiConfig_SetSSID"/>.
///     Any change in the network configuration is effective immediately but will be lost across a
///     configuration reload or a reboot unless the <see cref="WifiConfig_PersistConfig"/> function
///     is called to save the configuration to nonvolatile storage.
///     <para> **Errors** </para> If an error is encountered, returns -1 and sets errno to the error
///     value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>ENOMEM: there is not enough memory to add a new network.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <returns>
///     The network ID of the new network (non-negative value), or -1 for failure, in which
///     case errno is set to the error value. The network ID is passed to
///     WifiConfig_Setxxx APIs as the identifier for the network to be configured.
/// </returns>
int WifiConfig_AddNetwork(void);

/// <summary>
///     Adds a new network that is a duplicate of the network with the given id. The new network's
///     config name will be the given config name. The new network is disabled by default. This
///     network can be configured using the WifiConfig_Setxxx APIs such as <see
///     cref="WifiConfig_SetSSID"/>. Any change in the network configuration is effective
///     immediately but will be lost across a configuration reload or a reboot unless the <see
///     cref="WifiConfig_PersistConfig"/> function is called to save the configuration to
///     nonvolatile storage.
///     <para> **Errors** </para> If an error is encountered, returns -1 and sets errno to the error
///     value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EFAULT: the <paramref name="configName"/> is NULL.</para>
///     <para>ERANGE: the <paramref name="configName"/> is less than one character or greater than
///     <see cref="WIFICONFIG_CONFIG_NAME_MAX_LENGTH"/> characters.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     <para>ENOMEM: there is not enough memory to add a new network.</para>
///     <para>EEXIST: the <paramref name="configName"/> is not unique.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID of the network to be duplicated.
/// </param>
/// <param name="configName">
///     A pointer to a byte array containing the configuration name. The byte array
///     must be NULL terminated and be less than or equal to
///     <see cref="WIFICONFIG_CONFIG_NAME_MAX_LENGTH"/> bytes (excluding the termination).
/// </param>
/// <remarks>
/// The newly created network configuration is separate from the original. Any changes made to the
/// original network will not be reflected in the duplicated one, and vice versa.
/// </remarks>
/// <returns>
///     The network ID of the new network (non-negative value), or -1 for failure, in which
///     case errno is set to the error value. The network ID is passed to
///     WifiConfig_Setxxx APIs as the identifier for the network to be configured.
/// </returns>
int WifiConfig_AddDuplicateNetwork(int networkId, const char *configName);

/// <summary>
///     Sets the SSID for the network.
///     The setting is effective immediately but will be lost across a configuration reload or a
///     reboot unless the <see cref="WifiConfig_PersistConfig"/> function is called after this
///     function.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="ssid"/> is NULL.</para>
///     <para>ERANGE: the <paramref name="ssidLength"/> is 0 or greater than <see
///     cref="WIFICONFIG_SSID_MAX_LENGTH"/>.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID for the network to be configured. The network ID is returned by
///     <see cref="WifiConfig_AddNetwork"/> or <see cref="WifiConfig_AddDuplicateNetwork"/>.
/// </param>
/// <param name="ssid">
///     A pointer to an SSID byte array with unspecified character encoding that identifies
///     the Wi-Fi network.
/// </param>
/// <param name="ssidLength">
///     The number of bytes in the <paramref name="ssid"/> of the Wi-Fi network.
///     Must be less than or equal to <see cref="WIFICONFIG_SSID_MAX_LENGTH"/>.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_SetSSID(int networkId, const uint8_t *ssid, size_t ssidLength);

/// <summary>
///     Sets the security type for the network.
///     The setting is effective immediately but will be lost across a configuration reload or a
///     reboot unless the <see cref="WifiConfig_PersistConfig"/> function is called after this
///     function.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig or
///     EnterpriseWifiConfig (when securityType == WifiConfig_Security_Wpa2_EAP_TLS)
///     capability.</para>
///     <para>EINVAL: the <paramref name="securityType"/> is an invalid value. </para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID for the network to be configured. The network ID is returned by
///     <see cref="WifiConfig_AddNetwork"/> or <see cref="WifiConfig_AddDuplicateNetwork"/>.
/// </param>
/// <param name="securityType">
///     The security type for the specified network.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_SetSecurityType(int networkId, WifiConfig_Security_Type securityType);

/// <summary>
///     Enables or disables the specified network configuration.
///     The setting is effective immediately but will be lost across a configuration reload or a
///     reboot unless the <see cref="WifiConfig_PersistConfig"/> function is called after this
///     function.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID for the network to be configured. The network ID is returned by
///     <see cref="WifiConfig_AddNetwork"/> or <see cref="WifiConfig_AddDuplicateNetwork"/>.
/// </param>
/// <param name="enabled">
///      A boolean value indicating whether the network should be enabled (true) or
///      disabled (false).
/// </param>
/// <returns>
///     0 for success, -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_SetNetworkEnabled(int networkId, bool enabled);

/// <summary>
///     Writes the current configuration for the network to nonvolatile storage so that
///     it will persist over a device reboot. All networks must have an SSID set for the function to
///     return successfully.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>ENOSPC: there are too many Wi-Fi networks to persist the config; remove one and try
///     again.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_PersistConfig(void);

/// <summary>
///     Reloads configuration from nonvolatile storage. All unsaved configuration will be lost.
///     All current Wifi connections will be disconnected and the connection process is restarted.
///     This command is required to reload certificates used for EAP-TLS if new certificates have
///     been installed using the CertStore APIs.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_ReloadConfig(void);

/// <summary>
///     Sets the Pre-Shared Key (PSK) for the network. The PSK is used for
///     networks configured with the WifiConfig_Security_Wpa2_Psk security type.
///     The setting is effective immediately but will be lost across a configuration reload or a
///     reboot unless the <see cref="WifiConfig_PersistConfig"/> function is called after this
///     function.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="psk"/> is NULL.</para>
///     <para>ERANGE: the <paramref name="pskLength"/> greater than
///     <see cref="WIFICONFIG_WPA2_KEY_MAX_BUFFER_SIZE"/>.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID for the network to be configured. The network ID is returned by
///     <see cref="WifiConfig_AddNetwork"/> or <see cref="WifiConfig_AddDuplicateNetwork"/>.
/// </param>
/// <param name="psk">
///     A pointer to a buffer that contains the pre-shared key for the Wi-Fi network.
/// </param>
/// <param name="pskLength">
///     The length of the pre-shared key for the Wi-Fi network. Must be less than or
///     equal to <see cref="WIFICONFIG_WPA2_KEY_MAX_BUFFER_SIZE"/>.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_SetPSK(int networkId, const char *psk, size_t pskLength);

/// <summary>
///     Enables or disables targeted scanning for the network. Targeted scanning is used to connect
///     to access points that are not broadcasting their SSID or to connect to an access point in a
///     noisy environment. Targeted scanning is disabled by default.
///
///     N.B. Enabling targeted scanning will cause the device to transmit probe requests that
///     contain the SSID of the network. Other devices may thus be able to discover the SSID of the
///     access point you're trying to connect to. Do not use targeted scanning if this poses a
///     problem.
///
///     The setting is effective immediately but will be lost across a configuration reload or a
///     reboot unless the <see cref="WifiConfig_PersistConfig"/> function is called after this
///     function.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID for the network to be configured. The network ID is returned by
///     <see cref="WifiConfig_AddNetwork"/> or <see cref="WifiConfig_AddDuplicateNetwork"/>.
/// </param>
/// <param name="enabled">
///      A boolean value indicating whether targeted scanning for the network should be
///      enabled (true) or disabled (false).
/// </param>
/// <returns>
///     0 for success, -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_SetTargetedScanEnabled(int networkId, bool enabled);

/// <summary>
///     Gets the client identity of this network. This identity is EAP-method-specific and is used
///     for authentication.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the EnterpriseWifiConfig
///     capability.</para>
///     <para>EFAULT: the <paramref name="outIdentity"/> is NULL.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID to get information from. The network ID is returned by
///     <see cref="WifiConfig_AddNetwork"/> or <see cref="WifiConfig_AddDuplicateNetwork"/>.
/// </param>
/// <param name="outIdentity">
///     A pointer to the <see cref="WifiConfig_ClientIdentity"/> struct that receives the client
///     identity associated with the given network.
///     The returned identity will be a NULL terminated byte array.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_GetClientIdentity(int networkId, WifiConfig_ClientIdentity *outIdentity);

/// <summary>
///     Sets the client identity for the network. This identity is EAP-method-specific and is used
///     for authentication.
///     The setting is effective immediately but will be lost across a configuration reload or a
///     reboot unless the <see cref="WifiConfig_PersistConfig"/> function is called after this
///     function.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the EnterpriseWifiConfig
///     capability.</para>
///     <para>EFAULT: the <paramref name="identity"/> is NULL.</para>
///     <para>ERANGE: the <paramref name="identity"/> greater than
///     <see cref="WIFICONFIG_EAP_IDENTITY_MAX_LENGTH"/>.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID for the network to be configured. The network ID is returned by
///     <see cref="WifiConfig_AddNetwork"/> or <see cref="WifiConfig_AddDuplicateNetwork"/>.
/// </param>
/// <param name="identity">
///     A pointer to the identity byte array. The byte array must be NULL terminated and
///     be less than or equal to <see cref="WIFICONFIG_EAP_IDENTITY_MAX_LENGTH"/> bytes
///     (excluding the termination).
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_SetClientIdentity(int networkId, const char *identity);

/// <summary>
///     Sets name for a network configuration. The name can be used as a convenient handle
///     to identify a network configuration. It is strongly recommended that this name be unique.
///     The setting is effective immediately but will be lost across a configuration reload or a
///     reboot unless the <see cref="WifiConfig_PersistConfig"/> function is called after this
///     function.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="configName"/> is NULL.</para>
///     <para>ERANGE: the <paramref name="configName"/> greater than
///     <see cref="WIFICONFIG_CONFIG_NAME_MAX_LENGTH"/>.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>ENODEV: the <paramref name="networkId"/> does not match any of the stored
///     networks.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID for the network to be configured. The network ID is returned by
///     <see cref="WifiConfig_AddNetwork"/> or <see cref="WifiConfig_AddDuplicateNetwork"/>.
/// </param>
/// <param name="configName">
///     A pointer to a byte array containing the configuration name. The byte array
///     must be NULL terminated and be less than or equal to
///     <see cref="WIFICONFIG_CONFIG_NAME_MAX_LENGTH"/> bytes (excluding the termination).
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_SetConfigName(int networkId, const char *configName);

/// <summary>
///     Gets the network ID for the network configuration with the given name. Calling this function
///     with a config name used by multiple networks is undefined behavior.
///     The name is assigned using <see cref="WifiConfig_SetConfigName"/>. The network ID may change
///     as network configurations are added and removed, so it is recommended that
///     the network ID is retrieved again before changing stored network configurations
///     if the device has been rebooted or network configurations have been added or removed.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="configName"/> is NULL.</para>
///     <para>ERANGE: the <paramref name="configName"/> greater than
///     <see cref="WIFICONFIG_CONFIG_NAME_MAX_LENGTH"/>.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENETDOWN: the Wi-Fi network interface is not available.</para>
///     <para>ENODEV: the specified network configuration cannot be found.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="configName">
///     A pointer to the name of the network configuration. This is the name that was set
///     using <see cref="WifiConfig_SetConfigName"/>. The string must be NULL
///     terminated and be less than or equal to <see cref="WIFICONFIG_CONFIG_NAME_MAX_LENGTH"/>
///     bytes (excluding the termination).
/// </param>
/// <returns>
///     The network ID of the specified network (non-negative value), or -1 for failure, in which
///     case errno is set to the error value. The network ID is passed to the
///     WifiConfig_Setxxx APIs as the identifier for the network to be configured.
/// </returns>
int WifiConfig_GetNetworkIdByConfigName(const char *configName);

/// <summary>
///     Gets the diagnostics information for the network. The result will contain the latest
///     information from a network's failed connection attempt. The result will also include the
///     network's current configuration state and connection state. If a network connected
///     successfully without first failing to connect, then there will not be any diagnostic
///     information.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the WifiConfig capability.</para>
///     <para>EFAULT: the <paramref name="networkDiagnostics"/> is NULL.</para>
///     <para>EINVAL: the <paramref name="networkId"/> is invalid.</para>
///     <para>EAGAIN: the Wi-Fi device isn't ready yet.</para>
///     <para>ENODEV: no diagnostic information was found for network <paramref
///     name="networkId"/>. This is expected if a network did not experience any errors while
///     connecting. ENODEV will also be set if the <paramref name="networkId"/> does not
///     exist.</para>
///     Any other errno may also be specified; such errors aren't deterministic and
///     there's no guarantee that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkId">
///     The network ID for the network to be queried. The network ID is returned by
///     <see cref="WifiConfig_AddNetwork"/> or <see cref="WifiConfig_AddDuplicateNetwork"/>.
/// </param>
/// <param name="networkDiagnostics">
///     A pointer to a WifiConfig_NetworkDiagnostics structure.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int WifiConfig_GetNetworkDiagnostics(int networkId,
                                            WifiConfig_NetworkDiagnostics *networkDiagnostics);

#ifdef __cplusplus
}
#endif
