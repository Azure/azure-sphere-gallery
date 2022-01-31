/// \file eventloop.h
/// \brief This header contains functions for the EventLoop construct.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/// <summary>
///     An EventLoop object monitors event sources and dispatches their events to handlers.
/// </summary>
/// <remarks>
///     An EventLoop object is single-threaded. The application can use one or more EventLoop
///     objects per thread, but each object must be used in only one thread. Calling an EventLoop
///     API for the same object from multiple threads will result in undefined behavior.
///
///     To dispatch the events that need processing, the application must call <see
///     cref="EventLoop_Run"/>. The handlers for the events will be called in the same thread where
///     <see cref="EventLoop_Run"/> was called.
/// </remarks>
typedef struct EventLoop EventLoop;

/// <summary>
///     An EventRegistration is a handle returned when a callback is registered with an event
///     source. This handle is later used to unregister the callback with the same source.
/// </summary>
typedef struct EventRegistration EventRegistration;

/// <summary>
///     Creates an EventLoop object.
/// </summary>
/// <returns>
///     A pointer to a new EventLoop object. To avoid memory leaks, the returned object must be
///     closed with <see cref="EventLoop_Close"/> once it is not needed anymore.
/// </returns>
EventLoop *EventLoop_Create(void);

/// <summary>
///     Closes the EventLoop object and releases its memory.
/// </summary>
/// <param name="el"> The EventLoop to close. </param>
/// <remarks>
///     Once <see cref="EventLoop_Close"/> returns, the pointer to the EventLoop object is invalid
///     since the memory is freed for other uses. Trying to use the pointer to an EventLoop that has
///     been closed will lead to undefined behavior.
/// </remarks>
void EventLoop_Close(EventLoop *el);

/// <summary>
///     Possible return conditions for <see cref="EventLoop_Run"/>.
/// </summary>
typedef int EventLoop_Run_Result;
enum {
    /// <summary> <see cref="EventLoop_Run"/> failed; errno has the specific error code. </summary>
    EventLoop_Run_Failed = -1,
    /// <summary> <see cref="EventLoop_Run"/> finished without processing any events. </summary>
    EventLoop_Run_FinishedEmpty = 0,
    /// <summary> <see cref="EventLoop_Run"/> finished after processing one or more events.
    /// </summary>
    EventLoop_Run_Finished = 1,
};

/// <summary>
///     Runs the EventLoop and dispatches pending events in the caller's thread of execution.
///
///     <para> **Errors** </para>
///     If an error is encountered, returns <see cref="EventLoop_Run_Failed"/> and sets errno to the
///     error value.
///     <para>EFAULT: <paramref name="el"/> is NULL. </para>
///     <para>EBUSY: EventLoop_Run was called recursively, which is not supported. </para>
///     <para>EINTR: the call was interrupted by a singal handler. </para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same behavior
///     will be retained through system updates.</para>
/// </summary>
/// <param name="el"> The EventLoop to run. </param>
/// <param name="duration_in_milliseconds">
///     The length of time to run the event loop. If 0, the loop will process one event (if one is
///     ready) and break immediately, regardless of the value of <paramref
///     name="process_one_event"/>. If &gt; 0, the loop will run for the specified duration or until
///     it is interrupted. If &lt; 0, the loop will keep running until interrupted. See <see
///     cref="EventLoop_Stop"/> and <see cref="process_one_event"/> for additional conditions.
/// </param>
/// <param name="process_one_event">
///     Whether to break the loop after the first event is processed. If false, the loop will keep
///     running for the duration specified by <see cref="duration_in_milliseconds"/> or until it is
///     interrupted (see <see cref="EventLoop_Stop"/>). This parameter is ignored if <paramref
///     name="duration_in_milliseconds"/> is 0.
/// </param>
/// <returns>
///     See <see cref="EventLoop_Run_Result"/>.
/// </returns>
/// <remarks>
///     The application can call <code>EventLoop_Run(el, -1, false)</code> to surrender the control
///     of the calling thread to the EventLoop.
///     If the application calls <code>EventLoop_Run(el, -1, true)</code>, the loop will block
///     waiting for the first event to become ready and then it will process the event and return.
///
///     An event handler can call <see cref="EventLoop_Stop"/> to exit the current <see
///     cref="EventLoop_Run"/> earlier.
/// </remarks>
EventLoop_Run_Result EventLoop_Run(EventLoop *el, int duration_in_milliseconds,
                                   bool process_one_event);

/// <summary>
///     Gets a descriptor that will be signaled for input when the EventLoop has work ready to
///     process.
///     The application can wait or poll on this descriptor to determine when the EvenLoop needs to
///     be executed via EventLoop_Run.
///
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: <paramref name="el"/> is NULL. </para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same behavior
///     will be retained through system updates.</para>
/// </summary>
/// <param name="el"> The EventLoop to get the descriptor of. </param>
/// <returns>
///     The waitable descriptor on success, or -1 on failure, in which case errno is set to the
///     error value.
/// </returns>
int EventLoop_GetWaitDescriptor(EventLoop *el);

/// <summary>
///     Stops the EventLoop's current run and causes <see cref="EventLoop_Run"/> to return control
///     to its caller.
///
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: <paramref name="el"/> is NULL. </para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same behavior
///     will be retained through system updates.</para>
/// </summary>
/// <returns>
///     0 for success, or -1 for failure, in which case errno will be set to the error value.
/// </returns>
/// <remarks>
///     This function can be called from an event callback or another thread to stop the current
///     loop and return from <see cref="EventLoop_Run"/>.
///
///     If called from a callback, EventLoop_Run will stop synchronously. Once EventLoop_Stop
///     returns, no further events will be processed by EventLoop_Run until the callback returns and
///     the former, on its turn, can return to its caller. EventLoop_Run will not process any
///     further events and will return to its caller. If called from another thread, EventLoop_Run
///     will stop asynchronously and return to its caller.
///
///     If called from another thread, EventLoop_Run will stop asynchronously. That means
///     EventLoop_Run may still process a few events after EventLoop_Stop returns and before the
///     former returns to its caller.
///
///     An EventLoop object is a single-threaded object. Trying to use the same EventLoop from
///     multiple threads simultaneously will result in undefined behavior. The only exception is
///     EventLoop_Stop.
/// </remarks>
int EventLoop_Stop(EventLoop *el);

/// <summary>
///     A bitmask of the I/O events that can be captured by the EventLoop object.
/// </summary>
typedef uint32_t EventLoop_IoEvents;
enum {
    /// <summary> Specifies no I/O event should be used. </summary>
    EventLoop_None = 0x00,
    /// <summary> The descriptor is available for read operations. </summary>
    EventLoop_Input = 0x01,
    /// <summary> The descriptor is available for write operations. </summary>
    EventLoop_Output = 0x04,
    /// <summary> An error condition happended on the descriptor. The EventLoop will always report
    /// this event independently of the bitmask passed to <see cref="EventLoop_RegisterIo"/>
    /// </summary>
    EventLoop_Error = 0x08,
};

/// <summary>
///     The callback invoked by the EventLoop when a registered I/O event occurs.
/// </summary>
/// <param name="el"> The EventLoop in which the callback was registered. </param>
/// <param name="fd"> The descriptor for which the event fired. </param>
/// <param name="events"> The bitmask of events fired. </param>
/// <param name="context"> An optional context pointer that was passed in the registration. </param>
typedef void EventLoopIoCallback(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);

/// <summary>
///     Registers an I/O event with the EventLoop.
///
///     <para> **Errors** </para>
///     If an error is encountered, returns NULL and sets errno to the error value.
///     <para>EFAULT: <paramref name="el"/> and/or <paramref name="callback"/> is NULL. </para>
///     <para>EINVAL: <paramref name="eventBitmask"/> is invalid. </para>
///     <para>EBADF: <paramref name="fd"/> is not a valid file descriptor.</para>
///     <para>EEXIST: The supplied file descriptor is already registered with this EventLoop</para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same behavior
///     will be retained through system updates.</para>
/// </summary>
/// <param name="el"> The EventLoop on which to register the I/O event. </param>
/// <param name="fd"> The descriptor associated with the event. </param>
/// <param name="eventBitmask"> Bitmask of the events to monitor. </param>
/// <param name="callback"> Pointer to a <see cref="EventLoopIoCallback"/> to call when any of the
/// specified events occur. </param>
/// <param name="context"> Optional context to pass to the <paramref name="callback"/> function.
/// </param>
/// <returns>
///     Pointer to an EventRegistration on success, or NULL for failure, in which case errno will be
///     set to the error value.
/// </returns>
/// <remarks>
///     If the function succeeds, it returns a pointer to an EventRegistration that tracks the
///     registration. The registration remains active until the application calls <see
///     cref="EventLoop_UnregisterIo"/> with the pointer or closes the EventLoop with <see
///     cref="EventLoop_Close"/>.
///
///     A file descriptor can have only one active registration per EventLoop at a time. Trying to
///     register a currently registered descriptor will result in an EEXIST error.
///
///     The file descriptor must stay open while registered. Closing a descriptor that has an active
///     registration will lead to undefined behavior.
/// </remarks>
EventRegistration *EventLoop_RegisterIo(EventLoop *el, int fd, EventLoop_IoEvents eventBitmask,
                                        EventLoopIoCallback *callback, void *context);

/// <summary>
///     Modifies the I/O events of an EventLoop registration.
///
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: <paramref name="el"/> and/or <paramref name="reg"/> is NULL. </para>
///     <para>EINVAL: <paramref name="eventBitmask"/> is invalid. </para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same behavior
///     will be retained through system updates.</para>
/// </summary>
/// <param name="el"> The EventLoop on which to modify the registration. </param>
/// <param name="reg"> The EventRegistration returned by <see cref="EventLoop_RegisterIo"/>.
/// </param> <param name="eventBitmask"> Bitmask of the events to monitor. </param> <returns>
///     0 for success, or -1 for failure, in which case errno will be set to the error value.
/// </returns>
/// <remarks>
///     The EventRegistration must have been returned by <see cref="EventLoop_RegisterIo"/> for the
///     same EventLoop object. Trying to modify a EventRegistration with a different EventLoop or
///     another object will lead to undefined behavior.
/// </remarks>
int EventLoop_ModifyIoEvents(EventLoop *el, EventRegistration *reg,
                             EventLoop_IoEvents eventBitmask);

/// <summary>
///     Unregisters an I/O event with the EventLoop.
///
///     <para> **Errors** </para>
///     If an error is encountered, returns -1 and sets errno to the error value.
///     <para>EFAULT: <paramref name="el"/> is NULL and <paramref name="reg"/> is not NULL. </para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same behavior
///     will be retained through system updates.</para>
/// </summary>
/// <param name="el"> The EventLoop on which to unregister the I/O event. </param>
/// <param name="reg"> The EventRegistration returned by <see cref="EventLoop_RegisterIo"/>.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno will be set to the error value.
/// </returns>
/// <remarks>
///     If <paramref name="reg"/> is NULL, no action occurs.
///
///     If <paramref name="reg"/> is not NULL, the EventRegistration must have been returned by <see
///     cref="EventLoop_RegisterIo"/> for the same EventLoop object. Trying to unregister a
///     EventRegistration with a different EventLoop or another object will lead to undefined
///     behavior.
///
///     An active EventRegistration can be unregistered once. Trying to unregister it multiple times
///     will lead to undefined behavior.
/// </remarks>
int EventLoop_UnregisterIo(EventLoop *el, EventRegistration *reg);

#ifdef __cplusplus
}
#endif
