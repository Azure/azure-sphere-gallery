//! The Applibs networking module contains functions and types that interact with the networking subsystem to query the network state, and to get and set the network service configuration.
use azure_sphere_sys::applibs::networking;
use azure_sphere_sys::applibs::static_inline_helpers;
use bitmask_enum::bitmask;
use chrono::{DateTime, NaiveDateTime, Utc};
use std::ffi::{CStr, CString};
use std::io::Error;

/// An option to enable or disable the default NTP server to use as a fallback.
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum NtpOption {
    /// Enable the default NTP server along with a custom or automatic NTP server.
    FallbackServerEnabled,
    /// Disable the default NTP server.
    FallbackServerDisabled,
}

/// A bit mask that specifies the connection status of a network interface.
#[bitmask(u32)]
pub enum ConnectionStatus {
    /// The interface is enabled.
    InterfaceUp = networking::Networking_InterfaceConnectionStatus_InterfaceUp,
    /// The interface is connected to a network.
    ConnectedToNetwork = networking::Networking_InterfaceConnectionStatus_ConnectedToNetwork,
    /// The interface has an IP address assigned to it.
    IpAvailable = networking::Networking_InterfaceConnectionStatus_IpAvailable,
    /// The interface is connected to the internet.
    ConnectedToInternet = networking::Networking_InterfaceConnectionStatus_ConnectedToInternet,
}

/// A bit mask that specifies the proxy configuration status.
#[bitmask(u32)]
pub enum ProxyOptions {
    /// The proxy is not configured.
    None = networking::Networking_ProxyOptions_None,
    /// The proxy is enabled.
    Enabled = networking::Networking_ProxyOptions_Enabled,
}

/// The proxy type.
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum ProxyType {
    /// The proxy type is invalid.
    Invalid = networking::Networking_ProxyType_Invalid as isize,
    /// The proxy type is HTTP.
    Http = networking::Networking_ProxyType_HTTP as isize,
}

/// The proxy authentication type.
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum AuthType {
    /// The authentication type is invalid.
    Invalid = networking::Networking_ProxyType_Invalid as isize,
    /// The authentication type is anonymous.
    Anonymous = networking::Networking_ProxyAuthType_Anonymous as isize,
    /// The authentication type is basic (username and password are required).
    Basic = networking::Networking_ProxyAuthType_Basic as isize,
}

/// A bit mask that specifies the proxy status.
#[bitmask(u32)]
pub enum ProxyStatus {
    /// The proxy configuration is enabled.
    Enabled = networking::Networking_ProxyStatus_Enabled,
    /// The proxy name is being resolved.
    ResolvingProxyName = networking::Networking_ProxyStatus_ResolvingProxyName,
    /// The proxy is ready.
    Ready = networking::Networking_ProxyStatus_Ready,
}

/// Verifies whether the network interface is up, connected to an access point, has an IP address, and the time is synced. It does not check whether there is actual internet connectivity.
pub fn is_networking_ready() -> Result<bool, std::io::Error> {
    unsafe {
        let mut is_ready: bool = false;
        let r = networking::Networking_IsNetworkingReady(&mut is_ready);
        if r == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(is_ready)
        }
    }
}

/// Gets the number of network interfaces in an Azure Sphere device.
/// The number of interfaces in the system will not change within a boot cycle.
pub fn interface_count() -> Result<usize, std::io::Error> {
    let count = unsafe { networking::Networking_GetInterfaceCount() };
    if count == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(count as usize)
    }
}

// Gets the list of network interfaces in an Azure Sphere device.
pub fn interfaces() -> Result<Vec<networking::Networking_NetworkInterface>, std::io::Error> {
    let count = self::interface_count()?;
    let mut v = Vec::<networking::Networking_NetworkInterface>::with_capacity(count as usize);
    unsafe {
        let vptr = v.as_mut_ptr();
        let vptr = vptr as *mut static_inline_helpers::Networking_NetworkInterface;
        let r = static_inline_helpers::Networking_GetInterfaces_inline(vptr, count);
        if r == -1 {
            Err(Error::last_os_error())
        } else {
            v.set_len(r as usize);
            v.truncate(r as usize);
            v.shrink_to_fit();
            Ok(v)
        }
    }
}

/// The number of interfaces in the system will not change within a boot cycle.
pub fn set_interface_state(interface_name: &str, is_enabled: bool) -> Result<(), std::io::Error> {
    let interface_name = std::ffi::CString::new(interface_name.as_bytes()).unwrap();
    let r = unsafe {
        networking::Networking_SetInterfaceState(
            interface_name.as_ptr() as *const libc::c_char,
            is_enabled,
        )
    };
    if r == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Releases the device dynamic IP address.
/// If an IP had been acquired via DHCP, this API synchronously sends out the DHCP release message, but there is no guarantee that it is received. This API stops the DHCP protocol from attempting to acquire an IP address until [`renew_ip`] is called.
/// The application manifest must include the NetworkConfig capability.
pub fn release_ip(interface_name: &str) -> Result<(), std::io::Error> {
    let interface_name = std::ffi::CString::new(interface_name.as_bytes()).unwrap();
    let r = unsafe {
        networking::Networking_IpConfig_ReleaseIp(interface_name.as_ptr() as *const libc::c_char)
    };
    if r == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Renews the device dynamic IP address lease.
/// If an IP address was acquired via DHCP, this function asynchronously renews the current IP address lease. This function does nothing if DHCP is still working to acquire an IP address. If the DHCP protocol has been stopped with [`release_ip`], the DHCP transaction will be re-started. Use [`is_networking_ready`] to determine if the new IP is acquired.
/// The application manifest must include the NetworkConfig capability.
pub fn renew_ip(interface_name: &str) -> Result<(), std::io::Error> {
    let interface_name = std::ffi::CString::new(interface_name.as_bytes()).unwrap();
    let r = unsafe {
        networking::Networking_IpConfig_RenewIp(interface_name.as_ptr() as *const libc::c_char)
    };
    if r == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Instructs the device to use the original OS default NTP server for time sync.
pub fn enable_default_ntp() -> Result<(), std::io::Error> {
    let r = unsafe { networking::Networking_TimeSync_EnableDefaultNtp() };
    if r == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Attempts to obtain and use NTP server addresses from DHCP option 042. The NTP servers obtained from DHCP are queried sequentially based on their priority, with the default server ranked last if it is enabled.
pub fn enable_automatic_ntp(option: NtpOption) -> Result<(), std::io::Error> {
    let option = match option {
        NtpOption::FallbackServerDisabled => {
            networking::Networking_NtpOption_FallbackServerDisabled
        }
        NtpOption::FallbackServerEnabled => networking::Networking_NtpOption_FallbackServerEnabled,
    };
    let r = unsafe { networking::Networking_TimeSync_EnableAutomaticNtp(option) };
    if r == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Instructs the device to use the NTP server addresses provided by the user. Up to two host names or IP addresses may be specified and up to three will be attempted if the default server is enabled.
pub fn enable_custom_ntp(
    primary_ntp_server: &String,
    secondary_ntp_server: Option<&String>,
    option: NtpOption,
) -> Result<(), std::io::Error> {
    let primary_ntp_server = CString::new(primary_ntp_server.as_bytes()).unwrap();
    let option = match option {
        NtpOption::FallbackServerDisabled => {
            networking::Networking_NtpOption_FallbackServerDisabled
        }
        NtpOption::FallbackServerEnabled => networking::Networking_NtpOption_FallbackServerEnabled,
    };
    let r = unsafe {
        if let Some(secondary) = secondary_ntp_server {
            let secondary = CString::new(secondary.as_bytes()).unwrap();
            networking::Networking_TimeSync_EnableCustomNtp(
                primary_ntp_server.as_ptr(),
                secondary.as_ptr(),
                option,
            )
        } else {
            networking::Networking_TimeSync_EnableCustomNtp(
                primary_ntp_server.as_ptr(),
                std::ptr::null() as *const libc::c_char,
                option,
            )
        }
    };
    if r == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

fn convert_tm_to_datetime(tm: &mut networking::tm) -> DateTime<Utc> {
    let timestamp = unsafe { networking::mktime(tm) };
    let naive =
        NaiveDateTime::from_timestamp_opt(timestamp / 1000, (timestamp % 1000) as u32 * 1_000_000)
            .unwrap();
    DateTime::from_naive_utc_and_offset(naive, Utc)
}

/// Gets the NTP server last used to successfully sync the device. The [`is_networking_ready`] API can be used to determine when this API can be called.
pub fn get_last_ntp_sync_info(
    out_time_before_sync: Option<&mut DateTime<Utc>>,
    out_ntp_time: Option<&mut DateTime<Utc>>,
) -> Result<Option<String>, std::io::Error> {
    unsafe {
        let mut length = 0;
        let r = networking::Networking_TimeSync_GetLastNtpSyncInfo(
            std::ptr::null_mut(),
            &mut length,
            std::ptr::null_mut(),
            std::ptr::null_mut(),
        );
        if r == -1 {
            let e = Error::last_os_error();
            if e.raw_os_error().unwrap() == libc::ENOENT {
                return Ok(None);
            } else {
                return Err(e);
            }
        }
        let mut tbs_value = networking::tm {
            tm_gmtoff: 0,
            tm_hour: 0,
            tm_isdst: 0,
            tm_mday: 0,
            tm_min: 0,
            tm_mon: 0,
            tm_sec: 0,
            tm_wday: 0,
            tm_yday: 0,
            tm_year: 0,
            tm_zone: std::ptr::null_mut(),
        };
        let mut ntp_value: networking::tm = tbs_value;
        let tbs = if out_time_before_sync.is_some() {
            &mut tbs_value
        } else {
            std::ptr::null_mut()
        };
        let ntp = if out_ntp_time.is_some() {
            &mut ntp_value
        } else {
            std::ptr::null_mut()
        };
        let mut server = vec![0 as u8; length as usize];
        let r = networking::Networking_TimeSync_GetLastNtpSyncInfo(
            server.as_mut_ptr(),
            &mut length,
            tbs,
            ntp,
        );
        if r == -1 {
            Err(Error::last_os_error())
        } else {
            let ntp_server = String::from_utf8_lossy(&server);
            let ntp_server = ntp_server.into();
            if let Some(t) = out_time_before_sync {
                *t = convert_tm_to_datetime(&mut tbs_value);
            }
            if let Some(t) = out_ntp_time {
                *t = convert_tm_to_datetime(&mut ntp_value);
            }
            Ok(Some(ntp_server))
        }
    }
}

/// The changes take effect immediately without a device reboot and persist through device reboots. The time-sync service is then configured as requested at boot time. This function allows applications to override the default behavior, which is to enable time-sync at boot time.
/// The application manifest must include the TimeSyncConfig capability.
pub fn set_timesync_enabled(enabled: bool) -> Result<(), std::io::Error> {
    let r = unsafe { networking::Networking_TimeSync_SetEnabled(enabled) };
    if r == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Indicates whether the time-sync service is enabled.
pub fn timesync_enabled() -> Result<bool, std::io::Error> {
    unsafe {
        let mut is_enabled = false;
        let r = networking::Networking_TimeSync_GetEnabled(&mut is_enabled);
        if r == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(is_enabled)
        }
    }
}

/// Gets the network connection status for a network interface. When it is called, the OS returns the last known status and attempts to update the specified interface status in the Networking_InterfaceConnectionStatus enum.
///
/// The [`ConnectionStatus`] enum returned reflects the last known status of the interface, network connectivity, IP address, and internet connection. When it is called, the OS attempts to update these status flags and make a call to [http://www.msftconnecttest.com](http://www.msftconnecttest.comm) as noted in Azure Sphere OS networking requirements, in order to determine if the device has internet connectivity.
///
/// An application may take action based on the Networking_InterfaceConnectionStatus_ConnectedToInternet status, which indicates if the device is connected to the internet. However, for various reasons, network or internet connectivity might be lost between the time the status was updated and the time the application attempts to connect to the internet. Therefore, the application should include logic that allows for smart choices when encountering changes in network and internet availability. The application should handle connection errors and adapt accordingly.
///
///If the status returned indicates that the device is not connected to the internet, the application may call it again in order to determine when the device status is changed.
///
///If [`ConnectionStatus::ConnectedToInternet`] indicates that the device is connected to the internet, the device should not poll for status more than once every 90 seconds. If this connection status function repeatedly returns the indication that the device is connected to the internet, the OS throttles the status check if the polling interval is too short. The recommended application polling interval is one request/two minutes.
pub fn get_interface_connection_status(
    interface_name: &str,
) -> Result<ConnectionStatus, std::io::Error> {
    // Null-terminate the interface_name
    let interface_name = std::ffi::CString::new(interface_name.as_bytes()).unwrap();

    unsafe {
        let mut status: networking::Networking_InterfaceConnectionStatus = 0;
        let r = networking::Networking_GetInterfaceConnectionStatus(
            interface_name.as_ptr(),
            &mut status,
        );
        if r == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(ConnectionStatus::from(status))
        }
    }
}

/// Configure TCP/IP networking.  Call [`IpConfig::apply`] to apply the changes to the device.
#[derive(Debug, Clone)]
pub struct IpConfig {
    ipconfig: networking::Networking_IpConfig,
    /// This ensures that the ADC Controller cannot be used across threads
    _threading: core::marker::PhantomData<*const ()>,
}

impl IpConfig {
    pub fn new() -> Self {
        let reserved: [u64; 5] = [0, 0, 0, 0, 0];
        let mut ipconfig = networking::Networking_IpConfig { reserved };
        unsafe { networking::Networking_IpConfig_Init(&mut ipconfig) };
        Self {
            ipconfig,
            _threading: Default::default(),
        }
    }

    /// Enables dynamic IP and disables static IP
    pub fn enable_dynamic_ip(&mut self) {
        unsafe { networking::Networking_IpConfig_EnableDynamicIp(&mut self.ipconfig) }
    }

    /// Enables static IP and disables dynamic IP
    pub fn enable_static_ip(
        &mut self,
        ip_address: networking::in_addr,
        subnet_mask: networking::in_addr,
        gateway_address: networking::in_addr,
    ) {
        unsafe {
            networking::Networking_IpConfig_EnableStaticIp(
                &mut self.ipconfig,
                ip_address,
                subnet_mask,
                gateway_address,
            )
        }
    }

    /// Automatically obtain DNS server addresse
    pub fn enable_autopmatic_dns(&mut self) {
        unsafe { networking::Networking_IpConfig_EnableAutomaticDns(&mut self.ipconfig) }
    }

    /// Uses custom DNS server addresses. Up to three addresses may be specified. Any existing DNS server configured via DHCP will be overridden.
    pub fn enable_custom_dns(
        &mut self,
        dns_server_address: Vec<networking::in_addr>,
    ) -> Result<(), std::io::Error> {
        let r = unsafe {
            networking::Networking_IpConfig_EnableCustomDns(
                &mut self.ipconfig,
                dns_server_address.as_ptr(),
                dns_server_address.len() as usize,
            )
        };
        if r == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(())
        }
    }

    /// Applies an IP configuration to a network interface.
    /// This function does not verify whether the static IP address is compatible with the dynamic IP addresses received through an interface using a DHCP client.
    /// This function does not verify whether a DHCP server is available on the network and if a dynamic IP address is configured.
    /// If overlapping IP address configurations are present on a device, the behavior of this function is undefined.
    /// The application manifest must include the NetworkConfig capability.
    pub fn apply<T: Into<CString>>(&self, interface_name: T) -> Result<(), std::io::Error> {
        let r = unsafe {
            let interface_name = interface_name.into();
            let interface_name = std::ffi::CString::new(interface_name.as_bytes()).unwrap();
            networking::Networking_IpConfig_Apply(
                interface_name.as_ptr() as *const libc::c_char,
                &self.ipconfig,
            )
        };
        if r == -1 {
            Err(Error::last_os_error())
        } else {
            Ok(())
        }
    }
}

impl Drop for IpConfig {
    fn drop(&mut self) {
        unsafe { networking::Networking_IpConfig_Destroy(&mut self.ipconfig) }
    }
}

/// Registers and starts an SNTP server for a network interface.
pub fn sntpserver_start(network_interface_name: &str) -> Result<(), std::io::Error> {
    let network_interface_name = std::ffi::CString::new(network_interface_name.as_bytes()).unwrap();
    unsafe {
        let mut config = networking::Networking_SntpServerConfig {
            reserved: [0, 0, 0],
        };
        networking::Networking_SntpServerConfig_Init(&mut config);
        let result =
            networking::Networking_SntpServer_Start(network_interface_name.as_ptr(), &config);
        networking::Networking_SntpServerConfig_Destroy(&mut config);
        if result == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }
}

/// Configure settings for the device's DHCP server.
#[derive(Debug, Clone)]
pub struct DhcpServerConfig {
    config: networking::Networking_DhcpServerConfig,
}

impl DhcpServerConfig {
    pub fn new() -> Self {
        let mut config = networking::Networking_DhcpServerConfig {
            reserved: [0, 0, 0, 0, 0, 0, 0, 0],
        };
        unsafe {
            networking::Networking_DhcpServerConfig_Init(&mut config);
        };
        Self { config }
    }

    /// Applies lease information
    pub fn set_lease(
        &mut self,
        start_ip_address: networking::in_addr,
        ip_address_count: u8,
        subnet_mask: networking::in_addr,
        gateway_address: networking::in_addr,
        lease_time_in_hours: u32,
    ) -> Result<(), std::io::Error> {
        let result = unsafe {
            networking::Networking_DhcpServerConfig_SetLease(
                &mut self.config,
                start_ip_address,
                ip_address_count,
                subnet_mask,
                gateway_address,
                lease_time_in_hours,
            )
        };
        if result == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Applies a set of NTP server IP addresse
    pub fn set_ntp_server_addresses(
        &mut self,
        ntp_server_addresses: Vec<networking::in_addr>,
    ) -> Result<(), std::io::Error> {
        if ntp_server_addresses.len() > 3 {
            return Err(Error::new(std::io::ErrorKind::Other, "Too many addresses"));
        }
        let result = unsafe {
            networking::Networking_DhcpServerConfig_SetNtpServerAddresses(
                &mut self.config,
                ntp_server_addresses.as_ptr(),
                ntp_server_addresses.len(),
            )
        };
        if result == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }
}

/// Registers, configures, and starts the DHCP server for a network interface. The configuration specified by this function call overwrites the existing configuration.
/// If the network interface is up when this function is called, the DHCP server will be shut down, configured, and started. If the interface is down, the server will start when the interface is up.
/// The interface must be configured with a static IP address before this function is called; otherwise, the EPERM error is returned.
/// The application manifest must include the DhcpService capability.
pub fn dhcpserver_start(
    interface_name: &str,
    config: &DhcpServerConfig,
) -> Result<(), std::io::Error> {
    let interface_name = std::ffi::CString::new(interface_name.as_bytes()).unwrap();
    let result =
        unsafe { networking::Networking_DhcpServer_Start(interface_name.as_ptr(), &config.config) };
    if result == 0 {
        Ok(())
    } else {
        Err(Error::last_os_error())
    }
}

impl Drop for DhcpServerConfig {
    fn drop(&mut self) {
        unsafe {
            networking::Networking_DhcpServerConfig_Destroy(&mut self.config);
        };
    }
}

/// Sets the hardware address for a network interface. The hardware address is persisted across reboots, and can only be set on an Ethernet interface. The application manifest must include the HardwareAddressConfig capability.
pub fn set_hardware_address(
    network_interface_name: &str,
    hardware_address: Vec<u8>,
) -> Result<(), std::io::Error> {
    let network_interface_name = std::ffi::CString::new(network_interface_name.as_bytes()).unwrap();
    let result = unsafe {
        static_inline_helpers::Networking_SetHardwareAddress_inline(
            network_interface_name.as_ptr(),
            hardware_address.as_ptr(),
            hardware_address.len(),
        )
    };
    if result == 0 {
        Ok(())
    } else {
        Err(Error::last_os_error())
    }
}

/// Retrieves the hardware address of the given network interface.
pub fn get_hardware_address(network_interface_name: &str) -> Result<[u8; 6], std::io::Error> {
    let network_interface_name = std::ffi::CString::new(network_interface_name.as_bytes()).unwrap();
    let mut addr = static_inline_helpers::Networking_Interface_HardwareAddress {
        address: [0, 0, 0, 0, 0, 0],
    };
    let result = unsafe {
        static_inline_helpers::Networking_GetHardwareAddress_inline(
            network_interface_name.as_ptr(),
            &mut addr,
        )
    };
    if result == 0 {
        Ok(addr.address)
    } else {
        Err(Error::last_os_error())
    }
}

/// Configure socket proxy support.  Call [`NetworkProxyConfig::apply`] to apply the configuration.
#[derive(Debug, Clone)]
pub struct NetworkProxyConfig {
    config: *mut networking::Networking_ProxyConfig,
}

impl NetworkProxyConfig {
    pub fn new() -> Result<Self, std::io::Error> {
        let config = unsafe { networking::Networking_Proxy_Create() };
        if config.is_null() {
            Err(std::io::Error::new(
                std::io::ErrorKind::OutOfMemory,
                "Out of memory",
            ))
        } else {
            Ok(NetworkProxyConfig { config })
        }
    }

    /// Gets the proxy configuration from the device.
    /// The application manifest must include the NetworkConfig or ReadNetworkProxyConfig capability.
    pub fn get(&mut self) -> Result<(), std::io::Error> {
        let result = unsafe { networking::Networking_Proxy_Get(self.config) };
        if result == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Applies a proxy configuration to the device.
    /// The application manifest must include the NetworkConfig capability.
    pub fn apply(&self) -> Result<(), std::io::Error> {
        let result = unsafe { networking::Networking_Proxy_Apply(self.config) };
        if result == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Set proxy options
    pub fn set_options(&mut self, proxy_options: ProxyOptions) -> Result<(), std::io::Error> {
        let result = unsafe {
            networking::Networking_Proxy_SetProxyOptions(self.config, proxy_options.bits())
        };
        if result == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Get proxy options
    pub fn options(&self) -> Result<ProxyOptions, std::io::Error> {
        let mut options: u32 = 0;
        let result =
            unsafe { networking::Networking_Proxy_GetProxyOptions(self.config, &mut options) };
        if result == 0 {
            Ok(ProxyOptions::from(options))
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Set the proxy address
    pub fn set_address(&mut self, address: &str, port: u16) -> Result<(), std::io::Error> {
        let address = std::ffi::CString::new(address.as_bytes()).unwrap();
        let result = unsafe {
            networking::Networking_Proxy_SetProxyAddress(self.config, address.as_ptr(), port)
        };
        if result == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Get the proxy address
    pub fn address(&self) -> Result<String, std::io::Error> {
        unsafe {
            let address = networking::Networking_Proxy_GetProxyAddress(self.config);
            if address.is_null() {
                Err(Error::last_os_error())
            } else {
                let c_str = CStr::from_ptr(address);
                let result = c_str.to_str().map(|s| s.to_owned());
                Ok(result.unwrap())
            }
        }
    }

    /// Get the proxy port
    pub fn port(&self) -> Result<u16, std::io::Error> {
        let mut port: u16 = 0;
        let result = unsafe { networking::Networking_Proxy_GetProxyPort(self.config, &mut port) };
        if result == 0 {
            Ok(port)
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Get the proxy type
    pub fn proxy_type(&self) -> ProxyType {
        let t = unsafe { networking::Networking_Proxy_GetProxyType(self.config) };
        if t == 0 {
            ProxyType::Http
        } else {
            ProxyType::Invalid
        }
    }

    /// Sets the proxy authentication method to anonymous.
    pub fn set_anonymous_authentication(&mut self) -> Result<(), std::io::Error> {
        let result =
            unsafe { networking::Networking_Proxy_SetAnonymousAuthentication(self.config) };
        if result == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Sets the proxy authentication method to basic.
    pub fn set_basic_authentication(
        &mut self,
        username: &str,
        password: &str,
    ) -> Result<(), std::io::Error> {
        let username = std::ffi::CString::new(username.as_bytes()).unwrap();
        let password = std::ffi::CString::new(password.as_bytes()).unwrap();
        let result = unsafe {
            networking::Networking_Proxy_SetBasicAuthentication(
                self.config,
                username.as_ptr(),
                password.as_ptr(),
            )
        };
        if result == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Get the username for proxy authentication
    pub fn username(&self) -> Result<String, std::io::Error> {
        unsafe {
            let user_name = networking::Networking_Proxy_GetProxyUsername(self.config);
            if user_name.is_null() {
                Err(Error::last_os_error())
            } else {
                let c_str = CStr::from_ptr(user_name);
                let result = c_str.to_str().map(|s| s.to_owned());
                Ok(result.unwrap())
            }
        }
    }

    /// Get the password for proxy authentication
    pub fn password(&self) -> Result<String, std::io::Error> {
        unsafe {
            let password = networking::Networking_Proxy_GetProxyUsername(self.config);
            if password.is_null() {
                Err(Error::last_os_error())
            } else {
                let c_str = CStr::from_ptr(password);
                let result = c_str.to_str().map(|s| s.to_owned());
                Ok(result.unwrap())
            }
        }
    }

    /// Gets the proxy authentication type.
    pub fn auth_type(&self) -> AuthType {
        let t = unsafe { networking::Networking_Proxy_GetAuthType(self.config) } as i32;
        if t == networking::Networking_ProxyAuthType_Anonymous {
            AuthType::Anonymous
        } else if t == networking::Networking_ProxyAuthType_Basic {
            AuthType::Basic
        } else {
            AuthType::Invalid
        }
    }

    /// Sets the list of host addresses for which proxy should not be used.
    pub fn set_no_proxy_addresses(
        &mut self,
        no_proxy_addresses: &str,
    ) -> Result<(), std::io::Error> {
        let no_proxy_addresses = std::ffi::CString::new(no_proxy_addresses.as_bytes()).unwrap();
        let result = unsafe {
            networking::Networking_Proxy_SetProxyNoProxyAddresses(
                self.config,
                no_proxy_addresses.as_ptr(),
            )
        };
        if result == 0 {
            Ok(())
        } else {
            Err(Error::last_os_error())
        }
    }

    /// Gets the comma-separated list of hosts for which proxy should not be used.
    pub fn no_proxy_addresses(&self) -> Result<String, std::io::Error> {
        unsafe {
            let addresses = networking::Networking_Proxy_GetNoProxyAddresses(self.config);
            if addresses.is_null() {
                Err(Error::last_os_error())
            } else {
                let c_str = CStr::from_ptr(addresses);
                let result = c_str.to_str().map(|s| s.to_owned());
                Ok(result.unwrap())
            }
        }
    }
}

/// Gets the proxy status.
pub fn proxy_status() -> Result<ProxyStatus, std::io::Error> {
    let mut status: u32 = 0u32;
    let result = unsafe { networking::Networking_Proxy_GetProxyStatus(&mut status) };
    if result == 0 {
        Ok(ProxyStatus::from(status))
    } else {
        Err(Error::last_os_error())
    }
}

impl Drop for NetworkProxyConfig {
    fn drop(&mut self) {
        unsafe {
            networking::Networking_Proxy_Destroy(self.config);
        }
    }
}
