/// \file networking.h
/// \brief This header contains functions and types that interact with the networking subsystem to
/// query the network state, and to get and set the network service configuration.
///
/// The function summaries include any required application manifest settings.
///
/// The default version of the networking structs(NETWORKING_STRUCTS_VERSION) is 1. Currently this
/// is the only valid version. These functions are not thread safe.
#pragma once

#include <stdbool.h>
#include <time.h>
#include <applibs/networking_structs_v1.h>

// Default the NETWORKING_STRUCTS_VERSION version to 1
#ifndef NETWORKING_STRUCTS_VERSION
#define NETWORKING_STRUCTS_VERSION 1
#endif

#if NETWORKING_STRUCTS_VERSION == 1
/// <summary>
///     Alias to the z__Networking_NetworkInterface_v1 structure. After you define
///     NETWORKING_STRUCTS_VERSION, you can refer to z__Networking_NetworkInterface_v1
///     structures with this alias.
/// </summary>
typedef struct z__Networking_NetworkInterface_v1 Networking_NetworkInterface;

#else // NETWORKING_STRUCTS_VERSION
#error NETWORKING_STRUCTS_VERSION is set to an unsupported value. Only version 1 is currently supported.
#endif // NETWORKING_STRUCTS_VERSION

#include <applibs/networking_internal.h>
#include <applibs/applibs_internal_api_traits.h>

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     A bit mask that specifies the connection status of a network interface.
/// </summary>
typedef uint32_t Networking_InterfaceConnectionStatus;
enum {
    /// <summary>The interface is enabled.</summary>
    Networking_InterfaceConnectionStatus_InterfaceUp = 1 << 0,
    /// <summary>The interface is connected to a network.</summary>
    Networking_InterfaceConnectionStatus_ConnectedToNetwork = 1 << 1,
    /// <summary>The interface has an IP address assigned to it.</summary>
    Networking_InterfaceConnectionStatus_IpAvailable = 1 << 2,
    /// <summary>The interface is connected to the internet.</summary>
    Networking_InterfaceConnectionStatus_ConnectedToInternet = 1 << 3
};

/// <summary>
///     An option to enable or disable the default NTP server to use as a fallback.
/// </summary>
typedef uint32_t Networking_NtpOption;
enum {
    /// <summary>Disable the default NTP.</summary>
    Networking_NtpOption_FallbackServerDisabled = 0,
    /// <summary>Enable the default NTP along with custom or automatic NTP.</summary>
    Networking_NtpOption_FallbackServerEnabled = 1,
};

/// <summary>
///     A bit mask that specifies the proxy status.
/// </summary>
typedef uint32_t Networking_ProxyStatus;
enum {
    /// <summary>The proxy configuration is enabled.</summary>
    Networking_ProxyStatus_Enabled = 1 << 0,
    /// <summary>The proxy name is getting resolved.</summary>
    Networking_ProxyStatus_ResolvingProxyName = 1 << 1,
    /// <summary>The proxy is ready.</summary>
    Networking_ProxyStatus_Ready = 1 << 2,
};

/// <summary>
///     Verifies whether networking is ready and time is synced.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the provided <paramref name="outIsNetworkingReady"/> parameter is NULL.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="outIsNetworkingReady">
///      A pointer to a boolean that returns the result. The return value is true if:
///      - the network interface, e.g. wlan0 or eth0 is enabled
///      - the interface is connected to a networking access point, e.g. Wi-Fi access point
///        or Ethernet router,
///      - the interface has obtained an IP address, and
///      - the device time is synced.
///      Otherwise, return false.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_IsNetworkingReady(bool *outIsNetworkingReady);

/// <summary>
///     Gets the number of network interfaces in the Azure Sphere device.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behaviour will be retained through system updates.
/// </summary>
/// <returns>
///     The number of network interfaces, or -1 for failure, in which case errno is set to the error
///     value.
/// </returns>
ssize_t Networking_GetInterfaceCount(void);

/// <summary>
///     Gets the list of network interfaces in the Azure Sphere device. If
///     <paramref name="outNetworkInterfaces"/> is too small to hold all network interfaces in the
///     system, this function fills the array and returns the number of array elements. The number
///     of interfaces in the system will not change within a boot cycle.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="outNetworkInterfacesArray"/> parameter is NULL.</para>
///     <para>ERANGE: the <paramref name="networkInterfacesArrayCount"/> parameter is 0.</para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="outNetworkInterfacesArray">
///      A pointer to an array of <see cref="Networking_NetworkInterface"/> structs to fill with
///      network interface properties. The caller must allocate memory for the array after calling
///      Networking_GetInterfacesCount to retrieve the number of interfaces on the device.
/// </param>
/// <param name="networkInterfacesArrayCount">
///      The number of elements <paramref name="outNetworkInterfacesArray"/> can hold.
/// </param>
/// <returns>
///     The number of network interfaces added to <paramref name="outNetworkInterfacesArray"/>.
///     Otherwise -1 for failure, in which case errno is set to the error value.
/// </returns>
static ssize_t Networking_GetInterfaces(Networking_NetworkInterface *outNetworkInterfacesArray,
                                        size_t networkInterfacesArrayCount);

/// <summary>
///     Enables or disables a network interface. The application manifest must include the
///     NetworkConfig capability.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the NetworkConfig capability.</para>
///     <para>ENOENT: the network interface does not exist.</para>
///     <para>EPERM: this function is not allowed on the interface.</para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="networkInterfaceName">
///      The name of the network interface to update.
/// </param>
/// <param name="isEnabled">
///      true to enable the interface, false to disable it.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_SetInterfaceState(const char *networkInterfaceName, bool isEnabled);

/// <summary>
///     Initializes a <see cref="Networking_IpConfig"/> struct with the default IP
///     configuration. The default IP configuration enables dynamic IP.
/// </summary>
/// <remarks>
///     When the <see cref="Networking_IpConfig"/> struct is no longer needed, the
///     <see cref="Networking_IpConfig_Destroy"/> function must be called on the struct to avoid
///     resource leaks.
/// </remarks>
/// <param name="ipConfig">
///     A pointer to the <see cref="Networking_IpConfig"/> struct that receives the default IP
///     configuration.
/// </param>
void Networking_IpConfig_Init(Networking_IpConfig *ipConfig);

/// <summary>
///     Destroys a <see cref="Networking_IpConfig"/> struct.
/// </summary>
/// <remarks>It's unsafe to call <see cref="Networking_IpConfig_Destroy"/> on a struct that hasn't
///     been initialized. After <see cref="Networking_IpConfig_Destroy"/> is called, the
///     <see cref="Networking_IpConfig"/> struct must not be used until it is re-initialized
///     with the <see cref="Networking_IpConfig_Init"/> function.
/// </remarks>
/// <param name="ipConfig">
///     A pointer to the <see cref="Networking_IpConfig"/> struct to destroy.
/// </param>
void Networking_IpConfig_Destroy(Networking_IpConfig *ipConfig);

/// <summary>
///     Enables dynamic IP and disables static IP for a <see cref="Networking_IpConfig"/> struct.
/// </summary>
/// <param name="ipConfig">
///     A pointer to the <see cref="Networking_IpConfig"/> struct to update.
/// </param>
void Networking_IpConfig_EnableDynamicIp(Networking_IpConfig *ipConfig);

/// <summary>
///     Enables static IP and disables dynamic IP for a <see cref="Networking_IpConfig"/> struct.
/// </summary>
/// <remarks>
///     Using a static IP configuration will prevent the device from automatically obtaining DNS
///     server addresses.
/// </remarks>
/// <param name="ipConfig">
///     A pointer to the <see cref="Networking_IpConfig"/> struct to update.
/// </param>
/// <param name="ipAddress">The static IP address.</param>
/// <param name="subnetMask">The static subnet mask.</param>
/// <param name="gatewayAddress">The static gateway address.</param>
void Networking_IpConfig_EnableStaticIp(Networking_IpConfig *ipConfig, struct in_addr ipAddress,
                                        struct in_addr subnetMask, struct in_addr gatewayAddress);

/// <summary>
///     Automatically obtain DNS server addresses for a <see cref="Networking_IpConfig"/> struct.
/// </summary>
/// <param name="ipConfig">
///     A pointer to the <see cref="Networking_IpConfig"/> struct to update.
/// </param>
void Networking_IpConfig_EnableAutomaticDns(Networking_IpConfig *ipConfig);

/// <summary>
///     Use custom DNS server addresses for a <see cref="Networking_IpConfig"/> struct. Up to three
///     addresses may be specified. Any existing DNS server addresses will be cleared.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="ipConfig"/> or <paramref name="dnsServerAddresses"/>
///     parameter is NULL.</para>
///     <para>EINVAL: more than three IP addresses were provided or an address in <paramref
///     name="dnsServerAddresses"/> equals INADDR_ANY.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="ipConfig">
///     A pointer to the <see cref="Networking_IpConfig"/> struct to update.
/// </param>
/// <param name="dnsServerAddresses">A pointer to an array of DNS server addresses.</param>
/// <param name="serverCount">
///     The number of DNS server addresses in the <paramref name="dnsServerAddresses"/> array.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_IpConfig_EnableCustomDns(Networking_IpConfig *ipConfig,
                                        const struct in_addr *dnsServerAddresses,
                                        size_t serverCount);

/// <summary>
///     Applies an IP configuration to a network interface. The application manifest must
///     include the NetworkConfig capability.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the calling application doesn't have the NetworkConfig capability.</para>
///     <para>ENOENT: the <paramref name="networkInterfaceName"/> parameter refers to an interface
///     that does not exist.</para>
///     <para>EPERM:  this operation is not allowed on the network interface.</para>
///     <para>EFAULT: the <paramref name="networkInterfaceName"/> or <paramref name="ipConfig"/>
///      parameter is NULL.</para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     <para>EFBIG: out of space to store the configuration.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behavior will be retained through system updates.
/// </summary>
/// <remarks>
///     <para>This function does not verify whether the static IP address is compatible with the
///     dynamic IP addresses received through an interface using a DHCP client.</para>
///     <para>This function does not verify whether a DHCP server is available on the network and if
///     a dynamic IP address is configured.</para>
///     <para>If overlapping IP address configurations are present on a device, the behavior of this
///     function is undefined.</para>
/// </remarks>
/// <param name="networkInterfaceName">
///      The name of the interface to configure.
/// </param>
/// <param name="ipConfig">
///      The pointer to the <see cref="Networking_IpConfig" /> struct that contains the IP
///      configuration to apply.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_IpConfig_Apply(const char *networkInterfaceName,
                              const Networking_IpConfig *ipConfig);

/// <summary>
///     Use the default NTP server address for time sync.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the calling application doesn't have the TimeSyncConfig capability.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_TimeSync_EnableDefaultNtp(void);

/// <summary>
///     Attempt to obtain and use NTP server addresses from DHCP option 42. The NTP servers obtained
///     from DHCP will be queried sequentially based on their priority, with the default server
///     ranked last if it is enabled.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the calling application doesn't have the TimeSyncConfig capability.</para>
///     <para>EINVAL: the provided <paramref name="option"/> parameter is invalid.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="option">Enables or disables the default NTP server.</param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_TimeSync_EnableAutomaticNtp(Networking_NtpOption option);

/// <summary>
///     Use custom NTP server addresses. Up to two hostnames or IP addresses may be specified. The
///     NTP servers will be queried sequentially based on their priority, with the default server
///     ranked last if it is enabled.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="primaryNtpServer"/> parameter is NULL.</para>
///     <para>EACCES: the calling application doesn't have the TimeSyncConfig capability.</para>
///     <para>ERANGE: The <paramref name="primaryNtpServer"/> or <paramref
///     name="secondaryNtpServer"/> length is greater than the maximum FQDN length or is not
///     null-terminated.</para>
///     <para>EINVAL: the <paramref name="primaryNtpServer"/> or <paramref
///     name="secondaryNtpServer"/> parameters are not valid addresses or the provided <paramref
///     name="option"/> parameter is invalid.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="primaryNtpServer">The primary NTP server address to use.</param>
/// <param name="secondaryNtpServer">The secondary NTP server addresss to use. This may be set to
/// NULL.</param>
/// <param name="option">Enables or disables the default NTP server.</param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_TimeSync_EnableCustomNtp(const char *primaryNtpServer,
                                        const char *secondaryNtpServer,
                                        Networking_NtpOption option);

/// <summary>
///     Gets the NTP server last used to successfully sync the device. The <see
///     cref="Networking_IsNetworkingReady"/> API can be used to determine when this API can be
///     called.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="inOutNtpServerLength"/> parameter provided is NULL but the
///     <paramref name="outNtpServer"/> parameter is not.</para>
///     <para>ENOENT: the device has not successfully completed a time sync.</para>
///     <para>ENOBUFS: the buffer is too small to receive the NTP server; the length will be
///     returned in  <paramref name="inOutNtpServerLength"/>.</para>
/// </summary>
/// <param name="outNtpServer">
///     A pointer to the character buffer that receives the NTP server hostname or IP address last
///     used to sync the device. This may be set to NULL.
/// </param>
/// <param name="inOutNtpServerLength">
///     A pointer to the number of bytes in the NTP server address buffer. If the buffer is not big
///     enough to hold the NTP server, the required length will be returned. This should be set to
///     NULL if <paramref name="outNtpServer"> is set to NULL.
/// </param>
/// <param name="outTimeBeforeSync">
///     A pointer to a struct that will be populated with the time prior to the last successful time
///     sync. This may be set to NULL.
/// </param>
/// <param name="outNtpTime">
///     A pointer to a struct that will be populated with the adjusted time of the last successful
///     time sync. This may be set to NULL.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_TimeSync_GetLastNtpSyncInfo(char *outNtpServer, size_t *inOutNtpServerLength,
                                           struct tm *outTimeBeforeSync, struct tm *outNtpTime);

/// <summary>
///     Enables or disables the time-sync service.
///     <para>The changes take effect immediately without a device reboot and persist through device
///     reboots. The time-sync service is then configured as requested at boot time. This function
///     allows applications to override the default behavior, which is to enable time-sync at boot
///     time.</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the calling application doesn't have the TimeSyncConfig capability.</para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="enabled">
///      true to enable the time-sync service; false to disable it.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_TimeSync_SetEnabled(bool enabled);

/// <summary>
///     Indicates whether the time-sync service is enabled. The application manifest must include
///     the TimeSyncConfig capability.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="outIsEnabled"/> parameter provided is null.</para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="outIsEnabled">
///      A pointer to a boolean value that receives the state of the time-sync service. The value
///      is set to true if the time-sync service is enabled, otherwise false.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_TimeSync_GetEnabled(bool *outIsEnabled);

/// <summary>
///     Gets the network connection status for a network interface.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="outStatus"/> parameter is NULL.</para>
///     <para>ENOENT: the <see cref="networkInterfaceName"/> interface does not exist.</para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="networkInterfaceName">
///      The name of the network interface.
/// </param>
/// <param name="outStatus">
///      A pointer to the <see cref="Networking_InterfaceConnectionStatus"/> enum that receives the
///      network connection status.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_GetInterfaceConnectionStatus(const char *networkInterfaceName,
                                            Networking_InterfaceConnectionStatus *outStatus);

/// <summary>
///     Initializes a <see cref="Networking_SntpServerConfig"/> struct with the default SNTP
///     Server configuration.
/// </summary>
/// <remarks>
///     When the <see cref="Networking_SntpServerConfig"/> struct is no longer needed, the
///     <see cref="Networking_SntpServerConfig_Destroy"/> function must be called on the struct to
///     avoid resource leaks.
/// </remarks>
/// <param name="sntpServerConfig">
///     A pointer to the <see cref="Networking_SntpServerConfig"/> struct that receives the
///     default SNTP server configuration.
/// </param>
void Networking_SntpServerConfig_Init(Networking_SntpServerConfig *sntpServerConfig);

/// <summary>
///     Destroys a <see cref="Networking_SntpServerConfig"/> struct.
/// </summary>
/// <remarks>
///     It's unsafe to call <see cref="Networking_SntpServerConfig_Destroy"/> on a struct that
///     hasn't been initialized. After <see cref="Networking_SntpServerConfig_Destroy"/> is called,
///     the <see cref="Networking_SntpServerConfig"/> struct must not be used until it is
///     re-initialized with the <see cref="Networking_SntpServerConfig_Init"/> function.
/// </remarks>
/// <param name="sntpServerConfig">
///     A pointer to the <see cref="Networking_SntpServerConfig"/> struct to destroy.
/// </param>
void Networking_SntpServerConfig_Destroy(Networking_SntpServerConfig *sntpServerConfig);

/// <summary>
///     Registers an SNTP server for a network interface. The application manifest must include the
///     SntpService capability.
///     <remarks>
///     If the SNTP server is already running and attached to the interface, this function returns
///     success. If the <see cref="networkInterfaceName"/> interface is down or disabled, this
///     function registers the SNTP server for the interface without starting the server.
///     </remarks>
///     <para>**Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the calling application doesn't have the SntpService capability.</para>
///     <para>EFAULT: the <paramref name="networkInterfaceName"/> parameter is NULL.</para>
///     <para>EFAULT: the <paramref name="sntpServerConfig"/> parameter is NULL.</para>
///     <para>ENOENT: the <paramref name="networkInterfaceName"/> parameter refers to an interface
///     that does not exist.</para>
///     <para>EPERM:  this operation is not allowed on the network interface.</para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="networkInterfaceName">
///      The name of the network interface to configure.
/// </param>
/// <param name="sntpServerConfig">
///      The pointer to the <see cref="Networking_SntpServerConfig"/> struct that represents the
///      SNTP server configuration.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_SntpServer_Start(const char *networkInterfaceName,
                                const Networking_SntpServerConfig *sntpServerConfig);

/// <summary>
///     Initializes a <see cref="Networking_DhcpServerConfig"/> struct with the default DHCP
///     server configuration.
/// </summary>
/// <remarks>
///     When the <see cref="Networking_DhcpServerConfig"/> struct is no longer needed, the
///     <see cref="Networking_DhcpServerConfig_Destroy"/> function must be called on the struct to
///     avoid resource leaks.
/// </remarks>
/// <param name="dhcpServerConfig">
///     A pointer to the <see cref="Networking_DhcpServerConfig"/> struct that receives the
///     default DHCP server configuration.
/// </param>
void Networking_DhcpServerConfig_Init(Networking_DhcpServerConfig *dhcpServerConfig);

/// <summary>
///     Destroys a <see cref="Networking_DhcpServerConfig"/> struct.
/// </summary>
/// <remarks>
///     It's unsafe to call <see cref="Networking_DhcpServerConfig_Destroy"/> on a struct that
///     hasn't been initialized. After <see cref="Networking_DhcpServerConfig_Destroy"/> is called,
///     the <see cref="Networking_DhcpServerConfig"/> struct must not be used until it is
///     re-initialized with the <see cref="Networking_DhcpServerConfig_Init"/> function.
/// </remarks>
/// <param name="dhcpServerConfig">
///     A pointer to the <see cref="Networking_DhcpServerConfig"/> struct to destroy.
/// </param>
void Networking_DhcpServerConfig_Destroy(Networking_DhcpServerConfig *dhcpServerConfig);

/// <summary>
///     Applies lease information to a <see cref="Networking_DhcpServerConfig"/> struct.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="dhcpServerConfig"/> parameter is NULL.</para>
///     <para>EINVAL: the parameter values are invalid or conflict with each other.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="dhcpServerConfig">
///     A pointer to the <see cref="Networking_DhcpServerConfig"/> struct to update.
/// </param>
/// <param name="startIpAddress">The starting IP address in the address range to lease.</param>
/// <param name="ipAddressCount">The number of IP addresses the server can lease.</param>
/// <param name="subnetMask">The subnet mask for the IP addresses.</param>
/// <param name="gatewayAddress">The gateway address for the network interface.</param>
/// <param name="leaseTimeInHours">The duration of the lease, in hours.</param>
/// <returns>
///     0 for success, -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_DhcpServerConfig_SetLease(Networking_DhcpServerConfig *dhcpServerConfig,
                                         struct in_addr startIpAddress, uint8_t ipAddressCount,
                                         struct in_addr subnetMask, struct in_addr gatewayAddress,
                                         uint32_t leaseTimeInHours);

/// <summary>
///     Applies a set of NTP server IP addresses to a <see cref="Networking_DhcpServerConfig"/>
///     struct.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="dhcpServerConfig"/> parameter is NULL.</para>
///     <para>EFAULT: the <paramref name="ntpServerAddresses"/> parameter is NULL.</para>
///     <para>EINVAL: more than three IP addresses were provided or the addresses are
///     invalid.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="dhcpServerConfig">
///     A pointer to the <see cref="Networking_DhcpServerConfig"/> struct to update.
/// </param>
/// <param name="ntpServerAddresses">A pointer to an array of NTP server IP addresses.</param>
/// <param name="serverCount">The number of IP addresses in the
///     <paramref name="ntpServerAddresses"/> array.</param>
/// <returns>
///     0 for success, -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_DhcpServerConfig_SetNtpServerAddresses(Networking_DhcpServerConfig *dhcpServerConfig,
                                                      const struct in_addr *ntpServerAddresses,
                                                      size_t serverCount);

/// <summary>
///     Registers, configures, and starts the DHCP server for a network interface. The configuration
///     specified by this function call overwrites the existing configuration. The application
///     manifest must include the DhcpService capability.
/// <remarks>
///     <para>If the network interface is up when this function is called, the DHCP server will be
///     shut down, configured, and started. If the interface is down, the server will start when the
///     interface is up.</para>
///     <para>The interface must be configured with a static IP address before this function is
///     called; otherwise, the EPERM error is returned.</para>
/// </remarks>
///     <para> **Errors** </para> If an error is encountered, returns -1 and sets errno to the error
///     value.
///     <para>EACCES: the application manifest does not include the DhcpService capability.</para>
///     <para>ENOENT: the <paramref name="networkInterfaceName"/> parameter refers to an interface
///     that does not exist.</para>
///     <para>EPERM:  the operation is not allowed on the network interface.</para>
///     <para>EFAULT: the <paramref name="networkInterfaceName"/> parameter is NULL.</para>
///     <para>EFAULT: the <paramref name="dhcpServerConfig"/> parameter is NULL.</para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     <para>EINVAL: the configuration struct has invalid parameters.</para>
///     <para>EFBIG: out of space to store the configuration.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="networkInterfaceName">
///      The name of the network interface to configure.
/// </param>
/// <param name="dhcpServerConfig">
///      The pointer to the <see cref="Networking_DhcpServerConfig"/> struct that represents the
///      DHCP server configuration
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_DhcpServer_Start(const char *networkInterfaceName,
                                const Networking_DhcpServerConfig *dhcpServerConfig);

/// <summary>
///     Sets the hardware address for an interface. The hardware address is persisted across
///     reboots, and can only be set on an Ethernet interface. The application manifest must
///     include the HardwareAddressConfig capability.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the application manifest does not include the HardwareAddressConfig
///     capability.</para>
///     <para>ENOENT: the network interface does not exist.</para>
///     <para>EPERM: this function is not allowed on the interface.</para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     <para>ERANGE: the <paramref name="hardwareAddressLength"/> is greater than
///     <see cref="HARDWARE_ADDRESS_LENGTH"/>.</para>
///     <para>EINVAL: the <paramref name="hardwareAddress"/> is invalid. Examples:
///       - An all zeroes hardware address (00:00:00:00:00:00).
///       - Group hardware addresses (a hardware address with its first octet's least significant
///       bit set to 1).
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkInterfaceName">
///      The name of the network interface to update.
/// </param>
/// <param name="hardwareAddress">
///      A pointer to an array of bytes containing the hardware address.
/// </param>
/// <param name="hardwareAddressLength">
///      The length of the hardware address. This should always be equal to <see
///      cref="HARDWARE_ADDRESS_LENGTH"/>.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int Networking_SetHardwareAddress(const char *networkInterfaceName,
                                         const uint8_t *hardwareAddress,
                                         size_t hardwareAddressLength);

/// <summary>
///      Retrieves the hardware address of the given network interface.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>ENOENT: the network interface does not exist.</para>
///     <para>EPERM: this function is not allowed on the interface.</para>
///     <para>EAGAIN: the networking stack isn't ready.</para>
///     <para>EINVAL: the <paramref name="outAddress"/> is invalid.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behavior will be retained through system updates.
/// </summary>
/// <param name="networkInterfaceName">
///      The name of the network interface to update.
/// </param>
/// <param name="outAddress">
///      A pointer to a <see cref="HardwareAddress"/> that receives the network interface's
///     hardware address.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static int Networking_GetHardwareAddress(const char *networkInterfaceName,
                                         Networking_Interface_HardwareAddress *outAddress);

#if !defined(AZURE_SPHERE_PUBLIC_SDK)

/// <summary>
///    The maximum length for the proxy address field (not including the null terminator)
/// </summary>
#define PROXY_ADDRESS_MAX_LENGTH 255

/// <summary>
///    The maximum length for the comma separated list containing host names or addresses which do
///    not use proxy
/// </summary>
#define PROXY_NO_PROXY_ADDRESSES_MAX_LENGTH 255

/// <summary>
///    The maximum length for the username field (not including the null terminator) to be used for
///    proxy authentication
/// </summary>
#define PROXY_USERNAME_MAX_LENGTH 63

/// <summary>
///    The maximum length for the password field (not including the null terminator) to be used for
///    proxy authentication
/// </summary>
#define PROXY_PASSWORD_MAX_LENGTH 63

typedef uint32_t Networking_ProxyOptions;
enum {
    /// <summary>The proxy is not configured.</summary>
    Networking_ProxyOptions_None = 1 << 0,
    /// <summary>The proxy is enabled.</summary>
    Networking_ProxyOptions_Enabled = 1 << 1,
};

typedef int8_t Networking_ProxyType;
enum {
    /// <summary>The proxy type is invalid.</summary>
    Networking_ProxyType_Invalid = -1,
    /// <summary>The proxy type is HTTP.</summary>
    Networking_ProxyType_HTTP = 0
};

typedef int8_t Networking_ProxyAuthType;
enum {
    /// <summary>The authentication type is invalid.</summary>
    Networking_ProxyAuthType_Invalid = -1,
    /// <summary>The authentication type is anonymous.</summary>
    Networking_ProxyAuthType_Anonymous = 0,
    /// <summary>The authentication type is basic (username and password are required).</summary>
    Networking_ProxyAuthType_Basic = 1
};

/// <summary>
///     Creates a <see cref="Networking_ProxyConfig"/> struct with the default proxy
///     configuration.
///     By default proxy configuration option Networking_ProxyOptions_Enabled is set.
///     By default the proxy type is Networking_ProxyType_HTTP.
///     <para> **Errors** </para>
///     If an error is encountered, returns NULL and sets errno to the error value.
///     <para>ENOMEM: Out of memory.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <remarks>
///     When the <see cref="Networking_ProxyConfig"/> struct is no longer needed, the
///     <see cref="Networking_Proxy_Destroy"/> function must be called on the struct to avoid
///     resource leaks.
/// </remarks>
/// <returns>
///     On success: A pointer to the <see cref="Networking_ProxyConfig"/> struct with default
///     proxy configuration.
///     On failure: NULL.
/// </returns>
Networking_ProxyConfig *Networking_Proxy_Create();

/// <summary>
///     Destroys a <see cref="Networking_ProxyConfig"/> previously created by <see
///     cref="Networking_Proxy_Create"/>.
/// </summary>
/// <remarks>
///     Once <see cref="Networking_Proxy_Destroy"/> returns, the pointer to <see
///     cref="Networking_ProxyConfig"/> is invalid and its use will have undefined behavior.
/// </remarks>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct to destroy.
/// </param>
void Networking_Proxy_Destroy(Networking_ProxyConfig *proxyConfig);

/// <summary>
///     Applies a proxy configuration to the device. The application manifest must
///     include the NetworkConfig capability.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the calling application doesn't have the NetworkConfig capability.</para>
///     <para>EFAULT: the <paramref name="proxyConfig"/> parameter is NULL.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behavior will be retained through system updates.
/// </summary>
/// <param name="proxyConfig">
///      The pointer to the <see cref="Networking_ProxyConfig"/> struct that contains the proxy
///      configuration to apply.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_Proxy_Apply(const Networking_ProxyConfig *proxyConfig);

/// <summary>
///     Gets a proxy configuration from the device. The application manifest must
///     include the NetworkConfig or ReadNetworkProxyConfig capability.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: the calling application doesn't have the NetworkConfig or
///     ReadNetworkProxyConfig capability.</para>
///     <para>EFAULT: the <paramref name="proxyConfig"/> parameter is
///     NULL.</para>
///     <para>ENOENT: there is currently no proxy configured</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behavior will be retained through system updates.
/// </summary>
/// <param name="proxyConfig">
///      The pointer to the <see cref="Networking_ProxyConfig"/> struct that will receive
///      the proxy configuration.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_Proxy_Get(Networking_ProxyConfig *proxyConfig);

/// <summary>
///     Sets proxy options for a <see cref="Networking_ProxyConfig"/> struct.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="proxyConfig"/> parameter is NULL.</para>
///     Any other errno may be specified; such errors aren't deterministic and there's no guarantee
///     that the same behavior will be retained through system updates.
/// </summary>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct to update.
/// </param>
/// <param name="proxyOptions">The proxy options to set.
///     NOTE: Disabling the proxy will by default preserve the proxy configuration so that later
///     the proxy can be reenabled without needing to provide the setting again.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_Proxy_SetProxyOptions(Networking_ProxyConfig *proxyConfig,
                                     Networking_ProxyOptions proxyOptions);

/// <summary>
///     Sets the proxy address for a <see cref="Networking_ProxyConfig"/> struct.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="proxyConfig"/> or <paramref name="proxyAddress"/>
///     parameter is NULL.</para>
///     <para>ERANGE: The <paramref name="proxyAddress"/> length is greater than <see
///     cref="PROXY_ADDRESS_MAX_LENGTH"/> or is not null-terminated.</para>
/// </summary>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct to update.
/// </param>
/// <param name="proxyAddress">
///     A pointer to a null-terminated string containing the IPv4 address of the proxy, like
///     "192.168.1.1", or the network name assigned to the proxy.
/// </param>
/// <param name="proxyPort">
///     The port to use on the proxy.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_Proxy_SetProxyAddress(Networking_ProxyConfig *proxyConfig, const char *proxyAddress,
                                     uint16_t proxyPort);

/// <summary>
///     Sets the proxy authentication in <see cref="Networking_ProxyConfig"/> struct to anonymous.
///     Previous information about other authentication methods (like username and password) which
///     might have been set earlier will be deleted.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="proxyConfig"/> parameter is NULL.</para>
/// </summary>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct to update.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_Proxy_SetAnonymousAuthentication(Networking_ProxyConfig *proxyConfig);

/// <summary>
///     Sets the proxy authentication in <see cref="Networking_ProxyConfig"/> struct to basic.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="proxyConfig"/>, <paramref name="username"/> or <paramref
///     name="password"/> parameter is NULL.</para>
///     <para>ERANGE: The <paramref name="username"/>
///     length is greater than <see cref="PROXY_USERNAME_MAX_LENGTH"/> or is not null-terminated, or
///     the <paramref name="password"/> length is greater than <see
///     cref="PROXY_PASSWORD_MAX_LENGTH"/> or is not null-terminated.</para>
/// </summary>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct to update.
/// </param>
/// <param name="username">
///     A pointer to the string containing the username to be used for authentication.
/// </param>
/// <param name="password">
///     A pointer to the string containing the password to be used for authentication.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_Proxy_SetBasicAuthentication(Networking_ProxyConfig *proxyConfig,
                                            const char *username, const char *password);

/// <summary>
///     Sets the list of host addresses in a <see cref="Networking_ProxyConfig"/> struct for which
///     proxy should not be used.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="proxyConfig"/> or <paramref
///     name="noProxyAddresses"/> parameter is NULL.</para>
///     <para>ERANGE: The <paramref name="noProxyAddresses"/>
///     length is greater than <see cref="PROXY_NO_PROXY_ADDRESSES_MAX_LENGTH"/> or is not
///     null-terminated.</para>
/// </summary>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct to update.
/// </param>
/// <param name="noProxyAddresses">
///     A pointer to a null-terminated string containing a comma-separated list of host
///     addresses/names.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_Proxy_SetProxyNoProxyAddresses(Networking_ProxyConfig *proxyConfig,
                                              const char *noProxyAddresses);

/// <summary>
///     Gets proxy options set on the proxy.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="proxyConfig"/> or <paramref
///     name="proxyOptions"/> parameter is NULL.</para>
/// </summary>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct with the proxy configuration.
/// </param>
/// <param name="proxyOptions">
///     A pointer to the <see cref="Networking_ProxyOptions"/> to get.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_Proxy_GetProxyOptions(const Networking_ProxyConfig *proxyConfig,
                                     Networking_ProxyOptions *proxyOptions);

/// <summary>
///     Gets the network address used by the proxy.
///     <para> **Errors** </para>
///     If an error is encountered, returns NULL and sets errno to the error value.
///     <para>EFAULT: the <paramref name="proxyConfig"/> parameter is NULL.</para>
/// </summary>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct with the proxy
///     configuration.
/// </param>
/// <returns>
///     On success: const pointer to the proxy address, valid until <see
///     cref="Networking_Proxy_Destroy"/> is called.
///     On failure: NULL.
/// </returns>
const char *Networking_Proxy_GetProxyAddress(const Networking_ProxyConfig *proxyConfig);

/// <summary>
///     Gets the network address and port used by the proxy.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="proxyConfig"/> or <paramref name="proxyPort"/> parameter
///     is NULL.</para>
/// </summary>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct with the proxy
///     configuration.
/// </param>
/// <param name="proxyPort">
///     A pointer to the location where the network port used by the proxy should be stored.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_Proxy_GetProxyPort(const Networking_ProxyConfig *proxyConfig, uint16_t *proxyPort);

/// <summary>
///     Gets the proxy type.
///     <para> **Errors** </para>
///     If an error is encountered, returns Networking_ProxyType_Invalid and sets errno to the error
///     value.
///     <para>EFAULT: the <paramref name="proxyConfig"/> parameter is NULL.</para>
/// </summary>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct with the proxy configuration.
/// </param>
/// <returns>
///     On success: the proxy type.
///     On failure: Networking_ProxyType_Invalid.
/// </returns>
Networking_ProxyType Networking_Proxy_GetProxyType(const Networking_ProxyConfig *proxyConfig);

/// <summary>
///     Gets the username for proxy authentication.
///     <para> **Errors** </para>
///     If an error is encountered, returns NULL and sets errno to the error value.
///     <para>EFAULT: the <paramref name="proxyConfig"/> parameter is NULL.</para>
/// </summary>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct with the proxy configuration.
/// </param>
/// <returns>
///     On success: const pointer to the username used for proxy authentication, valid until <see
///     cref="Networking_Proxy_Destroy"/> is called.
///     On failure: NULL.
/// </returns>
const char *Networking_Proxy_GetProxyUsername(const Networking_ProxyConfig *proxyConfig);

/// <summary>
///     Gets the password for proxy authentication.
///     <para> **Errors** </para>
///     If an error is encountered, returns NULL and sets errno to the error value.
///     <para>EFAULT: the <paramref name="proxyConfig"/> parameter is NULL.</para>
/// </summary>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct with the proxy configuration.
/// </param>
/// <returns>
///     On success: const pointer to the password used for proxy authentication, valid until <see
///     cref="Networking_Proxy_Destroy"/> is called.
///     On failure: NULL.
/// </returns>
const char *Networking_Proxy_GetProxyPassword(const Networking_ProxyConfig *proxyConfig);

/// <summary>
///     Gets the proxy authentication type.
///     <para> **Errors** </para>
///     If an error is encountered, returns Networking_ProxyAuthType_Invalid and sets errno to the
///     error value.
///     <para>EFAULT: the <paramref name="proxyConfig"/> parameter is NULL.</para>
/// </summary>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct with the proxy
///     configuration.
/// </param>
/// <returns>
///     On success: the proxy authentication type
///     On failure: Networking_ProxyAuthType_Invalid.
/// </returns>
Networking_ProxyAuthType Networking_Proxy_GetAuthType(const Networking_ProxyConfig *proxyConfig);

/// <summary>
///     Gets the comma-separated list of hosts for which proxy should not be used.
///     <para> **Errors** </para>
///     If an error is encountered, returns NULL and sets errno to the error value.
///     <para>EFAULT: the <paramref name="proxyConfig"/> or <paramref name="noProxyAddresses"/>
///     parameter is NULL.</para>
/// </summary>
/// <param name="proxyConfig">
///     A pointer to the <see cref="Networking_ProxyConfig"/> struct with the proxy configuration.
/// </param>
/// <returns>
///     On success: const pointer to the comma-separated list of hosts for which proxy should not be
///     used, valid until <see cref="Networking_Proxy_Destroy"/> is called.
///     On failure : NULL.
/// </returns>
const char *Networking_Proxy_GetNoProxyAddresses(const Networking_ProxyConfig *proxyConfig);

/// <summary>
///     Gets the proxy status.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="proxyStatus"/> parameter is NULL.</para>
/// </summary>
/// <param name="proxyStatus">
///     A pointer to the <see cref="Networking_ProxyStatus"/> to update.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Networking_Proxy_GetProxyStatus(Networking_ProxyStatus *proxyStatus);

#endif // !defined(AZURE_SPHERE_PUBLIC_SDK)

#ifdef __cplusplus
}
#endif
