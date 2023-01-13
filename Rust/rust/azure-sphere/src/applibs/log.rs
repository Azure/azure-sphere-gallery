//! The Applibs log module contains functions that log debug messages. Debug messages are displayed when you debug an application through the Azure Sphere SDK. These functions are thread safe.
use azure_sphere_sys::applibs::log;
use std::convert::Into;
use std::io::Error;

/// Formats and logs a debug message
#[macro_export]
macro_rules! debug_checked {
    ($arg:expr) => {
        $crate::applibs::log::log_debug_checked(concat!($arg, "\0"))
    };
    ($($args:tt)*) => {{
        $crate::applibs::log::log_debug_checked($crate::alloc::format!($($args)*))
    }};
}
pub use crate::debug_checked;

/// Formats and logs a debug message
///
/// #Errors
/// Returns an error with the errno if the call to `Log_Debug` fails
pub fn log_debug_checked<T: Into<Vec<u8>>>(message: T) -> Result<(), std::io::Error> {
    let message = message.into();
    let result = unsafe { log::Log_Debug(message.as_ptr() as *const libc::c_char) };
    if result == 0 {
        Ok(())
    } else {
        Err(Error::last_os_error())
    }
}

/// Formats and logs a debug message
#[macro_export]
macro_rules! debug {
    ($arg:expr) => {
        $crate::applibs::log::log_debug(concat!($arg, "\0"))
    };
    ($($args:tt)*) => {{
        $crate::applibs::log::log_debug($crate::alloc::format!($($args)*))
    }};
}
pub use crate::debug;

/// Formats and logs a debug message
///
/// Returns an error with the errno if the call to `Log_Debug` fails
pub fn log_debug<T: Into<Vec<u8>>>(message: T) {
    let message = message.into();
    let _ = unsafe { log::Log_Debug(message.as_ptr() as *const libc::c_char) };
}
