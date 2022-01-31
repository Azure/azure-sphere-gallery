/// \file networking_structs_v1.h
/// \brief This header contains data structures and enumerations for the Networking library.
///
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <netinet/ip.h>
#include <net/if.h>

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     The length of a hardware address
/// </summary>
#define HARDWARE_ADDRESS_LENGTH 6

/// <summary>
///     The IP configuration options for a network interface.
/// </summary>
/// <remarks>
///     This enum is used by the <see cref="Networking_NetworkInterface"/> struct.
/// </remarks>
typedef enum {
    /// <summary>The interface doesn't have a DHCP client attached, so a static IP address must be
    /// enabled and then applied to the interface. To enable the static IP address, call
    /// <see cref="Networking_IpConfig_EnableStaticIp"/>. To apply it to the interface, call
    /// <see cref="Networking_IpConfig_Apply"/>.
    /// </summary>
    Networking_IpType_DhcpNone = 0,
    /// <summary>The interface has a DHCP client attached and is configured to use dynamic IP
    /// assignment.
    /// </summary>
    Networking_IpType_DhcpClient = 1
} Networking_IpType;

/// <summary>
///     Specifies the type for Networking_IpType enum values.
/// </summary>
typedef uint8_t Networking_IpType_Type;

/// <summary>
///     The valid network technologies used by a network interface.
/// </summary>
typedef enum {
    /// <summary>The network technology is unspecified.</summary>
    Networking_InterfaceMedium_Unspecified = 0,
    /// <summary>Wi-Fi.</summary>
    Networking_InterfaceMedium_Wifi = 1,
    /// <summary>Ethernet.</summary>
    Networking_InterfaceMedium_Ethernet = 2
} Networking_InterfaceMedium;

/// <summary>
///     Specifies the type for Networking_InterfaceMedium enum values.
/// </summary>
typedef uint8_t Networking_InterfaceMedium_Type;

/// <summary>
///     The properties of a network interface. This is an alias to a versioned structure. Define
///     NETWORKING_STRUCTS_VERSION to use this alias.
/// </summary>
struct z__Networking_NetworkInterface_v1 {
    /// <summary>A magic number that uniquely identifies the struct version.</summary>
    uint32_t z__magicAndVersion;
    /// <summary>Indicates whether the network interface is enabled.</summary>
    bool isEnabled;
    /// <summary>The network interface name including the null-terminator.</summary>
    char interfaceName[IF_NAMESIZE];
    /// <summary>reserved.</summary>
    uint32_t reserved;
    /// <summary>The <see cref="Networking_IpType"/> enum that contains the IP types for the
    /// interface.</summary>
    Networking_IpType_Type ipConfigurationType;
    /// <summary>The <see cref="Networking_InterfaceMedium"/> enum that contains the network types
    /// for the interface.</summary>
    Networking_InterfaceMedium_Type interfaceMediumType;
};

/// <summary>
///     An opaque buffer that represents the IP configuration for a network interface.
/// </summary>
typedef struct Networking_IpConfig {
    uint64_t reserved[5];
} Networking_IpConfig;

/// <summary>
///     A forward declaration representing an opaque buffer for the proxy configuration.
/// </summary>
typedef struct Networking_ProxyConfig Networking_ProxyConfig;

/// <summary>
///     An opaque buffer that represents the SNTP server configuration for a network interface.
/// </summary>
typedef struct Networking_SntpServerConfig {
    uint64_t reserved[3];
} Networking_SntpServerConfig;

/// <summary>
///     An opaque buffer that represents the DHCP server configuration for a network interface.
/// </summary>
typedef struct Networking_DhcpServerConfig {
    uint64_t reserved[8];
} Networking_DhcpServerConfig;

/// <summary>
///     A struct used when retrieving the hardware address of an interface
/// </summary>
typedef struct Networking_Interface_HardwareAddress {
    uint8_t address[HARDWARE_ADDRESS_LENGTH];
} Networking_Interface_HardwareAddress;

#ifdef __cplusplus
}
#endif
