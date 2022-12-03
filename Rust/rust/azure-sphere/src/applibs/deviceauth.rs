//! The Applibs deviceauth module contains functions and types for applications to get certificate paths over TLS.
use azure_sphere_sys::applibs::deviceauth;
use std::io::Error;

/// Returns a file path to a client certificate managed by the Azure Sphere OS. Libraries can use this path to load a certificate for TLS communications.
/// This function always returns a file path, but because the certificate is managed by the OS, the certificate may not always be ready for use. No additional
/// status on the certificate is provided by this function. Use [`crate::applibs::application::is_device_auth_ready`] to check whether the certificate is ready for use.
/// The certificate, which is valid for 24 hours, is in x509 format and can be parsed with wolfSSL library functions.
///
/// The returned path is valid only for the lifetime of the current application process. The path may change when the application restarts.
pub fn certificate_path() -> Result<&'static str, std::io::Error> {
    let cert_path = unsafe {
        let c_buf = deviceauth::DeviceAuth_GetCertificatePath();
        if c_buf.is_null() {
            // The C function is never expected to return NULL, so no need to create an Option<T> return type here.
            return Err(Error::last_os_error());
        }
        std::ffi::CStr::from_ptr(c_buf)
    };
    let cert_path = cert_path.to_str();
    if cert_path.is_err() {
        Err(std::io::Error::from_raw_os_error(libc::EINVAL))
    } else {
        Ok(cert_path.unwrap())
    }
}
