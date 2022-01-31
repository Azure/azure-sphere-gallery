/// \file applications.h
/// \brief This header contains the functions and types needed to acquire information about
/// all applications.
///
#pragma once

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <applibs/applications_internal.h>

/// <summary>
///     Get the total memory usage in kilobytes. This is the total amount of memory currently used 
///     by the applications. It includes both user mode and kernel allocations and is returned as a 
///     raw value (in kB).
///     <para> **Errors** </para>
///     If an error is encountered, returns 0 and sets errno to the error value.
///     <para>EAGAIN: Information temporarily unavailable. The call may work if tried again later.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <returns>
///     0 on error, in which case errno is set to the error value, or the number of Kilobytes
///     of memory allocated.
/// </returns>
static size_t Applications_GetTotalMemoryUsageInKB(void);

/// <summary>
///     Get the user mode memory usage in kilobytes. This is the amount of memory allocated by applications
///     which includes allocations made by shared libraries. It is returned as a raw value (in kB).
///     <para> **Errors** </para>
///     If an error is encountered, returns 0 and sets errno to the error value.
///     <para>EAGAIN: Information temporarily unavailable. The call may work if tried again later.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <returns>
///     0 on error, in which case errno is set to the error value, or the number of Kilobytes
///     of memory allocated.
/// </returns>
static size_t Applications_GetUserModeMemoryUsageInKB(void);

/// <summary>
///     Get the peak user mode memory usage in kilobytes. This is the high watermark or the maximum value 
///     of user mode allocations. It is returned as a raw value (in kB).
///     <para> **Errors** </para>
///     If an error is encountered, returns 0 and sets errno to the error value.
///     <para>EAGAIN: Information temporarily unavailable. The call may work if tried again later.</para>
///     Any other errno may also be specified; such errors aren't deterministic and there's no
///     guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <returns>
///     0 on error, in which case errno is set to the error value, or the number of Kilobytes
///     of memory allocated.
/// </returns>
static size_t Applications_GetPeakUserModeMemoryUsageInKB(void);

#ifdef __cplusplus
}
#endif
