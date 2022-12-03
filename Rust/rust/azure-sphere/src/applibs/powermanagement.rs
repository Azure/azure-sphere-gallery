//! The Applibs powermanagement module contains functions that access the power management capabilities for a device.
use azure_sphere_sys::applibs::powermanagement;
use azure_sphere_sys::applibs::static_inline_helpers;
use std::io::Error;

/// Power profiles
#[repr(u32)]
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum PowerProfile {
    /// The system should prioritize power savings over performance.
    PowerSave = powermanagement::PowerManagement_PowerSaver,
    /// the system should balance power savings and performance according to system load.
    Balanced = powermanagement::PowerManagement_Balanced,
    /// the system should prioritize performance over power savings.
    HighPerformance = powermanagement::PowerManagement_HighPerformance,
}

/// Forces a system reboot. Reboot is equivalent to a hard reset and results in the system stopping and restarting.
/// Your application must declare the ForcePowerDown value in the PowerControls field of the application manifest.
pub fn force_system_reboot() -> Result<(), std::io::Error> {
    let result = unsafe { static_inline_helpers::PowerManagement_ForceSystemReboot_inline() };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Forces the system to the Power Down state for a specified number of seconds.
/// Power Down is the lowest power-consuming state the system is capable of entering while still being able to wake from limited external interrupts or automatically after a time-out.
/// The time spent in the state may be shorter if an external wakeup interrupt occurs.
/// Your application must declare the ForcePowerDown value in the PowerControls field of the application manifest.
pub fn force_system_powerdown(maximum_residency_in_seconds: u32) -> Result<(), std::io::Error> {
    let result = unsafe {
        static_inline_helpers::PowerManagement_ForceSystemPowerDown_inline(
            maximum_residency_in_seconds,
        )
    };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}

/// Sets the system power profile. The system dynamically adjusts the CPU frequency to balance power consumption and performance according to the specified Power Profile.
/// *Note*
/// Power profiles do not persist across reboots and should always be set when your application starts.
/// Your application must declare the SetPowerProfile value in the PowerControls field of the application manifest.
pub fn set_system_power_profile(desired_profile: PowerProfile) -> Result<(), std::io::Error> {
    let result = unsafe {
        static_inline_helpers::PowerManagement_SetSystemPowerProfile_inline(desired_profile as u32)
    };
    if result == -1 {
        Err(Error::last_os_error())
    } else {
        Ok(())
    }
}
