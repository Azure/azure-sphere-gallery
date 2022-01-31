/// \file wificonfig_structs_v1.h
/// \brief This header contains data structures and enumerations for the WiFiConfig library.
///
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     The maximum number of bytes for a Wi-Fi SSID.
/// </summary>
#define WIFICONFIG_SSID_MAX_LENGTH 32
/// <summary>
///     The maximum number of characters for a WPA2 key. The key can be either a
///     passphrase of 8-63 ASCII characters or 64 hexadecimal digits.
/// </summary>
#define WIFICONFIG_WPA2_KEY_MAX_BUFFER_SIZE 64
/// <summary>
///     The buffer size for a Wi-Fi BSSID.
/// </summary>
#define WIFICONFIG_BSSID_BUFFER_SIZE 6

/// <summary>
///     The security key setting for a Wi-Fi network.
/// </summary>
typedef enum {
    /// <summary>Unknown security setting.</summary>
    WifiConfig_Security_Unknown = 0,
    /// <summary>No key management.</summary>
    WifiConfig_Security_Open = 1,
    /// <summary>A WPA2 pre-shared key.</summary>
    WifiConfig_Security_Wpa2_Psk = 2,
    /// <summary>WPA2-Enterprise using EAP-TLS.</summary>
    WifiConfig_Security_Wpa2_EAP_TLS = 3,
} WifiConfig_Security;

typedef uint8_t WifiConfig_Security_Type;

/// <summary>
///     The properties of a scanned Wi-Fi network, which represent a 802.11 Basic
///     Service Set (BSS).
///
///     After you define WIFICONFIG_STRUCTS_VERSION, you can use the
///     WifiConfig_ScannedNetwork alias to access this structure.
/// </summary>
struct z__WifiConfig_ScannedNetwork_v1 {
    /// <summary>A magic number that uniquely identifies the struct version.</summary>
    uint32_t z__magicAndVersion;
    /// <summary>The fixed length buffer that contains the SSID.</summary>
    uint8_t ssid[WIFICONFIG_SSID_MAX_LENGTH];
    /// <summary>The fixed length buffer that contains the BSSID.</summary>
    uint8_t bssid[WIFICONFIG_BSSID_BUFFER_SIZE];
    /// <summary>The size of the SSID element in bytes.</summary>
    uint8_t ssidLength;
    /// <summary>The <see cref="WifiConfig_Security" /> value that specifies the security key
    /// setting.</summary>
    WifiConfig_Security_Type security;
    /// <summary>The BSS center frequency in MHz.</summary>
    uint32_t frequencyMHz;
    /// <summary>The RSSI (Received Signal Strength Indicator) value.</summary>
    int8_t signalRssi;
};

/// <summary>
///     The properties of a stored Wi-Fi network, which represents a 802.11 Service Set.
///
///     After you define WIFICONFIG_STRUCTS_VERSION, you can use the WifiConfig_StoredNetwork alias
///     to access this structure.
/// </summary>
struct z__WifiConfig_StoredNetwork_v1 {
    /// <summary>A magic number that uniquely identifies the struct version.</summary>
    uint32_t z__magicAndVersion;
    /// <summary>The fixed length buffer that contains the SSID.</summary>
    uint8_t ssid[WIFICONFIG_SSID_MAX_LENGTH];
    /// <summary>The size of the SSID element in bytes.</summary>
    uint8_t ssidLength;
    /// <summary>Indicates whether the network is enabled.</summary>
    bool isEnabled;
    /// <summary>Indicates whether the network is connected.</summary>
    bool isConnected;
    /// <summary>The <see cref="WifiConfig_Security" /> value that specifies the security key
    /// setting.</summary>
    WifiConfig_Security_Type security;
};

/// <summary>
///     The properties of a connected Wi-Fi network, which represent a 802.11 Basic Service
///     Set (BSS).
///
///     After you define WIFICONFIG_STRUCTS_VERSION, you can use the
///     WifiConfig_ConnectedNetwork alias to access this structure.
/// </summary>
struct z__WifiConfig_ConnectedNetwork_v1 {
    /// <summary>A magic number that uniquely identifies the struct version.</summary>
    uint32_t z__magicAndVersion;
    /// <summary>The fixed length buffer that contains the SSID.</summary>
    uint8_t ssid[WIFICONFIG_SSID_MAX_LENGTH];
    /// <summary>The fixed length buffer that contains the BSSID.</summary>
    uint8_t bssid[WIFICONFIG_BSSID_BUFFER_SIZE];
    /// <summary>The size of the SSID element in bytes.</summary>
    uint8_t ssidLength;
    /// <summary>The <see cref="WifiConfig_Security" /> value that specifies the security key
    /// setting.</summary>
    WifiConfig_Security_Type security;
    /// <summary>The BSS center frequency in MHz.</summary>
    uint32_t frequencyMHz;
    /// <summary>The RSSI (Received Signal Strength Indicator) value.</summary>
    int8_t signalRssi;
};

#ifdef __cplusplus
}
#endif
