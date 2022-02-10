/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <sys/timerfd.h>
#include <unistd.h>

#include <init/globals.h>
#include <utils/event_loop_timer.h>

struct event_loop_timer_t {
    EventRegistration *reg;
    int fd;
    void *context;
    event_loop_timer_callback_t callback;
};

static void timer_callback(EventLoop *eloop, int fd, EventLoop_IoEvents events, void *context)
{
    ASSERT(context);
    event_loop_timer_t *timer = (event_loop_timer_t *)context;

    // consume timer event now to avoid trigger again
    uint64_t timer_data = 0;
    if (read(timer->fd, &timer_data, sizeof(timer_data)) == -1) {
        return;
    }

    timer->callback(timer->context);
}


event_loop_timer_t* event_loop_register_timer(EventLoop *eloop, const struct timespec *init, const struct timespec *interval,
                            event_loop_timer_callback_t callback, void *context)
{
    ASSERT(eloop);
    ASSERT(callback);

    event_loop_timer_t *timer = MALLOC(sizeof(event_loop_timer_t));
    timer->callback = callback;
    timer->context = context;

    if ((timer->fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)) < 0) {
        FREE(timer);
        return NULL;
    }

    if (event_loop_set_timer(timer, init, interval) < 0) {
        FREE(timer);
        return NULL;
    }

    if ((timer->reg = EventLoop_RegisterIo(eloop, timer->fd, EventLoop_Input, timer_callback, timer)) == NULL) {
        FREE(timer);
        return NULL;
    }

    return timer;
}


void event_loop_unregister_timer(EventLoop *eloop, event_loop_timer_t *timer)
{
    ASSERT(eloop);
    ASSERT(timer);
    EventLoop_UnregisterIo(eloop, timer->reg);
    close(timer->fd);
    FREE(timer);
}


int event_loop_set_timer(event_loop_timer_t *timer, const struct timespec *init, const struct timespec *interval)
{
    ASSERT(timer);

    struct itimerspec its = {.it_value = {0, 0}, .it_interval={0, 0}};

    if (init) {
        its.it_value = *init;
    }

    if (interval) {
        its.it_interval = *interval;
    }

    return timerfd_settime(timer->fd, 0, &its, NULL);
}

int event_loop_set_timer_and_context(event_loop_timer_t *timer, const struct timespec *init, const struct timespec *interval,
                                    void *context)
{
    timer->context = context;
    return event_loop_set_timer(timer, init, interval);
}


int event_loop_cancel_timer(event_loop_timer_t *timer)
{
    ASSERT(timer);
    return event_loop_set_timer(timer, NULL, NULL);
}

