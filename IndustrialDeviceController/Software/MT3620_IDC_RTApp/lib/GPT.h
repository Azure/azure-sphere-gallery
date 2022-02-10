/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef AZURE_SPHERE_GPT_H_
#define AZURE_SPHERE_GPT_H_

#include "Common.h"
#include "Platform.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/// <summary>Returned when user tries to use running timer.</summary>
#define ERROR_GPT_ALREADY_RUNNING       (ERROR_SPECIFIC - 1)

/// <summary>Returned when user tries to stop or restart non-running timer</summary>
#define ERROR_GPT_NOT_RUNNING           (ERROR_SPECIFIC - 2)

/// <summary>Returned when user tries to use invalid timeout</summary>
#define ERROR_GPT_TIMEOUT_INVALID       (ERROR_SPECIFIC - 3)

/// <summary>Returned when user tries to set invalid speed</summary>
#define ERROR_GPT_SPEED_INVALID         (ERROR_SPECIFIC - 4)

/// <summary>Returned when user tries to pause a paused timer</summary>
#define ERROR_GPT_ALREADY_PAUSED        (ERROR_SPECIFIC - 5)

/// <summary>Returned when user tries to resume a timer not paused</summary>
#define ERROR_GPT_NOT_PAUSED            (ERROR_SPECIFIC - 6)

/// <summary>Opaque GPT handle.</summary>
typedef struct GPT GPT;

typedef enum {
    GPT_MODE_ONE_SHOT = 0,
    GPT_MODE_REPEAT   = 1,
    GPT_MODE_NONE     = 2,
    GPT_MODE_COUNT
} GPT_Mode;

typedef enum {
    GPT_UNITS_SECOND   = 1,
    GPT_UNITS_MILLISEC = 1000,
    GPT_UNITS_MICROSEC = 1000000
} GPT_Units;

/// <summary>
/// <para>Acquires a handle to the timer's context.</para>
/// </summary>
/// <param name="id">Which timer to initialize and acquire a handle for.</param>
/// <param name="speedHz">Target speed to initialise the timer at (constructor
/// will find closest value supported by HW).</param>
/// <param name="mode">Mode that the timer should run in (if unsupported, then
/// this will return NULL)</param>
/// <returns>A handle to a timer or NULL on failure.</returns>
GPT* GPT_Open(int32_t id, float speedHz, GPT_Mode mode);

/// <summary>
/// <para>Releases a timer handle.
/// Once released the handle is free to be opened again.</para>
/// </summary>
/// <param name="handle">The GPT handle which is to be released.</param>
void GPT_Close(GPT *handle);

/// <summary>
/// <para>Set timer speed.</para>
/// </summary>
/// <param name="handle">Timer to update.</param>
/// <param name="speedHz">Target speed - function will find closest value
/// supported by HW).</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t GPT_SetSpeed(GPT *handle, float speedHz);

/// <summary>
/// <para>Set timer mode (one shot or repeat).</para>
/// </summary>
/// <param name="handle">Timer to update.</param>
/// <param name="mode">Mode that the timer should run in (if unsupported, then
/// this will error with ERROR_UNSUPPORTED)</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t GPT_SetMode(GPT *handle, GPT_Mode mode);

/// <summary>
/// <para>Gives user access to platform unit ID (useful in callbacks)</para>
/// </summary>
/// <param name="handle">The GPT handle to be queried.</param>
int32_t GPT_GetId(GPT *handle);

/// <summary>
/// <para>Returns enabled status of timer. If the timer is enabled, some
/// operations are disallowed</para>
/// </summary>
/// <param name="handle">The GPT handle to the timer.</param>
/// <returns>Boolean with enabled state.</returns>
bool GPT_IsEnabled(GPT *handle);

/// <summary>
/// <para>Allows user to recover actual set speed of timer (may be different
/// from value passed at construction.</para>
/// </summary>
/// <param name="handle">The GPT handle to the timer.</param>
/// <param name="speedHz">User owned pointer to unsigned, function will write
/// speed value in Hz to this if the function returns ERROR_NONE.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t GPT_GetSpeed(GPT *handle, float *speedHz);

/// <summary>
/// <para>Gives user access to the timer mode set.</para>
/// </summary>
/// <param name="handle">The GPT handle to the timer.</param>
/// <param name="mode">User owned pointer to GPT_Mode; function will write
/// mode value if the function returns ERROR_NONE.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t GPT_GetMode(GPT *handle, GPT_Mode *mode);

/// <summary>
/// <para>Returns timer count.</para>
/// </summary>
/// <param name="handle">The GPT handle to the timer.</param>
/// <returns>Timer count or 0 on failure.</returns>
uint32_t GPT_GetCount(GPT *handle);

/// <summary>
/// <para>Returns timer running time in [units].</para>
/// </summary>
/// <param name="handle">The GPT handle to the timer.</param>
/// <param name="units">Units of timeout specified.</param>
/// <returns>Timer running time or 0 on failure.</returns>
uint32_t GPT_GetRunningTime(GPT *handle, GPT_Units units);

/// <summary>
/// <para>Gives user access to number of times timer has expired and
/// restarted.</para>
/// </summary>
/// <param name="handle">The GPT handle to the timer.</param>
/// <param name="numCycles">User owned pointer to unsigned; function will write
/// number of cycles to this if function returns ERROR_NONE.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t GPT_GetNumCycles(GPT *handle, uint32_t *numCycles);

/// <summary>
/// <para>Stops passed timer.</para>
/// </summary>
/// <param name="handle">The GPT handle to the timer.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t GPT_Stop(GPT *handle);

/// <summary>
/// <para>Pauses timer and saves state - recovered using GPT_Resume.</para>
/// </summary>
/// <param name="handle">The GPT handle to the timer.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t GPT_Pause(GPT *handle);

/// <summary>
/// <para>Recovers paused timer's state and sets to enabled.</para>
/// </summary>
/// <param name="handle">The GPT handle to the timer.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t GPT_Resume(GPT *handle);

/// <summary>
/// <para>Register a callback for the supplied timer (if timer interrupt
/// based). Only one callback can be registered at a time for each timer.
/// If the timer is already running, user should close it first, reopen and
/// start new timer.</para>
/// <para>Only call this function from the main application thread or from a
/// timer callback.</para>
/// </summary>
/// <param name="handle">Which hardware timer to use.</param>
/// <param name="timeout">Timeout period in [units].</param>
/// <param name="units">Units of timeout specified.</param>
/// <param name="callback">Function to invoke in interrupt context when the
/// timer expires. Is passed a handle to the timer, so has callback access to
/// this same API. Note that this callback happens within an interrupt, so
/// if there is significant computation, it might be best to defer execution</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t GPT_StartTimeout(
    GPT       *handle,
    uint32_t   timeout,
    GPT_Units  units,
    void       (*callback)(GPT *));

/// <summary>
/// <para>Start timer in freerun mode.</para>
/// </summary>
/// <param name="handle">Which hardware timer to use.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t GPT_Start_Freerun(GPT *handle);

/// <summary>
/// <para>Wait for timeout to expire. If the timer doesn't
/// support the level of precision requested (i.e. not all timers support
/// microsecond precison) then ERROR_GPT_TIMEOUT_INVALID is returned.
/// </summary>
/// <param name="handle">Which hardware timer to use.</param>
/// <param name="timeout">Timeout period in [units].</param>
/// <param name="units">Units of timeout specified.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t GPT_WaitTimer_Blocking(
    GPT       *handle,
    uint32_t   timeout,
    GPT_Units  units);


///------------------------Test helpers--------------------------

#define GPT_MAX_TEST_SPEEDS 8

typedef struct {
    uint32_t speeds[GPT_MAX_TEST_SPEEDS];
    uint32_t count;
} GPT_TestSpeeds;

/// <summary>
/// <para>Helper function; Allows user to query what speeds are available.
/// If the timer is 2-speed, then this function writes .</para>
/// <para>If the timer supports multiple speeds, then function allows the user
/// access to some intermediate speeds provided by the HW.
/// Use should read datasheet to get more detail on which speeds are supported
/// by which timers.</para>
/// </summary>
/// <param name="handle">Which hardware timer to use.</param>
/// <param name="testSpeeds">GPT_TestSpeeds object; to which will be written all
/// test speeds and the number of test speeds</param>
void GPT_GetTestSpeeds(GPT *handle, GPT_TestSpeeds *testSpeeds);

#ifdef __cplusplus
 }
#endif

#endif // #ifndef AZURE_SPHERE_GPT_H_
