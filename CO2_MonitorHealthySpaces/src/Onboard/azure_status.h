/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_azure_iot.h"
#include "dx_gpio.h"
#include "dx_timer.h"
#include "dx_utilities.h"

extern DX_GPIO_BINDING gpio_network_led;
extern DX_TIMER_BINDING tmr_azure_status_led_off;
extern DX_TIMER_BINDING tmr_azure_status_led_on;
extern bool azure_connected;