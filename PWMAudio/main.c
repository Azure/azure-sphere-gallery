/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "lib/CPUFreq.h"
#include "lib/VectorTable.h"
#include "lib/NVIC.h"
#include "lib/GPIO.h"
#include "lib/mt3620/gpio.h"
#include "lib/GPT.h"
#include "lib/UART.h"
#include "lib/Print.h"
#include "lib/GPT.h"

#define USE_2M_SOURCE
// #define USE_32K_SOURCE

const uint32_t notes_hz[8] = {
    262, // c
    294, // d
    330, // e
    350, // f
    392, // g
    440, // a
    494, // b
    523  // c
};

void set_pwm0(int frequency_hz) {
#if defined(USE_32K_SOURCE)
    uint32_t time_base = 1000000;
    uint32_t base_frequency = MT3620_PWM_32k;
    uint32_t freq_multiplier = 1;
#elif defined(USE_2M_SOURCE)
    uint32_t time_base = 1000000000;
    uint32_t base_frequency = MT3620_PWM_2M;
#else
    #error "Please define either: USE_32K_SOURCE or USE_2M_SOURCE"
#endif

    // desired frequency in microseconds (for 32 KHz clock; nanoseconds for 2M clock)
    uint32_t freq_us = (time_base / frequency_hz) / 2;
    uint32_t tick_value = freq_us / (time_base / base_frequency);

    PWM_ConfigurePin(0, base_frequency, tick_value, tick_value);
}

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(26000000);

    GPT *timer = GPT_Open(MT3620_UNIT_GPT1, 32768, GPT_MODE_REPEAT);

    while(1) {
        for (int i = 0; i < 8; i++) {
            set_pwm0(notes_hz[i]);
            GPT_WaitTimer_Blocking(timer, 500, GPT_UNITS_MILLISEC);
        }

        for (int i = 7; i >= 0; i--) {
            set_pwm0(notes_hz[i]);
            GPT_WaitTimer_Blocking(timer, 500, GPT_UNITS_MILLISEC);
        }
    }
}