/* Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 *
 * This example is built on the Azure Sphere DevX library.
 *   1. DevX is an Open Source community-maintained implementation of the Azure Sphere SDK samples.
 *   2. DevX is a modular library that simplifies common development scenarios.
 *        - You can focus on your solution, not the plumbing.
 *   3. DevX documentation is maintained at https://github.com/Azure-Sphere-DevX/AzureSphereDevX.Examples/wiki
 *
 * DEVELOPER BOARD SELECTION
 *
 * The following developer boards are supported.
 *
 *	 1. AVNET Azure Sphere Starter Kit Revision 1.
 *   2. AVNET Azure Sphere Starter Kit Revision 2.
 *
 * ENABLE YOUR DEVELOPER BOARD
 *
 * Each Azure Sphere developer board manufacturer maps pins differently. You need to select the
 *    configuration that matches your board.
 *
 * Follow these steps:
 *
 *	 1. Open azsphere_board.txt
 *	 2. Uncomment the set command that matches your developer board.
 *	 3. Visual Studio, save to auto-generate the CMake Cache.
 *   4. Visual Studio Code, ctrl+shift+p, type and select CMake: configure
 *
 ************************************************************************************************/

#include "main.h"

/***********************************************************************************************************
 * CO2, temperature, humidity, and pressure sensor data
 *
 * Read SDC30 CO2 sensor and Avnet onboard sensors
 * Publish data to Azure IoT Hub/Central
 **********************************************************************************************************/

/// <summary>
/// Publish HVAC telemetry
/// </summary>
/// <param name="eventLoopTimer"></param>
static void publish_telemetry_handler(EventLoopTimer *eventLoopTimer)
{
    static int msgId = 0;

    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    if (telemetry.valid && azure_connected)
    {
        // clang-format off
        // Serialize telemetry as JSON
        if (dx_jsonSerialize(msgBuffer, sizeof(msgBuffer), 7,
            DX_JSON_INT, "msgId", msgId++,
            DX_JSON_INT, "co2ppm", telemetry.latest.co2ppm,
            DX_JSON_INT, "humidity", telemetry.latest.humidity,
            DX_JSON_INT, "pressure", telemetry.latest.pressure,
            DX_JSON_INT, "temperature", telemetry.latest.temperature,
            DX_JSON_INT, "peakUserMemoryKiB", (int)Applications_GetPeakUserModeMemoryUsageInKB(),
            DX_JSON_INT, "totalMemoryKiB", (int)Applications_GetTotalMemoryUsageInKB()))
        // clang-format on
        {
            dx_Log_Debug("%s\n", msgBuffer);

            // Publish telemetry message to IoT Hub/Central
            dx_azurePublish(msgBuffer, strlen(msgBuffer), messageProperties, NELEMS(messageProperties), &contentProperties);
        }
        else
        {
            dx_Log_Debug("JSON Serialization failed: Buffer too small\n");
            dx_terminate(APP_ExitCode_Telemetry_Buffer_Too_Small);
        }
    }
}

/// <summary>
/// Determine if telemetry value changed. If so, update it's device twin
/// </summary>
/// <param name="new_value"></param>
/// <param name="previous_value"></param>
/// <param name="device_twin"></param>
static void device_twin_update(int *latest_value, int *previous_value, DX_DEVICE_TWIN_BINDING *device_twin)
{
    if (*latest_value != *previous_value)
    {
        *previous_value = *latest_value;
        dx_deviceTwinReportValue(device_twin, latest_value);
    }
}

/// <summary>
/// Only update device twins if data changed to minimize network and cloud costs
/// </summary>
/// <param name="temperature"></param>
/// <param name="pressure"></param>
static void update_device_twins(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    if (telemetry.valid && azure_connected)
    {
        device_twin_update(&telemetry.latest.temperature, &telemetry.previous.temperature, &dt_temperature);
        device_twin_update(&telemetry.latest.pressure, &telemetry.previous.pressure, &dt_pressure);
        device_twin_update(&telemetry.latest.humidity, &telemetry.previous.humidity, &dt_humidity);
        device_twin_update(&telemetry.latest.co2ppm, &telemetry.previous.co2ppm, &dt_carbon_dioxide);
    }
}

/// <summary>
/// Handler called every 20 seconds
/// The CO2 sensor and Avnet Onboard sensors are read
/// Then reload 20 second oneshot timer
/// </summary>
/// <param name="eventLoopTimer"></param>
static void read_telemetry_handler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    onboard_sensors_read(&telemetry.latest);

    if (co2_read(&telemetry))
    {
        // clang-format off
        telemetry.valid =
            IN_RANGE(telemetry.latest.temperature, -20, 50) &&
            IN_RANGE(telemetry.latest.pressure, 800, 1200) &&
            IN_RANGE(telemetry.latest.humidity, 0, 100) &&
            IN_RANGE(telemetry.latest.co2ppm, 0, 20000);
        // clang-format on
    }
    else
    {
        telemetry.valid = false;
    }

    dx_timerOneShotSet(&tmr_read_telemetry, &(struct timespec){20, 0});
}

/***********************************************************************************************************
 *
 * Generate buzzer alert if recorded CO2 level above CO2 alert level
 * Press Button B to Be Quiet. This overides the buzzer alert
 **********************************************************************************************************/

/// <summary>
/// Be quite button handler
/// Toggles global bool be_quiet variable
/// </summary>
static void read_buttons_handler(EventLoopTimer *eventLoopTimer)
{
    static GPIO_Value_Type button_b_state;

    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }

    if (dx_gpioStateGet(&gpio_button_b, &button_b_state))
    {
        be_quiet = !be_quiet;
    }
}

/// <summary>
/// Oneshot handler to turn off BUZZ Click buzzer
/// </summary>
/// <param name="eventLoopTimer"></param>
static void co2_alert_buzzer_off_handler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }
    dx_pwmStop(&pwm_buzz_click);
}

static void update_co2_alert_status(void)
{
    if (!telemetry.valid)
    {
        return;
    }

    // Light level is a percentage. 0% is dark, 100% very bright
    // Calculate brightness. LED brightness is inverted, 100% duty cycle is off, 0% duty cycle is full on
    uint32_t brightness = 100 - (uint32_t)telemetry.latest.light;
    brightness = brightness == 100 ? 99 : brightness;

    if (telemetry.latest.co2ppm > co2_alert_level)
    {
        if (!be_quiet)
        {
            dx_pwmSetDutyCycle(&pwm_buzz_click, 5000, 1);
            dx_timerOneShotSet(&tmr_co2_alert_buzzer_off_oneshot, &(struct timespec){0, 10 * ONE_MS});
        }

        dx_pwmSetDutyCycle(&pwm_led_blue, 1000, 100);
        dx_pwmSetDutyCycle(&pwm_led_red, 1000, brightness);
    }
    else
    {
        dx_pwmSetDutyCycle(&pwm_led_blue, 1000, brightness);
        dx_pwmSetDutyCycle(&pwm_led_red, 1000, 100);
    }
}

/// <summary>
/// Turn on CO2 Buzzer if recorded CO2 ppm greater than co2_alert_level
/// </summary>
static void co2_alert_handler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }
    update_co2_alert_status();
}

/***********************************************************************************************************
 * REMOTE OPERATIONS: DEVICE TWINS
 *
 * Set CO2 Alert level
 **********************************************************************************************************/

/// <summary>
/// Update the co2_alert_level from the device twin callback
/// </summary>
static void set_co2_alert_level(DX_DEVICE_TWIN_BINDING *deviceTwinBinding)
{
    if (IN_RANGE(*(int *)deviceTwinBinding->propertyValue, 0, 20000))
    {
        co2_alert_level = *(int *)deviceTwinBinding->propertyValue;
        update_co2_alert_status();
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
    }
    else
    {
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_ERROR);
    }
}

static void set_device_altitude(DX_DEVICE_TWIN_BINDING *deviceTwinBinding)
{
    if (IN_RANGE(*(int *)deviceTwinBinding->propertyValue, 0, 10000))
    {
        co2_set_altitude(*(int *)deviceTwinBinding->propertyValue);
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_COMPLETED);
    }
    else
    {
        dx_deviceTwinAckDesiredValue(deviceTwinBinding, deviceTwinBinding->propertyValue, DX_DEVICE_TWIN_RESPONSE_ERROR);
    }
}

/***********************************************************************************************************
 * PRODUCTION
 *
 * Remote device restart
 * Deferred update support
 * Enable application level watchdog
 * Update software version and Azure connect UTC time device twins on first connection
 **********************************************************************************************************/

/***********************************************************************************************************
 * Add remote device restart support
 **********************************************************************************************************/

/// <summary>
/// Oneshot timer set from the restart direct method handler
/// </summary>
/// <param name="eventLoopTimer"></param>
static void delayed_restart_device_handler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }
    PowerManagement_ForceSystemReboot();
}


/// <summary>
/// Direct method sets oneshot timer to restart the device.
/// The delayed restart is to allow for the direct method to return success status to IoT Hub
/// </summary>
/// <param name="json"></param>
/// <param name="directMethodBinding"></param>
/// <param name="responseMsg"></param>
/// <returns></returns>
static DX_DIRECT_METHOD_RESPONSE_CODE restart_device_handler(JSON_Value *json, DX_DIRECT_METHOD_BINDING *directMethodBinding,
                                                             char **responseMsg)
{
    dx_timerOneShotSet(&tmr_delayed_restart_device, &(struct timespec){2, 0});
    return DX_METHOD_SUCCEEDED;
}

/***********************************************************************************************************
 * Add deferred update support
 **********************************************************************************************************/

/// <summary>
/// Algorithm to determine if a deferred update can proceed
/// </summary>
/// <param name="max_deferral_time_in_minutes">The maximum number of minutes you can defer</param>
/// <returns>Return 0 to start update, return greater than zero to defer</returns>
static uint32_t DeferredUpdateCalculate(uint32_t max_deferral_time_in_minutes, SysEvent_UpdateType type, SysEvent_Status status,
                                        const char *typeDescription, const char *statusDescription)
{
    // UTC +11 is for Australia/Sydney AEDT
    // Set the time_zone_offset to your time zone offset.
    const int time_zone_offset = 11;

    //  Get UTC time
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);

    // Calculate UTC plus offset and normalize.
    t->tm_hour += time_zone_offset;
    t->tm_hour = t->tm_hour % 24;

    // If local time is between 1 am and 5 am defer for zero minutes else defer for 15 minutes
    uint32_t requested_minutes = IN_RANGE(t->tm_hour, 1, 5) ? 0 : 15;

    char utc[40];

    // Update defer requested device twin
    snprintf(msgBuffer, sizeof(msgBuffer), "Utc: %s, Type: %s, Status: %s, Max defer minutes: %i, Requested minutes: %i",
             dx_getCurrentUtc(utc, sizeof(utc)), typeDescription, statusDescription, max_deferral_time_in_minutes, requested_minutes);

    dx_deviceTwinReportValue(&dt_defer_requested, msgBuffer);

    return requested_minutes;
}

/************************************************************************************************************
 * Add application level watchdog
 ***********************************************************************************************************/

/// <summary>
/// This timer extends the app level lease watchdog
/// </summary>
/// <param name="eventLoopTimer"></param>
static void watchdog_handler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0)
    {
        dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
        return;
    }
    timer_settime(watchdogTimer, 0, &watchdogInterval, NULL);
}

/// <summary>
/// Set up watchdog timer - the lease is extended via the Watchdog_handler function
/// </summary>
/// <param name=""></param>
void start_watchdog(void)
{
    struct sigevent alarmEvent;
    alarmEvent.sigev_notify = SIGEV_SIGNAL;
    alarmEvent.sigev_signo = SIGALRM;
    alarmEvent.sigev_value.sival_ptr = &watchdogTimer;

    if (timer_create(CLOCK_MONOTONIC, &alarmEvent, &watchdogTimer) == 0)
    {
        if (timer_settime(watchdogTimer, 0, &watchdogInterval, NULL) == -1)
        {
            Log_Debug("Issue setting watchdog timer. %s %d\n", strerror(errno), errno);
        }
    }
}

/************************************************************************************************************
 * Update software version and Azure connect UTC time device twins on first connection
 ***********************************************************************************************************/

/// <summary>
/// Called when the Azure connection status changes then unregisters this callback
/// </summary>
/// <param name="connected"></param>
static void hvac_startup_report(bool connected)
{
    snprintf(msgBuffer, sizeof(msgBuffer), "CO2 Monitor firmware: %s, DevX version: %s", CO2_MONITOR_FIRMWARE_VERSION,
             AZURE_SPHERE_DEVX_VERSION);
    dx_deviceTwinReportValue(&dt_sw_version, msgBuffer);                                       // DX_TYPE_STRING
    dx_deviceTwinReportValue(&dt_startup_utc, dx_getCurrentUtc(msgBuffer, sizeof(msgBuffer))); // DX_TYPE_STRING

    dx_azureUnregisterConnectionChangedNotification(hvac_startup_report);
}

/***********************************************************************************************************
 * APPLICATION BASICS
 *
 * Set Azure connection state
 * Initialize CO2 sensor
 * Initialize resources
 * Close resources
 * Run the main event loop
 **********************************************************************************************************/

/// <summary>
/// Update local azure_connected with new connection status
/// </summary>
/// <param name="connected"></param>
void azure_connection_state(bool connected)
{
    azure_connected = connected;
}

/// <summary>
///  Initialize peripherals, device twins, direct methods, timer_bindings.
/// </summary>
static void InitPeripheralsAndHandlers(void)
{
    dx_Log_Debug_Init(Log_Debug_Time_buffer, sizeof(Log_Debug_Time_buffer));

    dx_pwmSetOpen(pwm_bindings, NELEMS(pwm_bindings));
    dx_i2cSetOpen(i2c_bindings, NELEMS(i2c_bindings));

    // Onboard LEDs are wired such that high voltage is off, low is on
    // Slightly unintuitive, but a 100% duration cycle turns the LED off
    dx_pwmSetDutyCycle(&pwm_led_red, 1000, 100);
    dx_pwmSetDutyCycle(&pwm_led_green, 1000, 100);
    dx_pwmSetDutyCycle(&pwm_led_blue, 1000, 100);

    dx_azureConnect(&dx_config, NETWORK_INTERFACE, IOT_PLUG_AND_PLAY_MODEL_ID);

    co2_initialize();

    onboard_sensors_init();

    dx_gpioSetOpen(gpio_bindings, NELEMS(gpio_bindings));
    dx_timerSetStart(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinSubscribe(device_twin_bindings, NELEMS(device_twin_bindings));
    dx_directMethodSubscribe(direct_method_bindings, NELEMS(direct_method_bindings));

    dx_deferredUpdateRegistration(DeferredUpdateCalculate, NULL);

    // Up to five callbacks can be registered for Azure connection status changes
    dx_azureRegisterConnectionChangedNotification(azure_connection_state);
    dx_azureRegisterConnectionChangedNotification(hvac_startup_report);

    dx_timerOneShotSet(&tmr_read_telemetry, &(struct timespec){0, 250 * ONE_MS});

    telemetry.previous.temperature = telemetry.previous.pressure = telemetry.previous.humidity = telemetry.previous.co2ppm = INT32_MAX;

    // Uncomment for production
    // start_watchdog();
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    dx_timerSetStop(timer_bindings, NELEMS(timer_bindings));
    dx_deviceTwinUnsubscribe();
    dx_gpioSetClose(gpio_bindings, NELEMS(gpio_bindings));
    dx_i2cSetClose(i2c_bindings, NELEMS(i2c_bindings));
    dx_pwmSetClose(pwm_bindings, NELEMS(pwm_bindings));
    dx_timerEventLoopStop();
}

int main(int argc, char *argv[])
{
    dx_registerTerminationHandler();

    if (!dx_configParseCmdLineArguments(argc, argv, &dx_config))
    {
        return dx_getTerminationExitCode();
    }

    InitPeripheralsAndHandlers();

    // Main loop
    while (!dx_isTerminationRequired())
    {
        int result = EventLoop_Run(dx_timerGetEventLoop(), -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == -1 && errno != EINTR)
        {
            dx_terminate(DX_ExitCode_Main_EventLoopFail);
        }
    }

    ClosePeripheralsAndHandlers();
    dx_Log_Debug("Application exiting.\n");
    return dx_getTerminationExitCode();
}