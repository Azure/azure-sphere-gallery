//! The Applibs module contains the functions and types needed to acquire information about all applications.
//!
//! *Note*
//! These functions return the memory usage as seen by the OS. Currently, the freeing of memory by an application for allocations on the user heap is not reported by these functions. The memory will be returned to the malloc library for future use but the statistics reported by the OS remain unchanged unless the memory was allocated and freed by the OS itself. An example would be allocating memory for a socket. Therefore, these functions are useful for understanding worst-case scenarios to help your application operate conservatively for maximum reliability. Values are approximate and may vary across OS versions.
use azure_sphere_sys::applibs::static_inline_helpers;
use std::io::Error;

/// Gets the total memory usage of your high-level application in kibibytes. This is the total physical memory usage of your app on the system, including kernel allocations (such as buffers for sockets) on behalf of your app or the debugging server, returned as a raw value (in KiB). Values returned are approximate and may vary across operating system versions.
pub fn total_memory_usage() -> usize {
    return unsafe { static_inline_helpers::Applications_GetTotalMemoryUsageInKB_inline() };
}

/// Gets the user-mode memory usage of your high-level application in kibibytes. This is the amount of physical memory used directly by your app, the memory used by any libraries on its behalf (also referred to as anon allocations), and memory used by the debugging server, returned as a raw value (in KiB). Values returned are approximate and may vary across operating system versions.
pub fn user_mode_memory_usage() -> usize {
    return unsafe { static_inline_helpers::Applications_GetUserModeMemoryUsageInKB_inline() };
}

/// Gets the peak user-mode memory usage of your high-level application in kibibytes. This is the maximum amount of user memory used in the current session. It is returned as a raw value (in KiB). Values returned are approximate and may vary across operating system versions.
pub fn peak_user_mode_memory_usage() -> usize {
    return unsafe { static_inline_helpers::Applications_GetPeakUserModeMemoryUsageInKB_inline() };
}

/// Gets the human-readable string of the currently running OS version. This string is based on the year and month of the release and may include additional characters related to updates, hotfixes, or security patches that deviate from the initial release of the same month and year.
pub fn os_version() -> Result<String, std::io::Error> {
    let mut version = static_inline_helpers::Applications_OsVersion { version: [0; 16] };
    unsafe {
        let ret = static_inline_helpers::Applications_GetOsVersion_inline(&mut version);
        if ret == -1 {
            Err(Error::last_os_error())
        } else {
            return Ok(String::from_utf8_lossy(&version.version).into());
        }
    }
}
