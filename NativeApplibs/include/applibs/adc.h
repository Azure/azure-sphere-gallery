/// \file adc.h
/// \brief This header contains functionality for interacting with Industrial ADC devices/channels.
/// Access to individual ADC channels is restricted based on the "Adc" field of the application's
/// manifest.
///
/// ADC functions are thread-safe when accessing different ADC channels concurrently. However, the
/// caller must ensure thread safety when accessing the same ADC channel.
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     The ID of an ADC controller, indexed from 0.
/// </summary>
typedef uint32_t ADC_ControllerId;

/// <summary>
///     The ID of an ADC channel on an ADC device. Many ADCs might have multiple channels on a
///     single chip. An individual channel correspond with a single pin or input on the device.
/// </summary>
typedef uint32_t ADC_ChannelId;

/// <summary>
///     Opens an ADC controller for access.
/// </summary>
/// <param name="id">
///     The index of the ADC controller to access. The index is 0-based. The maximum value permitted
///     depends on the platform.
/// </param>
/// <returns>
///     A file descriptor, or -1 for failure, in which case errno will be set to the error value.
///     Applications should close the file descriptor when finished.
///     <para> **Errors** </para>
///     <para>EACCES: access to this ADC controller is not permitted; verify that the controller
///     exists and is listed in the "Adc" field of the application manifest.</para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same behavior
///     will be retained through system updates.</para>
/// </returns>
static int ADC_Open(ADC_ControllerId id);

/// <summary>
///     Returns the number of bits of valid data available via a call to ADC_Poll.
/// </summary>
/// <param name="fd">The file descriptor returned from a prior call to ADC_Open</param>
/// <param name="channel">
///     The channel index to query. The range of allowed values is device-dependent but is
///     typically zero-based.
/// </param>
/// <returns>
///     Returns the number of bits of valid data available via a call to ADC_Poll.
///     An example return value is 12, indicating that the ADC controller can supply 12 bits of
///     data, ranging from 0 to 4095. Returns -1 in case of an error, in which case errno will
///     be set to the error value.
/// </returns>
static int ADC_GetSampleBitCount(int fd, ADC_ChannelId channel);

/// <summary>
///     Sets the reference voltage being supplied to the ADC.  The reference voltage represents the
///     highest voltage that the ADC can accurately return as a digital sample.  For example,
///     an ADC with 12 bits of resolution, a 1.8V reference voltage, and a constant 0.9V input will
///     return values near (0.9V / 1.8V) * 4095, or 2047.
///     <para>The underlying hardware is free to map this value to one of several constants defined
///     in the corresponding datasheet. </para>
/// </summary>
/// <param name="fd">The file descriptor returned from a prior call to ADC_Open</param>
/// <param name="channel">
///     The channel index to set. The range of allowed values is device-dependent but is
///     typically zero-based.
/// </param>
/// <param name="referenceVoltage">
///     The desired reference voltage.
/// </param>
/// <returns>
///     Returns 0 on success, or -1 on error, in which case errno will be set to the error value.
///     <para> **Errors** </para>
///     <para>EINVAL: The reference voltage is out of the supported range of the hardware.</para>
/// </returns>
static int ADC_SetReferenceVoltage(int fd, ADC_ChannelId channel, float referenceVoltage);

/// <summary>
///     Returns the specified sample data from the channel.
/// </summary>
/// <param name="fd">The file descriptor returned from a prior call to ADC_Open</param>
/// <param name="channel">
///     The channel index to query. The range of allowed values is device-dependent but is
///     typically zero-based.
/// </param>
/// <param name="outSampleValue">
///     Pointer to an uint32_t to receive the data queried. Must not be NULL.
/// </param>
/// <returns>0 on success, or -1 for failure, in which case errno will be set to the error value.
///     <para> **Errors** </para>
///     <para>EACCES: access to this ADC controller is not permitted; verify that the controller
///     exists and is listed in the "Adc" field of the application manifest.</para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same behavior
///     will be retained through system updates.</para>
/// </returns>
static int ADC_Poll(int fd, ADC_ChannelId channel, uint32_t *outSampleValue);

#ifdef __cplusplus
}
#endif

#ifdef NATIVE
#include <applibs/adc_internal_native.h>
#else
#include <applibs/adc_internal.h>
#endif