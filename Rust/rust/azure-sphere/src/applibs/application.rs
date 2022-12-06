//! The Applibs module contains functions that communicate with and control real-time capable applications.
use azure_sphere_sys::applibs::application;
use std::ffi::CString;
use std::io::Error;
use std::net::TcpStream;
use std::os::unix::io::FromRawFd;

/// Creates a socket that can communicate with a real-time capable application.
/// The socket is created in a connected state, and may be used to transfer
/// messages to and from the real-time capable application. The message format is
/// similar to a datagram.
pub fn connect<P: Into<CString>>(component_id: P) -> Result<TcpStream, std::io::Error> {
    let component_id = component_id.into();
    let fd = unsafe { application::Application_Connect(component_id.as_ptr()) };
    if fd == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(unsafe { TcpStream::from_raw_fd(fd) })
    }
}

/// Verifies that the device authentication and attestation (DAA) certificate for the current device is ready.
pub fn is_device_auth_ready() -> Result<bool, std::io::Error> {
    let mut ready = false;
    let result = unsafe { application::Application_IsDeviceAuthReady(&mut ready) };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(ready)
    }
}
