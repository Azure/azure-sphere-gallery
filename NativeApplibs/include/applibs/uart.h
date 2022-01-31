/// \file uart.h
/// \brief This header contains the functions and types needed to open and use a UART
/// (Universal Asynchronous Receiver/Transmitter) on the device. Access to individual UARTs is
/// restricted based on the UART field of the application's manifest.
///
/// To use these functions, define UART_STRUCTS_VERSION in your source code with the structure
/// version you're using. Currently, the only valid version is 1. Thereafter, you can use the
/// friendly names of the UART_ structures, which start with UART_.
#pragma once

#include <applibs/uart_config_v1.h>

// Default the UART_STRUCTS_VERSION version to 1
#ifndef UART_STRUCTS_VERSION
#define UART_STRUCTS_VERSION 1
#endif

#if UART_STRUCTS_VERSION == 1
/// <summary>
///     Alias to the z__UART_Config_v1 structure. After you define
///     UART_STRUCTS_VERSION, you can refer to z__UART_Config_v1
///     structures with this alias.
/// </summary>
typedef struct z__UART_Config_v1 UART_Config;
#else
#error To use applibs/uart.h you must first #define a supported UART_STRUCTS_VERSION
#endif

/// <summary>
///     A UART ID, which specifies a UART peripheral instance.
/// </summary>
typedef int UART_Id;

#include <applibs/uart_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     Initializes a <see cref="UART_Config"/> struct with the default UART settings.
/// </summary>
/// <param name="uartConfig">
///     A pointer to a <see cref="UART_Config"/> struct that returns the default UART settings.
/// </param>
static inline void UART_InitConfig(UART_Config *uartConfig);

/// <summary>
///     Opens and configures a UART, and returns a file descriptor to use
///     for subsequent calls.
///     <para>Opens the UART for exclusive access.</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: access to UART is not permitted as the <paramref
///     name="uartId"/> is not listed in the Uart field of the application manifest.</para>
///     <para>ENODEV: the <paramref name="uartId"/> is invalid.</para>
///     <para>EINVAL: the <paramref name="uartConfig"/> represents an invalid configuration.</para>
///     <para>EBUSY: the <paramref name="uartId"/> is already open.</para>
///     <para>EFAULT: the <paramref name="uartConfig"/> is NULL.</para>
///     Any other errno may also be specified, but there's no guarantee that the same behavior will
///     be retained through system updates.
/// </summary>
/// <param name="uartId">
///     The ID of the UART to open.
/// </param>
/// <param name="uartConfig">
///     A pointer to a UART_Config struct that defines the configuration of the UART.
///     Call <see cref="UART_InitConfig"/> to get a UART_Config with default settings.
/// </param>
/// <returns>
///     The file descriptor of the UART if if was opened successfully, or -1 for failure,
///     in which case errno is set to the error value.
/// </returns>
static inline int UART_Open(UART_Id uartId, const UART_Config *uartConfig);

#ifdef __cplusplus
}
#endif
