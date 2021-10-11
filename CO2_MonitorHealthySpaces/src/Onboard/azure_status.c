/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "azure_status.h"

/// <summary>
/// Handler to turn off LEDs
/// </summary>
/// <param name="eventLoopTimer"></param>
void azure_status_led_off_handler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }
    dx_gpioOff(&gpio_network_led);
}

/// <summary>
/// Flash LEDs timer handler
/// </summary>
void azure_status_led_on_handler(EventLoopTimer *eventLoopTimer)
{
    static int init_sequence = 25;

    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    if (init_sequence-- > 0)
    {
        dx_gpioOn(&gpio_network_led);
        // on for 100ms off for 100ms = 200 ms in total
        dx_timerOneShotSet(&tmr_azure_status_led_on, &(struct timespec){0, 200 * ONE_MS});
        dx_timerOneShotSet(&tmr_azure_status_led_off, &(struct timespec){0, 100 * ONE_MS});
    }
    else if (azure_connected)
    {
        dx_gpioOn(&gpio_network_led);
        // off for 3900ms, on for 100ms = 4000ms in total
        dx_timerOneShotSet(&tmr_azure_status_led_on, &(struct timespec){4, 0});
        dx_timerOneShotSet(&tmr_azure_status_led_off, &(struct timespec){0, 100 * ONE_MS});
    }
    else
    {
        dx_gpioOn(&gpio_network_led);
        // on for 100ms off for 1300ms = 1400 ms in total
        dx_timerOneShotSet(&tmr_azure_status_led_on, &(struct timespec){1, 400 * ONE_MS});
        dx_timerOneShotSet(&tmr_azure_status_led_off, &(struct timespec){0, 700 * ONE_MS});
    }
}