/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef AZURE_SPHERE_I2S_H_
#define AZURE_SPHERE_I2S_H_

#include "Common.h"
#include "Platform.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/// <summary>Enum for audio input/output formats.</summary>
typedef enum {
    /// <summary>This will disable input/output when passed.</summary>
    I2S_FORMAT_NONE = 0,
    /// <summary>I2S is a standard format which supports one or two channels</summary>
    I2S_FORMAT_I2S,
    /// <summary>TDM is a standard format which supports any number of channels</summary>
    I2S_FORMAT_TDM,
} I2S_Format;

/// <summary>Opaque I2S handle.</summary>
typedef struct I2S I2S;

/// <summary>
/// <para>Acquires a handle before using a given I2S interface.</para>
/// </summary>
/// <param name="unit">Which I2S interface to initialize and acquire a handle for.</param>
/// <param name="mclk">The desired frequency of the master clock.</param>
/// <returns>A handle to an I2S interface or NULL on failure.</returns>
I2S *I2S_Open(Platform_Unit unit, unsigned mclk);

/// <summary>
/// <para>Releases a handle once it's finished using a given I2S interface.
/// Once released the handle is free to be opened again.</para>
/// </summary>
/// <param name="handle">The I2S handle which is to be released.</param>
void I2S_Close(I2S *handle);

/// <summary>
/// <para>Enables audio output on an I2S interface.</para>
/// </summary>
/// <param name="handle">The I2S handle on which output is to be enabled</param>
/// <param name="format">The desired format (I2S or TDM), set to I2S_FORMAT_NONE to disable.</param>
/// <param name="channels">The number of channels to output on (must be 1 or 2 for I2S).</param>
/// <param name="bits">The number of bits per sample (usually 16).</param>
/// <param name="rate">The target sample rate of the output.</param>
/// <param name="callback">The callback which will be used to fill the output buffer:
///                        (data: pointer to data buffer, size: size of buffer in bytes).</param>
/// <returns>ERROR_NONE on success or an error code on failure.</returns>
int32_t I2S_Output(
    I2S *handle, I2S_Format format, unsigned channels, unsigned bits, unsigned rate,
    bool (*callback)(void *data, uintptr_t size));

/// <summary>
/// <para>Enables audio output on an I2S interface.</para>
/// </summary>
/// <param name="handle">The I2S handle on which input is to be enabled</param>
/// <param name="format">The desired format (I2S or TDM), set to I2S_FORMAT_NONE to disable.</param>
/// <param name="channels">The number of channels to input (must be 1 or 2 for I2S).</param>
/// <param name="bits">The number of bits per sample (usually 16).</param>
/// <param name="rate">The target sample rate of the input.</param>
/// <param name="callback">The callback which will be used to process the input buffer:
///                        (data: pointer to data buffer, size: size of buffer in bytes)</param>
/// <returns>ERROR_NONE on success or an error code on failure.</returns>
int32_t I2S_Input(
    I2S *handle, I2S_Format format, unsigned channels, unsigned bits, unsigned rate,
    bool (*callback)(void *data, uintptr_t size));

/// <summary>
/// <para>Query the output sample rate of an I2S interface.</para>
/// </summary>
/// <param name="handle">The I2S handle on which the output sample rate is queried.</param>
/// <returns>The exact sample rate in herts of the output, or zero if disabled.</returns>
unsigned I2S_GetOutputSampleRate(I2S *handle);

/// <summary>
/// <para>Query the input sample rate of an I2S interface.</para>
/// </summary>
/// <param name="handle">The I2S handle on which the input sample rate is queried.</param>
/// <returns>The exact sample rate in herts of the input, or zero if disabled.</returns>
unsigned I2S_GetInputSampleRate(I2S *handle);

#ifdef __cplusplus
 }
#endif

#endif // #ifndef AZURE_SPHERE_I2S_H_
