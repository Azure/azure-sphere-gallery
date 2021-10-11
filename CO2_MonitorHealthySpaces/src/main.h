/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_deferred_update.h"
#include "dx_gpio.h"
#include "dx_json_serializer.h"
#include "dx_pwm.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "dx_version.h"

#include "hw/co2_monitor.h" // Hardware definition
#include "app_exit_codes.h" // application specific exit codes
#include "Onboard/onboard_sensors.h"
#include "Onboard/azure_status.h"

#include <applibs/applications.h>
#include <applibs/log.h>
#include <applibs/powermanagement.h>

#include "AzureSphereDrivers/EmbeddedScd30/scd30/scd30.h"

// https://docs.microsoft.com/en-us/azure/iot-pnp/overview-iot-plug-and-play
#define IOT_PLUG_AND_PLAY_MODEL_ID "dtmi:com:example:azuresphere:co2monitor;1"
#define NETWORK_INTERFACE "wlan0"
#define CO2_MONITOR_FIRMWARE_VERSION "3.02"

/***********************************************************************************************************
 * Forward declarations
 **********************************************************************************************************/

static void co2_alert_buzzer_off_handler(EventLoopTimer *eventLoopTimer);
static void co2_alert_handler(EventLoopTimer *eventLoopTimer);
static void DeviceTwinGenericHandler(DX_DEVICE_TWIN_BINDING *deviceTwinBinding);
static void publish_telemetry_handler(EventLoopTimer *eventLoopTimer);
static void read_buttons_handler(EventLoopTimer *eventLoopTimer);
static void read_telemetry_handler(EventLoopTimer *eventLoopTimer);
static void update_device_twins(EventLoopTimer *eventLoopTimer);
static void watchdog_handler(EventLoopTimer *eventLoopTimer);
void azure_status_led_off_handler(EventLoopTimer *eventLoopTimer);
void azure_status_led_on_handler(EventLoopTimer *eventLoopTimer);

/***********************************************************************************************************
 * Declare global variables
 **********************************************************************************************************/

#define Log_Debug(f_, ...) dx_Log_Debug((f_), ##__VA_ARGS__)
static char Log_Debug_Time_buffer[128];

#define JSON_MESSAGE_BYTES 256
static char msgBuffer[JSON_MESSAGE_BYTES] = {0};

// Set alert level to a reasonable default. This is updated by CO2PPMAlertLevel device twin
static int32_t co2_alert_level = 1000;
static float co2_ppm, temperature, relative_humidity;
static bool be_quiet = false;

ENVIRONMENT telemetry;

static DX_USER_CONFIG dx_config;
bool azure_connected = false;

static const struct itimerspec watchdogInterval = {{60, 0}, {60, 0}};
static timer_t watchdogTimer;

/***********************************************************************************************************
 * Common content properties for publish messages to IoT Hub/Central
 **********************************************************************************************************/

/// <summary>
/// Publish sensor telemetry using the following properties for efficient IoT Hub routing
/// https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messages-d2c
/// </summary>
static DX_MESSAGE_PROPERTY *messageProperties[] = {&(DX_MESSAGE_PROPERTY){.key = "appid", .value = "co2monitor"},
                                                   &(DX_MESSAGE_PROPERTY){.key = "type", .value = "telemetry"},
                                                   &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

static DX_MESSAGE_CONTENT_PROPERTIES contentProperties = {.contentEncoding = "utf-8", .contentType = "application/json"};

/***********************************************************************************************************
 * declare device twin bindings
 **********************************************************************************************************/

static DX_DEVICE_TWIN_BINDING dt_co2_ppm_alert_level = {
    .propertyName = "AlertLevel", .twinType = DX_DEVICE_TWIN_INT, .handler = DeviceTwinGenericHandler};

static DX_DEVICE_TWIN_BINDING dt_humidity = {.propertyName = "Humidity", .twinType = DX_DEVICE_TWIN_INT};
static DX_DEVICE_TWIN_BINDING dt_pressure = {.propertyName = "Pressure", .twinType = DX_DEVICE_TWIN_INT};
static DX_DEVICE_TWIN_BINDING dt_temperature = {.propertyName = "Temperature", .twinType = DX_DEVICE_TWIN_INT};
static DX_DEVICE_TWIN_BINDING dt_carbon_dioxide = {.propertyName = "CarbonDioxide", .twinType = DX_DEVICE_TWIN_INT};

static DX_DEVICE_TWIN_BINDING dt_startup_utc = {.propertyName = "StartupUtc", .twinType = DX_DEVICE_TWIN_STRING};
static DX_DEVICE_TWIN_BINDING dt_sw_version = {.propertyName = "SoftwareVersion", .twinType = DX_DEVICE_TWIN_STRING};

static DX_DEVICE_TWIN_BINDING dt_defer_requested = {.propertyName = "DeferredUpdateRequest", .twinType = DX_DEVICE_TWIN_STRING};

/***********************************************************************************************************
 * declare gpio bindings
 **********************************************************************************************************/

DX_GPIO_BINDING gpio_network_led = {.pin = AZURE_CONNECTED_LED, .name = "network_led", .direction = DX_OUTPUT, .initialState = GPIO_Value_Low, .invertPin = true};
static DX_GPIO_BINDING gpio_button_b = {.pin = BUTTON_B, .name = "button_b", .direction = DX_INPUT, .detect = DX_GPIO_DETECT_LOW};

/***********************************************************************************************************
 * declare timer bindings
 **********************************************************************************************************/

DX_TIMER_BINDING tmr_azure_status_led_off = {.name = "tmr_azure_status_led_off", .handler = azure_status_led_off_handler};
DX_TIMER_BINDING tmr_azure_status_led_on = {.period = {0, 500 * ONE_MS}, .name = "tmr_azure_status_led_on", .handler = azure_status_led_on_handler};
static DX_TIMER_BINDING tmr_co2_alert_buzzer_off_oneshot = {.name = "tmr_co2_alert_buzzer_off_oneshot",
                                                            .handler = co2_alert_buzzer_off_handler};
static DX_TIMER_BINDING tmr_co2_alert_timer = {.period = {8, 0}, .name = "tmr_co2_alert_timer", .handler = co2_alert_handler};
static DX_TIMER_BINDING tmr_publish_telemetry = {.period = {5, 0}, .name = "tmr_publish_telemetry", .handler = publish_telemetry_handler};
static DX_TIMER_BINDING tmr_read_buttons = {.period = {0, 100 * ONE_MS}, .name = "tmr_read_buttons", .handler = read_buttons_handler};
static DX_TIMER_BINDING tmr_read_telemetry = {.name = "tmr_read_telemetry", .handler = read_telemetry_handler};
static DX_TIMER_BINDING tmr_update_device_twins = {.period = {10, 0}, .name = "tmr_update_device_twins", .handler = update_device_twins};
static DX_TIMER_BINDING tmr_watchdog = {.period = {30, 0}, .name = "tmr_publish_telemetry", .handler = watchdog_handler};

/***********************************************************************************************************
 * declare pwm bindings
 **********************************************************************************************************/

static DX_PWM_CONTROLLER pwm_click_controller = {.controllerId = PWM_CLICK_CONTROLLER, .name = "PWM Click Controller"};
static DX_PWM_CONTROLLER pwm_rgb_controller = {.controllerId = PWM_RGB_CONTROLLER, .name = "PWM RBG Controller"};

static DX_PWM_BINDING pwm_buzz_click = {.pwmController = &pwm_click_controller, .channelId = 0, .name = "click 1 buzz"};
static DX_PWM_BINDING pwm_led_red = {.pwmController = &pwm_rgb_controller, .channelId = 0, .name = "pwm red led"};
static DX_PWM_BINDING pwm_led_green = {.pwmController = &pwm_rgb_controller, .channelId = 1, .name = "pwm green led"};
static DX_PWM_BINDING pwm_led_blue = {.pwmController = &pwm_rgb_controller, .channelId = 2, .name = "pwm green led"};

/***********************************************************************************************************
 * declare binding sets
 **********************************************************************************************************/

// All bindings referenced in the following binding sets are initialised in the InitPeripheralsAndHandlers function
static DX_DEVICE_TWIN_BINDING *device_twin_bindings[] = {&dt_co2_ppm_alert_level, &dt_startup_utc,    &dt_sw_version,
                                                  &dt_temperature,         &dt_pressure,       &dt_humidity,
                                                  &dt_carbon_dioxide,      &dt_defer_requested};

static DX_PWM_BINDING *pwm_bindings[] = {&pwm_buzz_click, &pwm_led_green, &pwm_led_red, &pwm_led_blue};

static DX_GPIO_BINDING *gpio_bindings[] = {&gpio_network_led, &gpio_button_b};

static DX_TIMER_BINDING *timer_bindings[] = {&tmr_read_telemetry,
                                      &tmr_co2_alert_buzzer_off_oneshot,
                                      &tmr_co2_alert_timer,
                                      &tmr_azure_status_led_on,
                                      &tmr_azure_status_led_off,
                                      &tmr_publish_telemetry,
                                      &tmr_update_device_twins,
                                      &tmr_read_buttons,
                                      &tmr_watchdog};
