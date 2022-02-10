/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <applibs/log.h>
#include <applibs/eventloop.h>

#include <init/globals.h>
#include <init/adapter.h>
#include <init/applibs_versions.h>
#include <init/device_hal.h>
#include <iot/azure_iot_utilities.h>
#include <iot/diag.h>
#include <iot/iot.h>
#include <utils/led.h>
#include <utils/llog.h>
#include <utils/network.h>
#include <utils/timer.h>
#include <utils/utils.h>
#include <utils/memory.h>
#include <safeclib/safe_lib.h>

#include "watchdog.h"

static EventLoop *s_eloop = NULL;

volatile bool g_app_running = true;

static volatile bool s_signal_term = false;

static void termination_handler(int signal_number)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async signal safe
    s_signal_term = true;
    g_app_running = false;
}

static void register_sigterm_handler(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = termination_handler;
    sigaction(SIGTERM, &action, NULL);
}

static int sphere_init(void)
{
    register_sigterm_handler();

    led_init();

    watchdog_init();

    s_eloop = EventLoop_Create();

    if (llog_init() != 0) {
        return -1;
    }

#ifdef ENABLE_SERIAL_LOG
    llog_config(LOG_ENDPOINT_SERIAL, LOG_VERBOSE);
#endif

    if (network_init() != 0) {
        LOGE("Failed to init wifi");
        return -1;
    }

    if (iot_init(s_eloop) != 0) {
        LOGE("Failed to init Azure Iot");
        return -1;
    }

    if (diag_init(s_eloop) != 0) {
        LOGE("Failed to init diag");
        return -1;
    }

    if (adapter_init(s_eloop) != 0) {
        LOGE("Failed to init adapter");
        return -1;
    }

    return 0;
}

static void sphere_deinit(void)
{
    adapter_deinit();
    iot_deinit();
    diag_deinit();
    network_deinit();
    llog_deinit();
    EventLoop_Close(s_eloop);
    led_deinit();
}


int app_main(int argc, char *argv[])
{
    if (argc > 1) {
        set_app_version(argv[1]);
    }

    printf("Sphere application (%s) starting, epoch=%ld\n", app_version(), time(NULL));

    if (sphere_init() != 0) {
        LOGE("fail init");
        return -1;
    }

    while (g_app_running) {
         if ((EventLoop_Run(s_eloop, -1, true) == EventLoop_Run_Failed) && errno != EINTR) {
            LOGE("event loop error, quit");
            break;
        }
        watchdog_kick();
    }

    if (s_signal_term) {
        LOGI("Program exit with SIGTERM");
        diag_log_event_to_file(EVENT_SIGTERM);
    }

    sphere_deinit();
    MEMORY_REPORT(1);
    printf("Sphere application exiting\n");

    return 0;
}

#ifndef TEST
int main(int argc, char *argv[])
{
    return app_main(argc, argv);
}
#endif