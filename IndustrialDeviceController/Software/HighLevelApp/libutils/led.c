/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <init/applibs_versions.h>

#include <applibs/gpio.h>
#include <hw/board_config.h>
#include <applibs/networking.h>

#include <init/adapter.h>
#include <utils/led.h>
#include <utils/llog.h>
#include <iot/iot.h>

#ifdef MT3620_RDB
#define GPIO_VALUE_LED_ON GPIO_Value_Low
#define GPIO_VALUE_LED_OFF GPIO_Value_High
#else
#define GPIO_VALUE_LED_ON GPIO_Value_High
#define GPIO_VALUE_LED_OFF GPIO_Value_Low
#endif

#define NUM_CHANNEL 2
#define NUM_LED 2

static const GPIO_Id LED_PINS[NUM_LED][NUM_CHANNEL] = {{BOARD_LED1_RED, BOARD_LED1_GREEN},
                                                       {BOARD_LED2_RED, BOARD_LED2_GREEN}};

static int s_leds[NUM_LED][NUM_CHANNEL] = {{-1, -1},
                                           {-1, -1}};


static int led_open_fd(int led)
{
    if (led >= NUM_LED) return -1;

    for (int c = 0; c < NUM_CHANNEL; c++) {

        if ((s_leds[led][c] = GPIO_OpenAsOutput(LED_PINS[led][c], GPIO_OutputMode_PushPull, GPIO_VALUE_LED_OFF)) < 0) {
            return -1;
        }
    }

    return 0;
}


static void led_close_fd(int led)
{
    if (led >= NUM_LED) return;

    for (int c = 0; c < NUM_CHANNEL; c++) {

        if (s_leds[led][c] >= 0) {
            GPIO_SetValue(s_leds[led][c], GPIO_VALUE_LED_OFF);
            close(s_leds[led][c]);
            s_leds[led][c] = -1;
        }
    }
}


// ---------------------------- public interface ------------------------------

int led_init(void)
{
    const struct timespec FLASH_DELAY = {0, 200 * 1e6};

    led_open_fd(NETWORK_LED);
    led_open_fd(APP_LED);

    for (int i=0; i<5; i++) {
        led_set_color(NETWORK_LED, LED_GREEN);
        led_set_color(APP_LED, LED_GREEN);
        nanosleep(&FLASH_DELAY, NULL);
        led_set_color(NETWORK_LED, LED_YELLOW);
        led_set_color(APP_LED, LED_YELLOW);        
        nanosleep(&FLASH_DELAY, NULL);
        led_set_color(NETWORK_LED, LED_RED);
        led_set_color(APP_LED, LED_RED);
        nanosleep(&FLASH_DELAY, NULL);  
    }

    return 0;
}

void led_deinit()
{
    led_close_fd(NETWORK_LED);
    led_close_fd(APP_LED);    
}

int led_set_color(int led, led_color_t color)
{
    for (int c = 0; c < NUM_CHANNEL; c++) {

        if (s_leds[led][c] < 0) return -1;

        bool on = color & (0x1 << c);

        if (GPIO_SetValue(s_leds[led][c], on ? GPIO_VALUE_LED_ON : GPIO_VALUE_LED_OFF) != 0) {
            return -1;
        }
    }

    return 0;
}

