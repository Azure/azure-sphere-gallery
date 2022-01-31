/// \file powermanagement_internal.h
/// \brief This header contains internal functions for the PowerManagement library.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/types.h>
#include <string.h>
#include <unistd.h>

#include <applibs/applibs_internal_api_traits.h>

#ifdef AZURE_SPHERE_PUBLIC_SDK
#include <linux/cpufreq_dev.h>
#else
#include <linux-as/linux/azure-sphere/cpufreq_dev.h>
#endif

#ifndef AZURE_SPHERE_PUBLIC_SDK
// TODO(zhen):(line:164) The CPU number used is hard-coded for now since MT3620 has single cpu core
// Multi-core support should be designed for future SoC which has multiple cores, cpus, clusters
// and etc. A proper way of finding right cpu number needs to be investigated, too.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     Defines the set of possible power management states the system can be placed into.
///
///     Note: A z__PowerManagement_System_PowerState enum beginning with the prefix "Force" indicates
///     that the system will override any deferral, immediately sending a SIGTERM to running apps,
///     then proceeding to the power state.
/// </summary>
typedef uint32_t z__PowerManagement_System_PowerState;
enum {
    /// <summary>
    /// The forced version of the Reboot system power state. This state results in the system
    /// stopping and resuming execution the same as if it entered
    /// z__PowerManagement_System_PowerState_ForcePowerDown and immediately woke back up.
    /// Requires an app to have the "ForceReboot" value in the PowerControls manifest capability.
    /// </summary>
    z__PowerManagement_System_PowerState_ForceReboot,

    /// <summary>
    /// The forced version of the Power Down system power state. Power Down is the lowest
    /// power-consuming state the system is capable of entering while still being able
    /// to wake from limited external interrupts or from an RTC alarm.
    /// Requires an app to have the "ForcePowerDown" value in the PowerControls manifest capability.
    /// </summary>
    z__PowerManagement_System_PowerState_ForcePowerDown
};

/// <summary>
///     The struct containing the parameters for the transition to the PowerDown state.
///
///     maximum_residency_in_seconds - the maximum time, in seconds, the system may be resident in
///     this state before transitioning back to active. The actual residency may be shorter than
///     the maximum if a wake event occurs sooner.
/// </summary>
struct z__PowerManagement_PowerDownStateTransitionParams {

    // <summary>
    //      The maximum number of seconds the system should remain in the power-down state.
    // </summary>
    unsigned int maximum_residency_in_seconds;
};

/// <summary>
///     Initiates a transition to the specified power state.
///     Depending on the system state requested, entry may not have occurred when this routine
///     returns; instead notification of entry into the state occurs via the Event Loop framework
///     in the same manner as device updates.
///
///     **Errors**
///     If an error is encountered, returns -1 and sets errno to the error value.
///         EACCES: access to <paramref name="state"/> is not permitted as the required entry is
///         not listed in the application manifest.
///         EINVAL: the <paramref name="state"/> is invalid or the
///              <paramref name="power_state_transition_params_ptr"/> contain invalid data.
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="type">
///     The <see cref="z__PowerManagement_System_PowerState"/> to transition.
/// </param>
/// <param name="power_state_transition_params_ptr">
///     A pointer to the struct containing the necessary parameters for the given
///     z__PowerManagement_System_PowerState type(i.e.
///     z__PowerManagement_PowerDownStateTransitionParams).
/// </param>
/// <returns>
///     Zero on success, or -1 for failure, in which case errno will
///     be set to the error value.
/// </returns>
int z__PowerManagement_TransitionToSystemPowerState(z__PowerManagement_System_PowerState type,
                                                    void *power_state_transition_params_ptr);

static inline int PowerManagement_ForceSystemReboot()
{
    return z__PowerManagement_TransitionToSystemPowerState(
        z__PowerManagement_System_PowerState_ForceReboot, NULL);
}

static inline int PowerManagement_ForceSystemPowerDown(unsigned int maximum_residency_in_seconds)
{
    struct z__PowerManagement_PowerDownStateTransitionParams params;
    params.maximum_residency_in_seconds = maximum_residency_in_seconds;

    return z__PowerManagement_TransitionToSystemPowerState(
        z__PowerManagement_System_PowerState_ForcePowerDown, (void *)&params);
}

/// <summary>
///     Open the cpufreq file descriptor for ioctl calls
///
///     **Errors**
///     If an error is encountered, returns -1 and sets errno to the error value.
///         EACCES: access to dev/cpufreq is not permitted as the required entry is
///                 not listed in the application manifest.
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <returns>
///     0 for success if the cpufreq file descriptor was successfully opened, or -1 for failure, in
///     which case errno will be set to the error value.
/// </returns>
static inline int PowerManagement_CpufreqOpen(void)
{
    int fd = open("/dev/cpufreq", O_WRONLY | O_CLOEXEC, 0);
    if (fd < 0) {
        if (errno == ENOENT) {
            // N.B. If there's no fd, then we don't have permission
            // to access it. That could be because we don't have
            // the capability for it. So we default to reporting back EACCES.
            errno = EACCES;
        }
        return -1;
    }

    return fd;
}

static inline int PowerManagement_SetSystemPowerProfile(
    PowerManagement_System_PowerProfile desired_profile)
{
    int fd = PowerManagement_CpufreqOpen();
    if (fd < 0) {
        return -1;
    }

    struct azure_sphere_cpufreq_dev_scaling_governor_for_cpu sgn;
    sgn.cpu = 0;
    const char *profile = NULL;
    //  SetSystemPowerProfile API receives PowerManagement_SystemPowerProfile a.k.a. easy mode and
    //  convert the power profile to corresponding cpufreq governor
    //  Then an ioctl call is made to apply the swap
    switch (desired_profile) {
    case PowerManagement_PowerSaver:
        profile = "conservative";
        break;
    case PowerManagement_Balanced:
        profile = "ondemand";
        break;

    case PowerManagement_HighPerformance:
        profile = "performance";
        break;
    default:
        close(fd);
        errno = EINVAL;
        return -1;
    };
    strncpy(sgn.governor_name, profile, sizeof(sgn.governor_name));
    int res = ioctl(fd, CPUFREQ_SET_SCALING_GOVERNOR_FOR_CPU, &sgn);
    close(fd);
    return res;
}

#ifdef __cplusplus
}
#endif
