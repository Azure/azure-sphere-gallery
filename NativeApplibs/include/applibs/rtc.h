/// \file rtc.h
/// \brief This header contains functionality for interacting with the hardware clock (real-time
/// clock or RTC). These functions are only permitted if the application has the SystemTime
/// capability in its application manifest.
///
/// While multiple applications are allowed to access this API, only one device can open the RTC
/// device at at time.
///
/// These functions are not thread safe.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
///     Synchronizes the hardware clock (RTC) to the current system time.  System time is
///     converted into UTC/GMT if necessary (i.e., based on the current timezone setting) before
///     being written to the RTC.
///
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: The calling application doesn't have the SystemTime capability.</para>
///     <para>EBUSY: The RTC device was in use and couldn't be opened. Caller should try again
///     periodically until it succeeds.</para>
///     <para>ENOENT: The RTC device wasn't found. The RTC device may have been disabled in
///     hardware.</para> Any other errno may also be specified; such errors aren't deterministic and
///     there's no guarantee that the same behaviour will be retained through system updates.
/// </summary>
/// <returns>
///     0 for success, -1 for failure, in which case errno is set to the error value.
/// </returns>
int clock_systohc(void);

#ifdef __cplusplus
}
#endif
