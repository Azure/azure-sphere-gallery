/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef AZURE_SPHERE_PRINT_H_
#define AZURE_SPHERE_PRINT_H_

#include "UART.h"
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
 extern "C" {
#endif

/// <summary>Returned when user tries to use printf with invalid fmt spec.</summary>
#define ERROR_UART_PRINTF_INVALID (ERROR_SPECIFIC - 1)

/// <summary>
/// <para>Buffers the supplied string and asynchronously writes it to the UART. Does not send
/// the null terminator. If there is not enough space to buffer the entire string, then it will
/// block.</para>
/// <para>See <see cref="UART_Write" />
/// for more information.</para>
/// </summary>
/// <param name="handle">Which UART to write the string to.</param>
/// <param name="msg">Null-terminated string to write to the UART.</param>
/// <returns>ERROR_NONE on success, or an error code.</returns>
int32_t UART_Print(UART *handle, const char *msg);

/// <summary>
/// <para>Encodes the supplied unsigned integer as a string and asynchronously writes it to the
/// UART. If there is not enough space to buffer the entire string, then it will block.</para>
/// <para>See <see cref="UART_Write" /> for more information.</para>
/// </summary>
/// <param name="handle">Which UART to print the value to.</param>
/// <param name="value">Value to print to the UART.</param>
/// <param name="base">Number base in which to print the integer (1 to 36).</param>
/// <param name="width">Fixed width to print, or zero for automatic.</param>
/// <param name="upper">For bases above 10, use upper case characters.</param>
/// <returns>ERROR_NONE on success, or an error code.</returns>
int32_t UART_PrintUIntBase(UART *handle, int32_t value, unsigned base, unsigned width, bool upper);

/// <summary>
/// <para>Encodes the supplied signed integer as a string and asynchronously writes it to the
/// UART. If there is not enough space to buffer the entire string, then it will block.</para>
/// <para>See <see cref="UART_Write" /> for more information.</para>
/// </summary>
/// <param name="handle">Which UART to print the value to.</param>
/// <param name="value">Value to print to the UART.</param>
/// <param name="base">Number base in which to print the integer (1 to 36).</param>
/// <param name="width">Fixed width to print, or zero for automatic.</param>
/// <param name="upper">For bases above 10, use upper case characters.</param>
/// <returns>ERROR_NONE on success, or an error code.</returns>
int32_t UART_PrintIntBase(UART *handle, int32_t value, unsigned base, unsigned width, bool upper);


/// <summary>
/// <para>Encodes the supplied signed integer as a decimal string and asynchronously writes it to
/// the UART. If there is not enough space to buffer the entire string, then it will block.</para>
/// <para>See <see cref="UART_PrintIntBase" /> for more information.</para>
/// </summary>
/// <param name="handle">Which UART to print the value to.</param>
/// <param name="value">Value to print to the UART.</param>
/// <returns>ERROR_NONE on success, or an error code.</returns>
static inline int32_t UART_PrintInt(UART *handle, int32_t value)
{
    return UART_PrintIntBase(handle, value, 10, 0, false);
}

/// <summary>
/// <para>Encodes the supplied signed integer as a decimal string and asynchronously writes it to
/// the UART. If there is not enough space to buffer the entire string, then it will block.</para>
/// <para>See <see cref="UART_PrintIntBase" /> for more information.</para>
/// </summary>
/// <param name="handle">Which UART to print the value to.</param>
/// <param name="value">Value to print to the UART.</param>
/// <returns>ERROR_NONE on success, or an error code.</returns>
static inline int32_t UART_PrintUInt(UART *handle, uint32_t value)
{
    return UART_PrintUIntBase(handle, value, 10, 0, false);
}

/// <summary>
/// <para>Encodes the supplied unsigned integer as a hex string and asynchronously writes it to
/// the UART. If there is not enough space to buffer the entire string, then it will block.</para>
/// <para>See <see cref="UART_PrintIntBase" /> for more information.</para>
/// </summary>
/// <param name="handle">Which UART to print the value to.</param>
/// <param name="value">Value to print to the UART.</param>
/// <returns>ERROR_NONE on success, or an error code.</returns>
static inline int32_t UART_PrintHex(UART *handle, uint32_t value)
{
    return UART_PrintUIntBase(handle, value, 16, 8, false);
}

/// <summary>
/// <para>Encodes the supplied unsigned integer as a hex string and asynchronously writes it to
/// the UART. If there is not enough space to buffer the entire string, then it will block.</para>
/// <para>See <see cref="UART_PrintIntBase" /> for more information.</para>
/// </summary>
/// <param name="handle">Which UART to print the value to.</param>
/// <param name="value">Value to print to the UART.</param>
/// <param name="width">Fixed width to print, or zero for automatic.</param>
/// <returns>ERROR_NONE on success, or an error code.</returns>
static inline int32_t UART_PrintHexWidth(UART *handle, uint32_t value, uintptr_t width)
{
    return UART_PrintUIntBase(handle, value, 16, width, false);
}

/// <summary>
/// <para>Subset of printf functionality for UART, supports format specs:
/// %d, %u, %f, %x, %o, %c and %s. Also supports width and significant place
/// specification (i.e. %08.7f) </para>
/// </summary>
/// <param name="handle">Which UART to print the value to.</param>
/// <param name="format">Format string.</param>
/// <param name="...">Subsequent arguments are the objects to be printed.</param>
/// <returns>ERROR_NONE on success, or an error code.</returns>
int32_t UART_Printf(UART *handle, const char *format, ...)
    __attribute__ ((format (printf, 2, 3)));

/// <summary>
/// <para>Non variadic variant of printf that consumes a variable array (va_list)
/// </summary>
/// <param name="handle">Which UART to print the value to.</param>
/// <param name="format">Format string.</param>
/// <param name="args">va_list managed by user code.</param>
/// <returns>ERROR_NONE on success, or an error code.</returns>
int32_t UART_vPrintf(UART *handle, const char *format, va_list args);

#ifdef __cplusplus
 }
#endif

#endif
