/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
   
#include "event_queue.h"
#include <applibs/log.h>
#include <math.h>
#include <time.h>

// Structures
//-----------
struct EventQueue::Event
{
    SQObject    callback;       ///< A ref-counted Squirrel callback to trigger upon the event.
    Event*      next;           ///< The next event in ascending time-sorted linked list.
    time_t      wakeupTime_s;   ///< The absolute wakeup time of the event in seconds.
    long        wakeupTime_ns;  ///< The subsecond component of the absolute wakeup time in nanoseconds.

    Event(SQObject callback, time_t wakeupTime_s, long wakeupTime_ns) : callback(callback), next(nullptr), wakeupTime_s(wakeupTime_s), wakeupTime_ns(wakeupTime_ns) {}
    Event(SQObject callback) : callback(callback), next(nullptr), wakeupTime_s(0), wakeupTime_ns(0) {}
    Event() : callback(SQObject {OT_NULL, nullptr}), next(nullptr), wakeupTime_s(0), wakeupTime_ns(0) {}
};

// Static Methods
//---------------
/// Registers the EventQueue class with Squirrel as a global (stored in the root table) singleton.
/// \param vm the instance of the VM to use.
/// \param name the name by which the class will be accessible from within Squirrel.
/// \returns a pointer to the singleton instance instantiated and placed within the root table.
EventQueue* EventQueue::registerWithSquirrelAsGlobal(HSQUIRRELVM vm, const char* name)
{
    SquirrelCppHelper::DelegateFunction delegateFunctions[1];
    delegateFunctions[0] = SquirrelCppHelper::DelegateFunction("wakeup", &EventQueue::SQUIRREL_METHOD_NAME(wakeup));

    return SquirrelCppHelper::registerClassAsGlobal<EventQueue>(vm, name, delegateFunctions, 1);
}

// Squirrel Methods
//-----------------
/// Queues an event/callback to be triggered after the specified time.
/// \param interval_s The length of time in seconds to elapse before the timer fires (may be integer of float).
/// \param callback The function to call when the timer fires. The function is called with no parameters.
SQUIRREL_METHOD_IMPL(EventQueue, wakeup)
{
    int types[] = {OT_INTEGER|OT_FLOAT, OT_CLOSURE};
    if(SQ_FAILED(SquirrelCppHelper::checkParameterTypes(vm, 2, 2, types)))
    {
        return SQ_ERROR;
    }

    // Capture the current system time
    struct timespec currentSystemTime;
    clock_gettime(CLOCK_MONOTONIC_RAW, &currentSystemTime);

    // Retrieve the wakeup time as a float
    SQObject transientObject;
    sq_getstackobj(vm, 2, &transientObject);
    SQFloat wakeupTime_s = sq_objtofloat(&transientObject);

    // Retrieve the callback closure and take a reference
    SQObject callback;
    sq_resetobject(&callback);
    sq_getstackobj(vm, 3, &callback);
    sq_addref(vm, &callback);

    // Calculate the absolute time and store in a new event
    float seconds;
    unsigned int nanoseconds = (unsigned int)(modff(wakeupTime_s, &seconds) * 100000000L);
    currentSystemTime.tv_sec += (unsigned int)seconds;
    currentSystemTime.tv_nsec += nanoseconds;
    currentSystemTime.tv_sec += nanoseconds / 1000000000L;
    currentSystemTime.tv_nsec = nanoseconds % 1000000000L;

    Event *event = (Event*)sq_malloc(sizeof(Event));
    new(event) Event(callback, currentSystemTime.tv_sec, currentSystemTime.tv_nsec);

    // Search the event queue to find the appropriate location to store the event
    Event *currentEvent = events;

    do
    {
        Event *nextEvent = currentEvent->next;

        if(nextEvent == nullptr)
        {
            currentEvent->next = event;
            break;
        }
        else if((currentSystemTime.tv_sec == nextEvent->wakeupTime_s && currentSystemTime.tv_nsec < nextEvent->wakeupTime_ns) ||
            currentSystemTime.tv_sec < nextEvent->wakeupTime_s)
        {
            event->next = nextEvent;
            currentEvent->next = event;
            break;
        }

        currentEvent = currentEvent->next;
    } while(currentEvent != nullptr);

    return 0;
}

// Methods
//--------
/// Processes the EventQueue, triggering and clearing any due events.
/// Should be called from a loop.
/// \param vm the instance of the VM to use.
/// \returns 'true' if an event was processed/triggers, otherwise 'false'.
/// \todo Need to support sleep, where system clock will stop working
bool EventQueue::process(HSQUIRRELVM vm)
{
    // Capture the current system time and build-in a 10ms buffer (Note: not handling overflow, is only for general optimisation)
    struct timespec currentSystemTime;
    clock_gettime(CLOCK_MONOTONIC_RAW, &currentSystemTime);
    currentSystemTime.tv_nsec += 10000000;

    Event *event = events->next;
    
    if( event != nullptr &&
       (event->wakeupTime_s < currentSystemTime.tv_sec ||
       (event->wakeupTime_s == currentSystemTime.tv_sec && event->wakeupTime_ns < currentSystemTime.tv_nsec)))
    {
        SQObject *callback = &event->callback; 
        sq_pushobject(vm, *callback);
        sq_pushroottable(vm);
        if(SQ_FAILED(sq_call(vm,1,false,true)))
        {
            Log_Debug("Execution of callback failed.\n");
        }
        sq_poptop(vm);
        events->next = event->next;
        sq_release(vm, callback);
        free(event);
        return true;
    }

    return false;
}

// Constructor
//------------
EventQueue::EventQueue()
{
    events = (Event*)sq_malloc(sizeof(Event));
    new(events) Event();
}

// Destructor
//-----------
EventQueue::~EventQueue()
{
    sq_free(events, sizeof(Event));
}