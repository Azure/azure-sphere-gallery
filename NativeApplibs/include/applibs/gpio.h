/// \file gpio.h
/// \brief This header contains functionality for interacting with GPIOs. Access to individual GPIOs
/// is restricted based on the Gpio field of the application's manifest.
///
/// GPIO functions are thread-safe when accessing different GPIOs concurrently. However, the caller
/// must ensure thread safety when accessing the same GPIO.
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     Specifies the type of a GPIO ID, which is used to specify a GPIO peripheral instance.
/// </summary>
typedef int GPIO_Id;

/// <summary>
///     Specifies the type of a GPIO value.
/// </summary>
typedef uint8_t GPIO_Value_Type;

/// <summary>
///     The possible read/write values for a GPIO.
/// </summary>
typedef enum {
    /// <summary>Low, or logic 0</summary>
    GPIO_Value_Low = 0,
    /// <summary>High, or logic 1</summary>
    GPIO_Value_High = 1
} GPIO_Value;

/// <summary>
///     Specifies the type of the GPIO output mode.
/// </summary>
typedef uint8_t GPIO_OutputMode_Type;

/// <summary>
///     The options for the output mode of a GPIO.
/// </summary>
typedef enum {
    /// <summary>Sets the output mode to push-pull.</summary>
    GPIO_OutputMode_PushPull = 0,
    /// <summary>Sets the output mode to open drain.</summary>
    GPIO_OutputMode_OpenDrain = 1,
    /// <summary>Sets the output mode to open source.</summary>
    GPIO_OutputMode_OpenSource = 2
} GPIO_OutputMode;

/// <summary>
///     Opens a GPIO (General Purpose Input/Output) as an input.
///     <para>Call <see cref="GPIO_GetValue"/> on an open input GPIO to read the input value.</para>
///     <para>Calling <see cref="GPIO_SetValue"/> on an open input GPIO will have no effect.</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: access to <paramref name="gpioId"/> is not permitted as the GPIO is not listed
///     in the Gpio field of the application manifest.</para>
///     <para>ENODEV: the provided <paramref name="gpioId"/> is invalid.</para>
///     <para>EBUSY: the <paramref name="gpioId"/> is already open.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="gpioId">
///     A <see cref="GPIO_Id"/> that identifies the GPIO.
/// </param>
/// <returns>
///     A file descriptor for the opened GPIO on success, or -1 for failure, in which case errno
///     is set to the error value.
/// </returns>
int GPIO_OpenAsInput(GPIO_Id gpioId);

/// <summary>
///     Opens a GPIO (General Purpose Input/Output) as an output.
///     <para>An output GPIO may be configured as <see
///     cref="GPIO_OutputMode_PushPull">push-pull</see>, <see cref="GPIO_OutputMode_OpenDrain">open
///     drain</see> or <see cref="GPIO_OutputMode_OpenSource">open source</see>. Call <see
///     cref="GPIO_SetValue"/> on an open output GPIO to set the output value. </para>
///     <para> You can also call <see cref="GPIO_GetValue"/> on an open output GPIO to read the
///     current value (for example, when the output GPIO is configured as <see
///     cref="GPIO_OutputMode_OpenDrain">open drain</see> or <see
///     cref="GPIO_OutputMode_OpenSource">open source</see>).</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: access to <paramref name="gpioId"/> is not permitted as the GPIO is not listed
///     in the Gpio field of the application manifest.</para>
///     <para>EBUSY: the <paramref name="gpioId"/> is already open.</para>
///     <para>ENODEV: the <paramref name="gpioId"/> is invalid.</para>
///     <para>EINVAL: the <paramref name="outputMode"/> is not a valid <see cref="GPIO_OutputMode"/>
///     or the <paramref name="initialValue"/> is not a valid <see cref="GPIO_Value"/>.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="gpioId">
///     A <see cref="GPIO_Id"/> identifying the GPIO.
/// </param>
/// <param name="outputMode">
///     The output mode of the GPIO. An output may be configured as
///     <see cref="GPIO_OutputMode_PushPull">push-pull</see>,
///     <see cref="GPIO_OutputMode_OpenDrain">open drain</see> or
///     <see cref="GPIO_OutputMode_OpenSource">open source</see>.
/// </param>
/// <param name="initialValue">
///     The initial <see cref="GPIO_Value"/> for the GPIO - <see cref="GPIO_Value_High"/> or
///     <see cref="GPIO_Value_Low"/>.
/// </param>
/// <returns>
///     A file descriptor for the opened GPIO on success, or -1 for failure, in which case errno will
///     be set to the error value.
/// </returns>
int GPIO_OpenAsOutput(GPIO_Id gpioId, GPIO_OutputMode_Type outputMode,
                      GPIO_Value_Type initialValue);

/// <summary>
///     Get the current value of a GPIO.
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="outValue"/> is NULL. </para>
///     <para>EBADF: the <paramref name="gpioFd"/> is not valid.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="gpioFd">
///     The file descriptor for the GPIO.
/// </param>
/// <param name="outValue">
///     The <see cref="GPIO_Value"/> read from the GPIO - <see cref="GPIO_Value_High"/> or
///     <see cref="GPIO_Value_Low"/>.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno will be set to the error value.
/// </returns>
int GPIO_GetValue(int gpioFd, GPIO_Value_Type *outValue);

/// <summary>
///     Set the output value for a an output GPIO.
///     <para>Only has an effect on GPIOs opened as outputs.</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EINVAL: the <paramref name="value"/> is not a <see cref="GPIO_Value"/>.</para>
///     <para>EBADF: the <paramref name="gpioFd"/> is not valid.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="gpioFd">
///     The file descriptor for the GPIO.
/// </param>
/// <param name="value">
///     The <see cref="GPIO_Value"/> to set - <see cref="GPIO_Value_High"/> or
///     <see cref="GPIO_Value_Low"/>.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno will be set to the error value.
/// </returns>
int GPIO_SetValue(int gpioFd, GPIO_Value_Type value);

#ifdef __cplusplus
}
#endif