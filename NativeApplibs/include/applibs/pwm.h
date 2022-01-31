/// \file pwm.h
/// \brief This header contains functionality for interacting with PWM hardware. Access to
/// individual PWMs is restricted based on the "Pwm" field of the application's manifest.
///
/// PWM functions are thread-safe.

#include <stdbool.h>
#include <stdint.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     Identifies a PWM controller to open for access.
/// </summary>
typedef unsigned int PWM_ControllerId;

/// <summary>
///     The ID of a PWM channel. Many controllers might have multiple channels on a single chip.
///     An individual channel correspond with a single pin or input on the device.
/// </summary>
typedef uint32_t PWM_ChannelId;

/// <summary>
///     The polarity to apply to a PWM channel.
/// </summary>
typedef uint32_t PwmPolarity;

enum {
    /// <summary>
    ///     Normal polarity implies a high signal during the duty cycle, and a low signal for
    ///     the remainder of the period. For example, a duty cycle of 100ns with a period of 300ns
    ///     results in a high signal for 100ns and a low signal for 200ns.
    /// </summary>
    PWM_Polarity_Normal,

    /// <summary>
    ///     Inversed polarity implies a signal that's low during the duty cycle, and is high
    ///     for the remainder of the period. For example, a duty cycle of 100ns with a period of
    ///     300ns results in a high signal for 200ns and a low signal for 100ns.
    /// </summary>
    PWM_Polarity_Inversed,
};

/// <summary>
///     The full state to apply to a specific PWM channel of the already-opened controller. The
///     driver guarantees it will apply state atomically.
/// </summary>
typedef struct PwmState {
    /// <summary>
    ///     The period_nsec parameter governs the total length of time, in nanoseconds, of each
    ///     cycle. This value constitutes the total time spent in both the high and low state.
    /// </summary>
    unsigned int period_nsec;

    /// <summary>
    ///     The dutyCycle_nsec parameter governs the total length of time, in nanoseconds, spent
    ///     either high or low during a cycle. The polarity governs whether this time is spent high
    ///     or low. dutyCycle_nsec must be less than the period.
    /// </summary>
    unsigned int dutyCycle_nsec;

    /// <summary>
    ///     Defines the polarity to apply. <see cref="PwmPolarity"/>
    /// </summary>
    PwmPolarity polarity;

    /// <summary>
    ///     True to enable the PWM functionality, false to disable it.
    /// </summary>
    bool enabled;
} PwmState;

/// <summary>
///     Opens a PWM controller for access.
/// </summary>
/// <param name="pwm">
///     The index of the PWM controller to access. The index is 0-based. The maximum value permitted
///     depends on the platform.
/// </param>
/// <returns>
///     A file descriptor, or -1 for failure, in which case errno will be set to the error value.
///     <para> **Errors** </para>
///     <para>EACCES: access to this PWM interface is not permitted; verify that the interface
///     exists and is in the "Pwm" field of the application manifest.</para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same behavior
///     will be retained through system updates.</para>
/// </returns>
static int PWM_Open(PWM_ControllerId pwm);

/// <summary>
///     Applies the provided state to a specific PWM channel of the already-opened controller.
/// </summary>
/// <param name="pwmFd">
///     The file descriptor returned from a prior call to PWM_Open.
/// </param>
/// <param name="pwmChannel">
///     The PWM channel of the controller to modify. The index is 0-based. The maximum value
///     permitted depends on the platform.
/// </param>
/// <param name="newState">
///     A pointer to a struct storing the new parameters to apply. <see cref="PwmState"/>. The
///     pointer must remain valid only for the duration of the call.
/// </param>
/// <returns>
///     Zero on success, -1 on error. The call sets errno when an error occurs.
///     <para> **Errors** </para>
///     <para>EBADF: the file descriptor is invalid.</para>
///     <para>ENODEV: the pwmChannel parameter is invalid. Verify the channel is valid for this
///     hardware platform.</para>
///     <para>EINVAL: the newState parameter passed to this call is
///     invalid. Verify that newState is not null, and contains valid entries.</para>
///     <para>Any other errno may also be specified, but there's no guarantee that
///     the same behavior will be retained through system updates.</para>
/// </returns>
static int PWM_Apply(int pwmFd, PWM_ChannelId pwmChannel, const PwmState *newState);

#ifdef __cplusplus
}
#endif

#ifdef NATIVE
#include <applibs/pwm_internal_native.h>
#else
#include <applibs/pwm_internal.h>
#endif