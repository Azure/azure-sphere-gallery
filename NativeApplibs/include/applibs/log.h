/// \file log.h
/// \brief This header contains functions for logging debug messages.
///
/// Debug messages are displayed when debugging the application through the Azure Sphere SDK.
///
/// This function is thread safe.
#pragma once

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     Logs and formats a debug message with printf formatting.
///     <para>The caller needs to provide an additional parameter for every argument specification
///     defined in the <paramref name="fmt"/> string.</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="fmt"/> is NULL.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="fmt">
///     The message string to log, with optional argument specifications.
/// </param>
/// <returns>
///      0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Log_Debug(const char *fmt, ...);

/// <summary>
///     Logs and formats a debug message with vprintf formatting.
///     <para>The <paramref name="args"/> va_list parameter should be initialized with va_start
///     before calling this function, and should be cleaned up by calling va_end afterwards.</para>
///     <para>The caller needs to provide an additional parameter for every argument specification
///     defined in the <paramref name="fmt"/> string.</para>
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: the <paramref name="fmt"/> is NULL.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <param name="fmt">
///     The message string to log.
/// </param>
/// <param name="args">
///     An argument list that has been initialized with va_start.
/// </param>
/// <returns>
///      0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int Log_DebugVarArgs(const char *fmt, va_list args);

#ifdef __cplusplus
}
#endif