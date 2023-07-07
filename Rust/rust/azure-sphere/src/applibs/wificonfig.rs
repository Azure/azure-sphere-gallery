//! The Applibs wificonfig module contains functions and types that manage Wi-Fi network configurations on a device.
use azure_sphere_sys::applibs::static_inline_helpers;
use azure_sphere_sys::applibs::wificonfig;
use std::io::Error;

// Note: SSID is Vec[u8] to avoid issues where SSIDs don't have to be well-formed utf8
// Note: BSSID is array of 6 u8 also to avoid utf8 encoding issues
// Note: identifiers such as WifiConfig_ClientIdentity are also treated as Vec<u8> rather than strings

const BSSID_SIZE: usize = 6;

/// The properties of a stored Wi-Fi network, which represents a 802.11 Service Set.
#[derive(Debug, Clone)]
pub struct StoredNetwork {
    /// The SSID.
    pub ssid: Vec<u8>,
    /// Indicates whether the network is enabled.
    pub is_enabled: bool,
    /// Indicates whether the network is connected.
    pub is_connected: bool,
    /// The security key setting.
    pub security: SecurityType,
}

/// The properties of a scanned Wi-Fi network, which represent a 802.11 Basic Service Set (BSS).
#[derive(Debug, Clone)]
pub struct ScannedNetwork {
    /// The SSID.
    pub ssid: Vec<u8>,
    /// The BSSID.
    pub bssid: [u8; BSSID_SIZE],
    /// The [`SecurityType`] value that specifies the security key setting.
    pub security: SecurityType,
    /// The BSS center frequency in MHz.
    pub frequency_mhz: u32,
    /// The RSSI (Received Signal Strength Indicator) value.
    pub signal_rssi: i8,
}

/// The security key setting for a Wi-Fi network.
#[repr(u8)]
#[derive(Debug, Clone, Copy)]
pub enum SecurityType {
    /// Unknown security setting.
    Unknown = wificonfig::WifiConfig_Security_WifiConfig_Security_Unknown as u8,
    /// No key management.
    Open = wificonfig::WifiConfig_Security_WifiConfig_Security_Open as u8,
    /// A WPA2 pre-shared key.
    Wpa2PSsk = wificonfig::WifiConfig_Security_WifiConfig_Security_Wpa2_Psk as u8,
    /// WPA2-Enterprise using EAP-TLS.
    Wpa2EAPTLS = wificonfig::WifiConfig_Security_WifiConfig_Security_Wpa2_EAP_TLS as u8,
}

/// For NetworkDiagnosticsError::AuthenticationFailed, information about why it failed
#[derive(Debug, Clone)]
pub struct AuthenticationFailedData {
    /// The certificate error. Note: There may be conditions where certError may not return an error.  See [WifiConfig_NetworkDiagnostics](https://learn.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-wificonfig/struct-wificonfig-networkdiagnostics) for details.
    pub cert_error: i32,
    /// The certificate's position in the certification chain. Meaningful only when certDepth is a non-negative (0 or positive) number.
    pub cert_depth: i32,
    /// The certificate's subject.
    pub cert_subject: Vec<u8>,
}

/// Reason for the network configuration error
#[derive(Debug, Clone)]
pub enum NetworkDiagnosticsError {
    /// No error
    Success,
    /// Generic error message when connection fails. For EAP-TLS networks, this error is potentially caused by not being able to reach the RADIUS server or using a client identity the RADIUS server does not recognize.
    ConnectionFailed,
    /// Network was not found.
    NetworkNotFound,
    /// Network password is missing.
    NoPskIncluded,
    /// Network is using an incorrect password.
    WrongKey,
    /// Authentication failed. This error applies only to EAP-TLS networks.
    AuthenticationFailed(AuthenticationFailedData),
    /// The stored network's security type does not match the available network.
    SecurityTypeMismatch,
    /// Network frequency not allowed.
    NetworkFrequencyNotAllowed,
    /// Network is not supported because no Extended Service Set (ESS), Personal Basic Service Set (PBSS),or Minimum Baseline Security Standard (MBSS) was detected.
    NetworkNotEssBpssMbss,
    /// Network is not supported.
    NetworkNotSupported,
    /// Network is not WPA2PSK, WPA2EAP, or Open.
    NetworkNonWpa,
    /// Unknown other error.  The value is included.
    Unknown(i32),
}

/// Information about the most recent failure to connect to a network.
pub struct NetworkDiagnostics {
    /// Indicates whether the network is enabled. This field indicates the current state of the network, not the state of the configuration. The value will be False if the network is temporarily disabled.
    pub is_enabled: bool,
    /// Indicates whether the network is connected.
    pub is_connected: bool,
    /// The reason for the most recent failure to connect to this network. See [WifiConfig_NetworkDiagnostics](https://learn.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-wificonfig/struct-wificonfig-networkdiagnostics) for details.
    pub error: NetworkDiagnosticsError,
    /// The OS time at which the error was recorded.
    pub timestamp: i64, // 64-bit time_t
}

/// The properties of a connected Wi-Fi network, which represent a 802.11 Basic Service Set (BSS).
#[derive(Debug, Clone)]
pub struct ConnectedNetwork {
    /// The SSID
    pub ssid: Vec<u8>,
    /// The BSSID.
    pub bssid: [u8; BSSID_SIZE],
    /// The [`SecurityType`] value that specifies the security key setting.
    pub security: SecurityType,
    /// The BSS center frequency in MHz.
    pub frequency_mhz: u32,
    /// The RSSI (Received Signal Strength Indicator) value.
    pub signal_rssi: i8,
}

fn ssid(ssid: &[u8; 32usize], ssid_length: u8) -> Vec<u8> {
    let ssid_length = std::cmp::max(ssid_length as usize, ssid.len());
    let mut ssid = Vec::<u8>::with_capacity(ssid_length);
    for i in 0..ssid_length as usize {
        ssid.push(ssid[i]);
    }
    ssid
}

pub(crate) unsafe fn vec_from_null_terminated_or_max(buf: &[u8]) -> Vec<u8> {
    let len = libc::strnlen(buf.as_ptr(), buf.len());
    let mut id = Vec::<u8>::with_capacity(len);
    for i in 0..len {
        id.push(buf[i]);
    }
    id
}

/// Gets the identifier of the stored client certificate for a Wi-Fi network.
pub fn client_cert_store_identifier(network_id: i32) -> Result<Vec<u8>, std::io::Error> {
    let mut identifier = static_inline_helpers::CertStore_Identifier {
        identifier: [0; static_inline_helpers::CERTSTORE_MAX_IDENTIFIER_LENGTH as usize + 1],
    };
    unsafe {
        let result = static_inline_helpers::WifiConfig_GetClientCertStoreIdentifier_inline(
            network_id,
            &mut identifier,
        );
        if result == 0 {
            Ok(vec_from_null_terminated_or_max(&identifier.identifier))
        } else {
            Err(Error::last_os_error())
        }
    }
}

/// Sets the identifier of the stored certificate to use as the client certificate for a Wi-Fi network. The setting is effective immediately but will be lost across a reboot unless the app calls [`persist_config`] after this function returns.
/// The application manifest must include the EnterpriseWifiConfig capability.
pub fn set_client_cert_store_identifier(
    network_id: i32,
    cert_store_identifer: &str,
) -> Result<(), std::io::Error> {
    let cert_store_identifer = std::ffi::CString::new(cert_store_identifer.as_bytes()).unwrap();
    let result = unsafe {
        static_inline_helpers::WifiConfig_SetClientCertStoreIdentifier_inline(
            network_id,
            cert_store_identifer.as_ptr(),
        )
    };
    if result == 0 {
        Ok(())
    } else {
        Err(Error::last_os_error())
    }
}

/// Gets the identifier of the stored RootCA certificate for a Wi-Fi network.
/// The application manifest must include the EnterpriseWifiConfig capability.
pub fn root_ca_store_identifier(network_id: i32) -> Result<Vec<u8>, std::io::Error> {
    let mut identifier = static_inline_helpers::CertStore_Identifier {
        identifier: [0; static_inline_helpers::CERTSTORE_MAX_IDENTIFIER_LENGTH as usize + 1],
    };
    unsafe {
        let result = static_inline_helpers::WifiConfig_GetRootCACertStoreIdentifier_inline(
            network_id,
            &mut identifier,
        );
        if result == 0 {
            Ok(vec_from_null_terminated_or_max(&identifier.identifier))
        } else {
            Err(Error::last_os_error())
        }
    }
}

/// Sets the identifier of the stored certificate to use as the root certificate authority for a network. If the identifier is not set, the device will not authenticate the server that it connects to. The setting is effective immediately but will be lost across a reboot unless the [`persist_config`] function is called after this function.
/// The application manifest must include the EnterpriseWifiConfig capability.
pub fn set_root_cat_cert_store_identifier(
    network_id: i32,
    cert_store_identifer: &str,
) -> Result<(), std::io::Error> {
    let cert_store_identifer = std::ffi::CString::new(cert_store_identifer.as_bytes()).unwrap();
    let result = unsafe {
        static_inline_helpers::WifiConfig_SetRootCACertStoreIdentifier_inline(
            network_id,
            cert_store_identifer.as_ptr(),
        )
    };
    if result == 0 {
        Ok(())
    } else {
        Err(Error::last_os_error())
    }
}

// WifiConfig_StoreOpenNetwork is deprecated. Use WifiConfig_AddNetwork() instead
// WifiConfig_StoreWpa2Network is deprecated. Use WifiConfig_AddNetwork() instead
// WifiConfig_ForgetNetwork is deprecated. Use WifiConfig_ForgetNetworkById() instead

/// Removes a Wi-Fi network from the device. Disconnects the device from the network if it's currently connected.
/// The setting is effective immediately but will be lost across a reboot unless the [`persist_config`] function is called after this function.
pub fn forget_network_by_id(network_id: u32) -> Result<(), std::io::Error> {
    let result = unsafe { wificonfig::WifiConfig_ForgetNetworkById(network_id as i32) };
    if result == 0 {
        Ok(())
    } else {
        Err(Error::last_os_error())
    }
}

/// Removes all stored Wi-Fi networks from the device. Disconnects the device from any connected network. This function is not thread safe.
///
/// The removal persists across device reboots.
pub fn forget_all_networks() -> Result<(), std::io::Error> {
    let result = unsafe { wificonfig::WifiConfig_ForgetAllNetworks() };
    if result == 0 {
        Ok(())
    } else {
        Err(Error::last_os_error())
    }
}

/// Gets the number of stored Wi-Fi networks on the device. This function is not thread safe.
/// The application manifest must include the WifiConfig capability.
pub fn stored_network_count() -> Result<usize, std::io::Error> {
    let result = unsafe { wificonfig::WifiConfig_GetStoredNetworkCount() };
    if result < 0 {
        Err(Error::last_os_error())
    } else {
        // Convert from ssize_t to u32 now that the failure value of -1 has been handled
        Ok(result as usize)
    }
}

/// Retrieves all stored Wi-Fi networks on the device. This function is not thread safe.
/// The application manifest must include the WifiConfig capability.
pub fn stored_networks() -> Result<Vec<StoredNetwork>, std::io::Error> {
    let count = stored_network_count()?;
    let mut networks =
        Vec::<static_inline_helpers::WifiConfig_StoredNetwork>::with_capacity(count as usize);

    let networks_ptr = networks.as_mut_ptr();
    let result =
        unsafe { static_inline_helpers::WifiConfig_GetStoredNetworks_inline(networks_ptr, count) };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        let mut new_networks = Vec::<StoredNetwork>::with_capacity(count as usize);
        for n in networks.iter() {
            let new_network = StoredNetwork {
                ssid: ssid(&n.ssid, n.ssidLength),
                is_enabled: n.isEnabled,
                is_connected: n.isConnected,
                security: security_type(n.security),
            };
            new_networks.push(new_network);
        }
        Ok(new_networks)
    }
}

fn security_type(ffi_security_type: u8) -> SecurityType {
    match ffi_security_type as u32 {
        wificonfig::WifiConfig_Security_WifiConfig_Security_Unknown => SecurityType::Unknown,
        wificonfig::WifiConfig_Security_WifiConfig_Security_Open => SecurityType::Open,
        wificonfig::WifiConfig_Security_WifiConfig_Security_Wpa2_Psk => SecurityType::Wpa2PSsk,
        wificonfig::WifiConfig_Security_WifiConfig_Security_Wpa2_EAP_TLS => {
            SecurityType::Wpa2EAPTLS
        }
        _ => SecurityType::Unknown,
    }
}

/// Gets a Wi-Fi network that is connected to the device. This function is not thread safe.
/// The application manifest must include the WifiConfig capability
pub fn current_network() -> Result<ConnectedNetwork, std::io::Error> {
    let mut current = static_inline_helpers::WifiConfig_ConnectedNetwork {
        z__magicAndVersion: 0,
        ssid: [0; static_inline_helpers::WIFICONFIG_SSID_MAX_LENGTH as usize],
        ssidLength: 0,
        bssid: [0; static_inline_helpers::WIFICONFIG_BSSID_BUFFER_SIZE as usize],
        security: wificonfig::WifiConfig_Security_WifiConfig_Security_Unknown as u8,
        frequencyMHz: 0,
        signalRssi: 0,
    };
    let result =
        unsafe { static_inline_helpers::WifiConfig_GetCurrentNetwork_inline(&mut current) };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        let new_connected = ConnectedNetwork {
            ssid: ssid(&current.ssid, current.ssidLength),
            bssid: current.bssid,
            security: security_type(current.security),
            frequency_mhz: current.frequencyMHz,
            signal_rssi: current.signalRssi,
        };
        Ok(new_connected)
    }
}

/// Gets the network ID of the currently connected network.
///
/// The network ID may change as network configurations are added and removed. If the device has been rebooted or network configurations have been added or removed, the application should retrieve the current network ID before it changes any stored network configurations.
/// The application manifest must include the WifiConfig capability.
pub fn connected_network_id() -> Result<u32, std::io::Error> {
    let network_id = unsafe { wificonfig::WifiConfig_GetConnectedNetworkId() };
    if network_id < 0 {
        Err(Error::last_os_error())
    } else {
        Ok(network_id as u32)
    }
}

/// Starts a scan to find all available Wi-Fi networks.
/// - This function is not thread safe
/// - This is a blocking call
///
/// The application manifest must include the WifiConfig capability.
pub fn trigger_scan_and_get_scanned_network_count() -> Result<usize, std::io::Error> {
    let count = unsafe { wificonfig::WifiConfig_TriggerScanAndGetScannedNetworkCount() };
    if count < 0 {
        Err(Error::last_os_error())
    } else {
        Ok(count as usize)
    }
}

/// Gets the Wi-Fi networks found by the last scan operation. This function is not thread safe.
/// The application manifest must include the WifiConfig capability.
pub fn trigger_scan_and_get_scanned_networks() -> Result<Vec<ScannedNetwork>, std::io::Error> {
    let count = trigger_scan_and_get_scanned_network_count()?;
    let mut networks =
        Vec::<static_inline_helpers::WifiConfig_ScannedNetwork>::with_capacity(count as usize);

    let networks_ptr = networks.as_mut_ptr();
    let result =
        unsafe { static_inline_helpers::WifiConfig_GetScannedNetworks_inline(networks_ptr, count) };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        let mut new_networks = Vec::<ScannedNetwork>::with_capacity(count as usize);
        for n in networks.iter() {
            let new_network = ScannedNetwork {
                ssid: ssid(&n.ssid, n.ssidLength),
                bssid: n.bssid,
                security: security_type(n.security),
                frequency_mhz: n.frequencyMHz,
                signal_rssi: n.signalRssi,
            };
            new_networks.push(new_network);
        }
        Ok(new_networks)
    }
}

/// Adds a Wi-Fi network to the device and returns the ID of the network.  Use set_*() functions to configure the network.
/// The new network isn't configured and can be configured with the set* functions. Changes to the network configuration are effective immediately but are lost when the device reboots unless the [`persist_config`] function is called to save the configuration to nonvolatile storage.
/// The number of networks you can store on a device is not fixed, but depends on the available resources and the amount of storage required for each network configuration.
/// The application manifest must include the WifiConfig capability.
pub fn add_network() -> Result<u32, std::io::Error> {
    let count = unsafe { wificonfig::WifiConfig_AddNetwork() };
    if count < 0 {
        Err(Error::last_os_error())
    } else {
        Ok(count as u32)
    }
}

/// Adds a new network that is a duplicate of the specified network with the specified ID. The new network is assigned the specified name and is disabled by default.
/// The number of networks you can store on a device is not fixed, but depends on the available resources and the amount of storage required for each network configuration.
/// The application manifest must include the WifiConfig capability.
pub fn add_duplicate_network(network_id: u32, config_name: &str) -> Result<u32, std::io::Error> {
    let new_network_id = unsafe {
        wificonfig::WifiConfig_AddDuplicateNetwork(network_id as i32, config_name.as_ptr())
    };
    if new_network_id < 0 {
        Err(Error::last_os_error())
    } else {
        Ok(new_network_id as u32)
    }
}

/// Sets the SSID for a Wi-Fi network.
/// Changes to the network configuration are effective immediately but are lost when the device reboots unless the [`persist_config`] function is called to save the configuration to nonvolatile storage.
/// The application manifest must include the WifiConfig capability.
pub fn set_ssid(network_id: u32, ssid: &[u8]) -> Result<(), std::io::Error> {
    let result = unsafe {
        static_inline_helpers::WifiConfig_SetSSID_inline(
            network_id as i32,
            ssid.as_ptr(),
            ssid.len(),
        )
    };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Sets the security type for a Wi-Fi network.
/// Changes to the network configuration are effective immediately but are lost when the device reboots unless the [`persist_config`] function is called to save the configuration to nonvolatile storage.
/// The application manifest must include the WifiConfig capability.
pub fn set_security_type(
    network_id: u32,
    security_type: SecurityType,
) -> Result<(), std::io::Error> {
    let security_type = match security_type {
        SecurityType::Open => static_inline_helpers::WifiConfig_Security_WifiConfig_Security_Open,
        SecurityType::Wpa2PSsk => {
            static_inline_helpers::WifiConfig_Security_WifiConfig_Security_Wpa2_Psk
        }
        SecurityType::Wpa2EAPTLS => {
            static_inline_helpers::WifiConfig_Security_WifiConfig_Security_Wpa2_EAP_TLS
        }
        _ => static_inline_helpers::WifiConfig_Security_WifiConfig_Security_Unknown,
    };
    let result = unsafe {
        static_inline_helpers::WifiConfig_SetSecurityType_inline(
            network_id as i32,
            security_type as u8,
        )
    };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Enables or disables a Wi-Fi network configuration.
/// Changes to the network configuration are effective immediately but are lost when the device reboots unless the [`persist_config`] function is called to save the configuration to nonvolatile storage.
/// The application manifest must include the WifiConfig capability.
pub fn set_enabled(network_id: u32, enabled: bool) -> Result<(), std::io::Error> {
    let result = unsafe {
        static_inline_helpers::WifiConfig_SetNetworkEnabled_inline(network_id as i32, enabled)
    };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Writes the current network configuration to nonvolatile storage so that it persists over a device reboot. This function doesn't reload the current configuration; call [`reload_config`] to reload.
/// The application manifest must include the WifiConfig capability.
pub fn persist_config() -> Result<(), std::io::Error> {
    let result = unsafe { static_inline_helpers::WifiConfig_PersistConfig_inline() };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Reloads the current network configuration from nonvolatile storage. Any unsaved configuration will be lost.
/// The application manifest must include the WifiConfig capability.
pub fn reload_config() -> Result<(), std::io::Error> {
    let result = unsafe { static_inline_helpers::WifiConfig_ReloadConfig_inline() };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Sets the pre-shared key (PSK) for a Wi-Fi network. The PSK is used for networks that are configured with the Wpa2_Psk security type.
/// Changes to the network configuration are effective immediately but are lost when the device reboots unless the [`persist_config`] function is called to save the configuration to nonvolatile storage.
/// The application manifest must include the WifiConfig capability.
pub fn set_psk(network_id: u32, psk: &[u8]) -> Result<(), std::io::Error> {
    let result = unsafe {
        static_inline_helpers::WifiConfig_SetPSK_inline(network_id as i32, psk.as_ptr(), psk.len())
    };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Enables or disables targeted scanning for a network. Targeted scanning is disabled by default.
/// Targeted scanning is used to connect to access points that aren't broadcasting their SSID, or are in a noisy environment.
/// **Note**
/// Targeted scanning causes the device to transmit probe requests that may reveal the SSID of the network to other devices. This should only be used in controlled environments, or on networks where this an acceptable risk.
///
/// Changes to the network configuration are effective immediately but are lost when the device reboots unless the [`persist_config`] function is called to save the configuration to nonvolatile storage.
/// The application manifest must include the WifiConfig capability.
pub fn set_targeted_scan_enabled(network_id: u32, enabled: bool) -> Result<(), std::io::Error> {
    let result = unsafe {
        static_inline_helpers::WifiConfig_SetTargetedScanEnabled_inline(network_id as i32, enabled)
    };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Gets the client identity of the network.
/// The application manifest must include the EnterpriseWifiConfig capability.
pub fn client_identity(network_id: u32) -> Result<Vec<u8>, std::io::Error> {
    let mut identity = static_inline_helpers::WifiConfig_ClientIdentity {
        identity: [0; static_inline_helpers::WIFICONFIG_EAP_IDENTITY_MAX_LENGTH as usize + 1],
    };
    unsafe {
        let result = static_inline_helpers::WifiConfig_GetClientIdentity_inline(
            network_id as i32,
            &mut identity,
        );
        if result == 0 {
            Ok(vec_from_null_terminated_or_max(&identity.identity))
        } else {
            Err(Error::last_os_error())
        }
    }
}

/// Sets the client identity for a network.
/// The application manifest must include the EnterpriseWifiConfig capability.
pub fn set_client_identity(network_id: u32, identity: &[u8]) -> Result<(), std::io::Error> {
    if identity[identity.len() - 1] != 0u8 {
        // the value isn't null-terminated
        return Err(std::io::Error::from_raw_os_error(libc::EINVAL));
    }
    let result = unsafe {
        static_inline_helpers::WifiConfig_SetClientIdentity_inline(
            network_id as i32,
            identity.as_ptr(),
        )
    };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Sets a name for a network configuration. The name can be used as a convenient handle to identify a network configuration. It is strongly recommended that this name be unique.
/// Changes to the network configuration are effective immediately but are lost when the device reboots unless the [`persist_config`] function is called to save the configuration to nonvolatile storage.
/// The application manifest must include the WifiConfig capability.
pub fn set_config_name(network_id: u32, config_name: &[u8]) -> Result<(), std::io::Error> {
    if config_name[config_name.len() - 1] != 0u8 {
        // the value isn't null-terminated
        return Err(std::io::Error::from_raw_os_error(libc::EINVAL));
    }
    let result = unsafe {
        static_inline_helpers::WifiConfig_SetConfigName_inline(
            network_id as i32,
            config_name.as_ptr(),
        )
    };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Gets the network ID for the network configuration with the given name. Use [`set_config_name`]to assign the network name. The network ID may change as network configurations are added and removed, so apps should get the network ID again before changing a stored network configuration.
/// The application manifest must include the WifiConfig capability.
pub fn network_id_by_config_name(config_name: &[u8]) -> Result<u32, std::io::Error> {
    if config_name[config_name.len() - 1] != 0u8 {
        // the value isn't null-terminated
        return Err(std::io::Error::from_raw_os_error(libc::EINVAL));
    }
    let result = unsafe { wificonfig::WifiConfig_GetNetworkIdByConfigName(config_name.as_ptr()) };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(result as u32)
    }
}

/// Gets diagnostic information about the most recent error on a specified network
/// The application manifest must include the WifiConfig capability.
pub fn network_diagnostics(network_id: u32) -> Result<NetworkDiagnostics, std::io::Error> {
    unsafe {
        let mut diag = static_inline_helpers::WifiConfig_NetworkDiagnostics {
            isEnabled: 0,
            isConnected: 0,
            error: 0,
            timestamp: 0,
            certError: 0,
            certDepth: 0,
            certSubject: static_inline_helpers::CertStore_SubjectName {
                name: [0u8; static_inline_helpers::CERTSTORE_SUBJECTNAME_MAX_LENGTH as usize + 1],
            },
        };
        let result = static_inline_helpers::WifiConfig_GetNetworkDiagnostics_inline(
            network_id as i32,
            &mut diag,
        );
        if result == -1 {
            Err(std::io::Error::last_os_error())
        } else {
            let new_error = match diag.error {
                0 => NetworkDiagnosticsError::Success,
                1 => NetworkDiagnosticsError::ConnectionFailed,
                2 => NetworkDiagnosticsError::NetworkNotFound,
                3 => NetworkDiagnosticsError::NoPskIncluded,
                4 => NetworkDiagnosticsError::WrongKey,
                5 => NetworkDiagnosticsError::AuthenticationFailed(AuthenticationFailedData {
                    cert_error: diag.certError,
                    cert_depth: diag.certDepth,
                    cert_subject: vec_from_null_terminated_or_max(&diag.certSubject.name),
                }),
                6 => NetworkDiagnosticsError::SecurityTypeMismatch,
                7 => NetworkDiagnosticsError::NetworkFrequencyNotAllowed,
                8 => NetworkDiagnosticsError::NetworkNotEssBpssMbss,
                9 => NetworkDiagnosticsError::NetworkNotSupported,
                10 => NetworkDiagnosticsError::NetworkNonWpa,
                _ => NetworkDiagnosticsError::Unknown(diag.error),
            };

            let new_diag = NetworkDiagnostics {
                is_enabled: if diag.isEnabled == 0 { false } else { true },
                is_connected: if diag.isConnected == 0 { false } else { true },
                error: new_error,
                timestamp: diag.timestamp,
            };
            Ok(new_diag)
        }
    }
}

/// Enable WiFi power savings
/// The application manifest must include the WifiConfig capability.
pub fn set_power_savings_enabled(enabled: bool) -> Result<(), std::io::Error> {
    let result =
        unsafe { static_inline_helpers::WifiConfig_SetPowerSavingsEnabled_inline(enabled) };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}
