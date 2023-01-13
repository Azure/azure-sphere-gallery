//! The Applibs rtc module contains functions that interact with the real-time clock (RTC).
use azure_sphere_sys::applibs::rtc;
use std::io::Error;

/// Synchronizes the real-time clock (RTC) with the current system time. The RTC only stores the time in UTC/GMT. Therefore, conversion from local time is necessary only if the local time zone isn't GMT.
/// This function requires the SystemTime capability in the application manifest.
pub fn clock_systohc() -> Result<(), std::io::Error> {
    let result = unsafe { rtc::clock_systohc() };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}
