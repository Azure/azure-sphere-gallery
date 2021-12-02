/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include <init/globals.h>
#include <iot/diag.h>

static const struct itimerspec watchdog_interval = {{0, 0}, {WATCHDOG_WARNING_SEC, 0}};

static struct watchdog_t {
    timer_t timer;
    volatile int warning;
} s_watchdog;

static void watchdog_handler(int sig, siginfo_t *si, void *uc)
{
    if (s_watchdog.warning == WATCHDOG_WARNING_TIMES) {
        // warning too many times, time to give up.
        diag_log_event_to_file(EVENT_WATCHDOG);
        fprintf(stderr, "WATCHDOG!!!\n");
        exit(-2);
    } else {
        s_watchdog.warning++;
        diag_log_event(EVENT_WATCHDOG_WARNING);
        fprintf(stderr, "SYSYTEM IS SLOW!!!\n");
        timer_settime(s_watchdog.timer, 0, &watchdog_interval, NULL);
    }
}

void watchdog_init(void)
{
    struct sigaction sa;

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = watchdog_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);

    struct sigevent alarm_event;
    alarm_event.sigev_notify = SIGEV_SIGNAL;
    alarm_event.sigev_signo = SIGALRM;
    alarm_event.sigev_value.sival_ptr = &s_watchdog.timer;

    timer_create(CLOCK_MONOTONIC, &alarm_event, &s_watchdog.timer);
    timer_settime(s_watchdog.timer, 0, &watchdog_interval, NULL);
}

// Must be called periodically
void watchdog_kick(void)
{
    // check that application is operating normally
    // if so, reset the watchdog
    s_watchdog.warning = 0;
    timer_settime(s_watchdog.timer, 0, &watchdog_interval, NULL);
}