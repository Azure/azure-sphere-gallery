//! The Applibs networking_curl module contains functions and types that interact with the networking subsystem to apply the stored proxy configuration on a curl handle.
use azure_sphere_sys::applibs::static_inline_helpers;
use curl::easy::Easy;
use std::io::Error;

/// Applies the proxy settings on the cURL handle.
/// The application manifest must include the NetworkConfig or ReadNetworkProxyConfig capability.
pub fn set_default_proxy(c: &Easy) -> Result<(), std::io::Error> {
    let ret = unsafe {
        let raw = c.raw() as *mut _;
        static_inline_helpers::Networking_Curl_SetDefaultProxy_inline(raw as *mut libc::c_void)
    };
    if ret == 0 {
        Ok(())
    } else {
        Err(Error::last_os_error())
    }
}
