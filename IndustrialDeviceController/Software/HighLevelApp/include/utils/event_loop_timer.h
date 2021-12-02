/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <time.h>
#include <applibs/eventloop.h>

typedef struct event_loop_timer_t event_loop_timer_t;

typedef void (*event_loop_timer_callback_t)(void *context);

/**
 * register a timer to current event loop, where callback will be invoked when timer expired
 * @param eloop EventLoop instance
 * @param init  initial expiration of the timer in seconds and nanoseconds, pass NULL or set both value to zero disarm timer 
 * @param interval repeat expiration in seconds and nanoseconds, pass NULL or set both to zero will expire just once as specified in init
 * @param callback - callback been invoked on timer expiration
 * @param context - context to be used when invoke callback
 * @return timer instance, resource must be released with event_loop_unregister_timer()
 */
event_loop_timer_t *event_loop_register_timer(EventLoop *eloop, const struct timespec *init, const struct timespec *interval,
                                              event_loop_timer_callback_t callback, void *context);


/**
 * unregister timer from event loop, pending timer will be cancelled
 * @param eloop event loop instance
 * @param timer timer to be unregistered
 */
void event_loop_unregister_timer(EventLoop *eloop, event_loop_timer_t *timer);

/**
 * set expiration value for given timer
 * @param timer timer to be set
 * @param init init expiration of timer in seconds and nanoseconds, pass NULL or set both value to zero disarm timer
 * @param interval repeat expiration in seconds and nanoseconds, pass NULL or set both to zero will expire just once as specified in init
 * @return 0 on succeed or -1 on failure
 */
int event_loop_set_timer(event_loop_timer_t *timer, const struct timespec *init, const struct timespec *interval);

/**
 * set expiration value and context for given timer
 * @param timer timer to be set
 * @param init init expiration of timer in seconds and nanoseconds, pass NULL or set both value to zero disarm timer
 * @param interval repeat expiration in seconds and nanoseconds, pass NULL or set both to zero will expire just once as specified in init
 * @param context context to be updated, all callback invoked on expiration will use new context value specified
 * @return 0 on succeed or -1 on failed
 */
int event_loop_set_timer_and_context(event_loop_timer_t *timer, const struct timespec *init, const struct timespec *interval,
                                    void *context);

/**
 * cancel given timer
 * @param timer timer to be cancelled
 * @return 0 on succed or -1 on failed
 */
int event_loop_cancel_timer(event_loop_timer_t *timer);