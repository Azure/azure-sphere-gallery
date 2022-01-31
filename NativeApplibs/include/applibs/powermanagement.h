/// file powermanagement.h
/// \brief This header contains functionality for interacting with power management.
/// Access to and control of different kinds of power management is restricted
/// based on the fields of the application's manifest.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/// <summary>
///     The possible system power profiles.
///     The power profile PowerManagement_HighPerformance is applied by default at MT3620
/// </summary>
typedef uint32_t PowerManagement_System_PowerProfile;
enum {
    /// Indicates to the system that the high-level applications should prioritize
    /// energy savings over performance, even if that means it takes longer to
    /// accomplish some workloads due to the fact that it will save energy by running
    /// slower.
    PowerManagement_PowerSaver = 0,
    /// Indicates to the system that the high-level applications should run with
    /// performance that balances energy savings with time taken to complete customer
    /// workloads, with the aim to avoid delaying workloads noticeably or unduly, while
    /// still providing the majority of the energy savings possible for a given SoC.
    PowerManagement_Balanced = 1,
    /// Indicates to the system that the high-level applications need high-performance
    /// from the SoC, and that the system should to a reasonable extent prioritize
    /// performance over power consumption. This does not preclude the system enacting
    /// some power savings when possible, so long as it does not meaningfully impact
    /// customer application performance.
    PowerManagement_HighPerformance = 2
};

/// <summary>
///     Asynchronously initiates the process of a forced system reboot.
///
///     Reboot results in the system stopping and resuming execution the same as if it entered
///     system power-down and immediately woke back up.
///
///     Note: Successful return of this routine only indicates the reboot was initiated.
///         In response to invocation of this routine, this application, as well as all others on
///         the system, will receive a SIGTERM, followed by the system rebooting.
///         Due to the asynchronous nature of this request, it is possible that the application
///         receives the SIGTERM before this routine returns.
///
///     **Errors**
///     If an error is encountered, returns -1 and sets errno to the error value.
///         EACCES: access to system reboot is not permitted as the required entry is not listed
///                 in the application manifest.
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <returns>
///     0 for success if the forced system reboot was successfully initiated, or -1 for failure, in
///     which case errno will be set to the error value.
/// </returns>
static int PowerManagement_ForceSystemReboot();

/// <summary>
///     Asynchronously initiates the process of forcing the system to transition to the power-down
///     state for up to maximum_residency_in_seconds amount of time.
///
///     System power down is the lowest power-consuming state the system is capable of entering
///     while still being able to wake from limited external interrupts or automatically after a
///     time-out.
///
///     The time spent in the state may be shorter if an external wakeup interrupt occurs,
///     or if internal system services require system execution sooner than the specified maximum
///     residency.
///
///     Note: Successful return of this routine only indicates the power-down was initiated.
///         In response to invocation of this routine, this application, as well as all others on
///         the system, will receive a SIGTERM, followed by the system powering down.
///         Due to the asynchronous nature of this request, it is possible that the application
///         receives the SIGTERM before this routine returns.
///
///     **Errors**
///     If an error is encountered, returns -1 and sets errno to the error value.
///         EACCES: access to force system power-down is not permitted as the required entry is not
///                 listed in the application manifest.
///         EINVAL: an invalid value was specified for maximum_residency_in_seconds.
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="maximum_residency_in_seconds">
///    the maximum time, in seconds, the system may be resident in this state before transitioning
///    back to active. The actual residency may be shorter than the maximum for the reasons
///    specified in this routine's description.
/// </param>
/// <returns>
///     0 for success if the forced power-down was successfully initiated, or -1 for failure, in
///     which case errno will be set to the error value.
/// </returns>
static int PowerManagement_ForceSystemPowerDown(unsigned int maximum_residency_in_seconds);

/// <summary>
///     Swaps power profile which applies system wide.
///
///     With the power profile swapping, the performance and energy usage of the system will
///     increase/decrease by changing cpu frequency and/or voltage
///
///     **Errors**
///     If an error is encountered, returns -1 and sets errno to the error value.
///         EACCES: access to set sysytem power profile is not permitted as the required entry is
///                 not listed in the application manifest.
///         EINVAL: an invalid sysytem power profile was specified for desired_profile.
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="desired_profile">
///     The <see cref="PowerManagement_SystemPowerProfile"/> to set.
/// </param>
/// <returns>
///     0 for success if the sysytem power profile was successfully set, or -1 for failure, in
///     which case errno will be set to the error value.
/// </returns>
static int PowerManagement_SetSystemPowerProfile(
    PowerManagement_System_PowerProfile desired_profile);

#ifdef __cplusplus
}
#endif

#include <applibs/powermanagement_internal.h>