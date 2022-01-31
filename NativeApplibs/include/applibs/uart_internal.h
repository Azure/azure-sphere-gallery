/// \file uart_internal.h
/// \brief This header contains internal functions of the UART API and should not be included
/// directly; include applibs/uart.h instead.
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///    Version support info for the UART_Config struct. Refer to UART_Config and the appropriate
///    uart_config_v[n].h header for actual struct fields.
/// </summary>
struct z__UART_Config_Base {
    /// <summary>Internal version field.</summary>
    uint32_t z__magicAndVersion;
};

/// <summary>
///    A magic value that provides version support for the UART_Config struct.
/// </summary>
#define z__UART_CONFIG_MAGIC 0xde2a0000

/// <summary>
///    Versioning support info for UART_InitConfig. Do not use this directly; use UART_InitConfig
///    instead.
/// </summary>
/// <param name="uartConfig">
///     A pointer to an object that contains the default UART settings. The object
///     is a <see cref="UART_Config"/> that is type cast as <see cref="z__UART_Config_Base"/>.
/// </param>
/// <param name="uartConfigStructVersion">
///     The <see cref="UART_Config"/> struct version.
/// </param>
/// <returns>
///    0 if uartConfig was initialized successfully, or -1 for failure, in which case errno
///    is set to the error value.
/// </returns>
int z__UART_InitConfig(struct z__UART_Config_Base *uartConfig, uint32_t uartConfigStructVersion);

/// <summary>
///     Initializes a <see cref="UART_Config"/> struct with the default UART settings.
/// </summary>
/// <param name="uartConfig">
///     A pointer to a <see cref="UART_Config"/> struct that returns the default UART settings.
/// </param>
static inline void UART_InitConfig(UART_Config *uartConfig)
{
    z__UART_InitConfig((struct z__UART_Config_Base *)uartConfig, UART_STRUCTS_VERSION);
}

/// <summary>
///    Adds struct version support to UART_Open. Do not use this directly; use UART_Open instead.
/// </summary>
/// <param name="uartId">
///     The ID of the UART to open.
/// </param>
/// <param name="uartConfig">
///     A pointer to an object that contains the UART settings. The object is a <see
///     cref="UART_Config"/> that is type cast as <see cref="z__UART_Config_Base"/>.
/// </param>
/// <returns>
///     The file descriptor of the UART if it was opened successfully. Returns -1
///     if the function fails, in which case errno is set to the error value.
/// </returns>
int z__UART_Open(UART_Id uartId, const struct z__UART_Config_Base *uartConfig);

/// <summary>
///     Opens a UART, configures the settings, and returns a file descriptor for subsequent calls
///     that operate on the UART.
///     <para>Opens a UART for exclusive access.</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: access to UART is not permitted as the <paramref
///     name="uartId"/> is not listed in the Uart field of the application manifest.</para>
///     <para>ENODEV: the <paramref name="uartId"/> is invalid.</para>
///     <para>EINVAL: the <paramref name="uartConfig"/> represents an invalid configuration.</para>
///     <para>EBUSY: the <paramref name="uartId"/> is already open.</para>
///     <para>EFAULT: the <paramref name="uartConfig"/> is NULL.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="uartId">
///     The ID of the UART to open.
/// </param>
/// <param name="uartConfig">
///     A pointer to a <see cref="UART_Config"/> object that contains the UART settings.
///     Call <see cref="UART_InitConfig"/> to get a UART_Config struct with default settings.
/// </param>
/// <returns>
///     The file descriptor of the UART if it was opened successfully. Returns -1
///     if the function fails, in which case errno is set to the error value.
/// </returns>
static inline int UART_Open(UART_Id uartId, const UART_Config *uartConfig)
{
    return z__UART_Open(uartId, (const struct z__UART_Config_Base *)uartConfig);
}

#ifdef __cplusplus
}
#endif
