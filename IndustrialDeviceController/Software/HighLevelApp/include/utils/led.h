/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
  
#pragma once

#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <applibs/gpio.h>

#define NETWORK_LED 1
#define APP_LED  0


typedef enum {
    LED_OFF = 0,     // 000 binary
    LED_RED = 1,     // 001 binary
    LED_GREEN = 2,   // 010 binary
    LED_YELLOW = 3,  // 011 binary
    LED_BLUE = 4,    // 100 binary
    LED_MAGENTA = 5, // 101 binary
    LED_CYAN = 6,    // 110 binary
    LED_WHITE = 7,   // 111 binary
    LED_UNKNOWN = 8  // 1000 binary
} led_color_t;


/**
 * initialized led module
 * @return 0 if succeed, negative otherwise
 */
int led_init(void);

/**
 * deinitialize led module
 */
void led_deinit(void);

/**
 * set led color
 * @param led which led
 * @param color which color, set color to LED_OFF will turn off led
 * @return 0 if succeed, negative otherwise
 */
int led_set_color(int led, led_color_t color);
