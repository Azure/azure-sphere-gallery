/// \file sysevent.h
/// \brief This header contains functions and types needed for system event notifications.
/// Applications can register for and unregister from update, power down, and shutdown
/// notifications. Apps can use these notifications to put themselves in a safe state before
/// application shutdown, or can attempt to defer these events.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <applibs/applibs_internal_api_traits.h>
#include <applibs/eventloop.h>

/// <summary>
///     Flags for system event types
/// </summary>
typedef uint32_t SysEvent_Events;
enum {
    /// <summary>
    ///     No event given
    /// </summary>
    SysEvent_Events_None = 0x00,

    /// <summary>
    ///     An OS or application update is ready for install
    /// </summary>
    SysEvent_Events_UpdateReadyForInstall = 0x01,

    /// <summary>
    ///     An OS or application update has started. This event will be followed by
    ///     the SysEvent_Events_UpdateReadyForInstall when the update is fully downloaded
    ///     and ready for installation. If no update is available,
    ///     SysEvent_Events_NoUpdateAvailable will be sent instead of this event.
    /// </summary>
    SysEvent_Events_UpdateStarted = 0x02,

    /// <summary>
    ///     No updates are available
    /// </summary>
    SysEvent_Events_NoUpdateAvailable = 0x04,

    /// <summary>
    ///     A mask of all valid system events
    /// </summary>
    SysEvent_Events_Mask = SysEvent_Events_UpdateStarted | SysEvent_Events_UpdateReadyForInstall |
                           SysEvent_Events_NoUpdateAvailable
};

/// <summary>
///     The status of the SysEvent_Events
/// </summary>
typedef uint32_t SysEvent_Status;
enum {
    /// <summary>
    ///     No value was specified.
    /// </summary>
    SysEvent_Status_Invalid = 0,
    /// <summary>
    ///     A warning that an event will occur, with the opportunity to defer the event.
    ///     The time in which the event will occur depends on the type of event.
    /// </summary>
    SysEvent_Status_Pending = 1,
    /// <summary>
    ///     A warning that an event will occur, without the opportunity to defer the event.
    ///     The time in which the event will occur depends on the type of event.
    /// </summary>
    SysEvent_Status_Final = 2,
    /// <summary>
    ///     The previously "Pending" event has been deferred, and will occur at a later time.
    /// </summary>
    SysEvent_Status_Deferred = 3,
    /// <summary>
    ///     The system event is complete.
    /// </summary>
    /// <remarks>
    ///     <para> For <see cref="SysEvent_Events_UpdateReadyForInstall"/>, this status is only sent
    ///     for application updates (<see cref="SysEvent_UpdateType_App"/>) because OS updates (<see
    ///     cref="SysEvent_UpdateType_System"/> require a device reboot to complete.</para>
    /// </remarks>
    SysEvent_Status_Complete = 4
};

/// <summary>
///     The type of update being applied
/// </summary>
typedef uint32_t SysEvent_UpdateType;
enum {
    /// <summary>
    ///     No value was specified.
    /// </summary>
    SysEvent_UpdateType_Invalid = 0,
    /// <summary>
    ///     An application update that will restart the updated application but will not
    ///     reboot the device
    /// </summary>
    SysEvent_UpdateType_App = 1,
    /// <summary>
    ///     An OS software update that will require a device reboot
    /// </summary>
    SysEvent_UpdateType_System = 2
};

/// <summary>
///     A struct containing information about update events.  This struct is returned by
///     SysEvent_Info_GetUpdateData after passing in a SysEvent_Info returned by a
///     SysEvent_EventsCallback.  This struct is only valid when the event type is
///     SysEvent_Events_UpdateReadyForInstall. Note: max_deferral_time_in_minutes is only
///     defined for SysEvent_Status_Pending
/// </summary>
typedef struct SysEvent_Info_UpdateData {
    unsigned int max_deferral_time_in_minutes;
    SysEvent_UpdateType update_type;
} SysEvent_Info_UpdateData;

/// <summary>
///     An opaque struct containing information about a system event. Data in this struct cannot be
///     accessed directly; rather, they must be accessed by calling a SysEvent_Info_Get function
///     specific to the event (such as <see cref="SysEvent_Info_GetUpdateData"/> for
///     <see cref="SysEvent_Events_UpdateReadyForInstall"/>).
///     events).
/// </summary>
typedef struct SysEvent_Info SysEvent_Info;

/// <summary>
///     Used to retrieve update information such as the maximum deferral time in minutes and the
///     type of update. This function can only be called with the <paramref name="info"/> parameter
///     passed to a <see cref="SysEvent_EventsCallback"/> before the callback returns.
///
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: <paramref name="info"/> and/or <paramref name="update_info"/> is NULL. </para>
///     <para>EINVAL: <paramref name="info"/> is not from a <see
///     cref="SysEvent_Events_UpdateReadyForInstall"/> event.</para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same
///     behavior will be retained through system updates.</para>
/// </summary>
/// <param name="info">
///     A pointer containing information about the SysEvent_Events, retrieved from a
///     SysEvent_EventsCallback
/// </param>
/// <param name="update_info">
///     A pointer to a SysEvent_Info_UpdateData structure that will be populated with the software
///     update information if successful
/// </param>
/// <returns>
///      On success, returns 0
///      On error, returns -1 and sets errno
/// </returns>
int SysEvent_Info_GetUpdateData(const SysEvent_Info *info, SysEvent_Info_UpdateData *update_info);

/// <summary>
///     This function will be called whenever a registered event changes status.  Only one
///     SysEvent_Events flag will be marked, since there is exactly one callback call for each event
///     status change.
/// </summary>
/// <param name="event"> The event which has changed state </param>
/// <param name="state"> The state which the event has changed to </param>
/// <param name="info"> Additional info about the state change. To retrieve more information,
///     the pointer needs to be passed to an event-specific function (such as <see
///     cref="SysEvent_Info_GetUpdateData"/>. The pointer is valid only until the callback returns.
/// </param>
/// <param name="context"> An optional context pointer which was passed in the registration </param>
typedef void SysEvent_EventsCallback(SysEvent_Events event, SysEvent_Status state,
                                     const SysEvent_Info *info, void *context);

/// <summary>
///     Registers the application to the set of events given by eventBitMask. The event registration
///     is returned on success, and will need to be retained until it is passed to
///     <see cref="SysEvent_UnregisterForEventNotifications"/>.
///
///     Note: There must be only one active EventRegistration at a time for all system event
///     notifications.
///
///     <para> **Errors** </para>
///     If an error is encountered, returns NULL and sets errno to the error value.
///     <para>EACCES: The application doesn't have the SystemEventNotifications capability. </para>
///     <para>EFAULT: <paramref name="el"/> and/or <paramref name="callback"/> is NULL. </para>
///     <para>EINVAL: <paramref name="eventBitmask"/> specifies invalid events. </para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same behavior
///     will be retained through system updates. </para>
/// </summary>
/// <param name="el"> The event loop to which the EventRegistration will be registered </param>
/// <param name="eventBitmask"> A bitmask with the events to listen for </param>
/// <param name="callback"> Pointer to a <see cref="SysEvent_EventsCallback" /> to call when any of
/// the specified events change state.</param>
/// <param name="context"> Optional context to pass to the <paramref name="callback"/> function.
/// </param>
/// <returns>
///      On success returns a pointer to an EventRegistration
///      On error returns nullptr and sets errno
/// </returns>
EventRegistration *SysEvent_RegisterForEventNotifications(EventLoop *el,
                                                          SysEvent_Events eventBitmask,
                                                          SysEvent_EventsCallback *callback,
                                                          void *context);

/// <summary>
///     Unregisters for the system notifications which were registered to by
///     SysEvent_RegisterForEventNotifications.
///     The EventRegistration must have been previously registered, and each EventRegistration
///     may only be unregistered once.
///
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: <paramref name="reg"/> is NULL. </para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same behavior
///     will be retained through system updates. </para>
/// </summary>
/// <param name="reg"> The event registration to remove from the event loop </param>
/// <returns>
///      On success returns 0
///      On error returns -1 and sets errno
/// </returns>
int SysEvent_UnregisterForEventNotifications(EventRegistration *reg);

/// <summary>
///     Attempts to defer an event for the specified duration. This function can be called when the
///     event status is <see cref="SysEvent_Status_Pending"/> to defer the event. If the event is
///     not pending, the call fails. If the specified duration is longer than the maximum allowed
///     for the event, the duration will be truncated.
///
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: The application doesn't have the capability to defer the event specified.
///     </para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same
///     behavior will be retained through system updates. </para>
/// </summary>
/// <param name="event"> The type of event which is being deferred. It must specify only one event
/// at a time.</param>
/// <param name="requested_defer_time_in_minutes"> The amount of time in minutes
/// for which the event will be deferred.</param>
/// <returns>
///     On success returns 0.
///     On error returns -1 and sets errno.
/// </returns>
/// <remarks>
///     This function is synchronous. If the call is successful the event is deferred and the
///     <see cref="SysEvent_Status_Deferred"/> will be sent to all applications registered for the
///     event.
///
///     <para> **Required Capabilities** </para>
///     <para><see cref="SysEvent_Events_UpdateReadyForInstall"/> requires
///     SoftwareUpdateDeferral.</para>
/// </remarks>
int SysEvent_DeferEvent(SysEvent_Events event, uint32_t requested_defer_time_in_minutes);

/// <summary>
///     Attempts to resume an event if it is deferred.
///
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EACCES: The application doesn't have the capability to resume the event specified.
///     </para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same
///     behavior will be retained through system updates. </para>
/// </summary>
/// <param name="event"> The type of event which is being resumed. It must specify only one event at
/// a time.</param>
/// <returns>
///      On success returns 0
///      On error returns -1 and sets errno
/// </returns>
/// <remarks>
///     This function can only be called if <see cref="SysEvent_DeferEvent"/> was called
///     successfully, otherwise the call will fail. SysEvent_ResumeEvent is not required after a
///      <see cref="SysEvent_DeferEvent"/> call. It's only useful if the deferral is no longer
///     needed.
/// </remarks>
int SysEvent_ResumeEvent(SysEvent_Events event);

#ifdef __cplusplus
}
#endif
