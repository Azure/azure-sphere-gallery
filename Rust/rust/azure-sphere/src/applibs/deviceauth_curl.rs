//! The Applibs deviceauth module contains functions and types for applications to perform mutual authentication over TLS.
//! The simplest way to perform device authentication is to configure [`curl_ssl_function`]  as the callback function for curl SSL authentication:
//! See [Connect to web services](https://learn.microsoft.com/en-us/azure-sphere/app-development/curl?tabs=cliv2beta)
use azure_sphere_sys::applibs::deviceauth_curl;
use curl::Error;
use std::ffi::c_void;

/// The possible results from the [`ssl_ctx_func`] function.
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum DeviceAuthSslResult {
    /// Success
    Success,
    /// Failed to access the current application's tenant ID.
    GetTenantIdError,
    /// Failed to load the device authentication certificate for the tenant.
    GetTenantCertificateError,
    /// Failed to enable hardware signing.
    EnableHwSignError,
    /// Unknown error
    UnknownError,
}

/// The simplest way to perform device authentication is to configure [`curl_ssl_function`] as the callback function for curl SSL authentication:
/// #Example
/// ```
///     let result = curl_easy.ssl_ctx_function(deviceauth::curl_ssl_function);
/// ```
/// To access individual hosts or domains, your application must identify them in the AllowedConnections field of the application manifest. If the application uses mutual authentication, the DeviceAuthentication field of the manifest must include the Azure Sphere tenant ID.
pub fn curl_ssl_function(ssl_ctx: *mut c_void) -> Result<(), Error> {
    let curl_error = unsafe { deviceauth_curl::DeviceAuth_SslCtxFunc(ssl_ctx) };
    if curl_error == 0 {
        Ok(())
    } else {
        Err(curl::Error::new(curl_error))
    }
}

/// Performs device authentication for TLS connections. Note that although the curl function names include SSL, Azure Sphere uses TLS for authentication.
///
/// An application's custom libcurl function calls [`ssl_ctx_func`] to perform device authentication of TLS connections. Your custom function must call [`ssl_ctx_func`] to perform the authentication, but may also perform other tasks related to authentication.
/// To access individual hosts or domains, your application must identify them in the AllowedConnections field of the application manifest. If the application uses mutual authentication, the DeviceAuthentication field of the manifest must include the Azure Sphere tenant ID.
pub fn ssl_ctx_func(sslctx: *mut c_void) -> DeviceAuthSslResult {
    let result = unsafe { deviceauth_curl::DeviceAuth_SslCtxFunc(sslctx) };
    match result {
        deviceauth_curl::DeviceAuthSslResult_DeviceAuthSslResult_Success => {
            DeviceAuthSslResult::Success
        }
        deviceauth_curl::DeviceAuthSslResult_DeviceAuthSslResult_GetTenantIdError => {
            DeviceAuthSslResult::GetTenantIdError
        }
        deviceauth_curl::DeviceAuthSslResult_DeviceAuthSslResult_GetTenantCertificateError => {
            DeviceAuthSslResult::GetTenantCertificateError
        }
        deviceauth_curl::DeviceAuthSslResult_DeviceAuthSslResult_EnableHwSignError => {
            DeviceAuthSslResult::EnableHwSignError
        }
        _ => DeviceAuthSslResult::UnknownError,
    }
}
