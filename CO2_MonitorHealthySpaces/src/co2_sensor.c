/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "co2_sensor.h"

#ifdef SCD30

/// <summary>
/// Initialize the SDC30 CO2 and Humidity sensor
/// </summary>
/// <param name=""></param>
/// <returns></returns>
bool co2_initialize(void)
{
    uint16_t interval_in_seconds = 2;
    int retry = 0;
    uint8_t asc_enabled, enable_asc;

    sensirion_i2c_init(i2c_co2_sensor.fd);

    while (scd30_probe() != STATUS_OK && ++retry < 5)
    {
        printf("SCD30 sensor probing failed\n");
        sensirion_sleep_usec(1000000u);
    }

    if (retry >= 5)
    {
        return false;
    }

    /*
    When scd30 automatic self calibration activated for the first time a period of minimum 7 days is needed so
    that the algorithm can find its initial parameter set for ASC. The sensor has to be exposed to fresh air for at least 1 hour every day.
    Refer to the datasheet for further conditions and scd30.h for more info.
    */

    if (scd30_get_automatic_self_calibration(&asc_enabled) == 0)
    {
        if (asc_enabled == 0)
        {
            enable_asc = 1;
            if (scd30_enable_automatic_self_calibration(enable_asc) == 0)
            {
                Log_Debug("scd30 automatic self calibration enabled. Takes 7 days, at least 1 hour/day outside, powered continuously");
            }
        }
    }

    scd30_set_measurement_interval(interval_in_seconds);
    sensirion_sleep_usec(20000u);
    scd30_start_periodic_measurement(0);
    sensirion_sleep_usec(interval_in_seconds * 1000000u);

    return true;
}

#else

/// <summary>
/// Initialize the SDC30 CO2 and Humidity sensor
/// </summary>
/// <param name=""></param>
/// <returns></returns>
bool co2_initialize(void)
{
    uint16_t asc_enabled, enable_asc;
    sensirion_i2c_hal_init(i2c_co2_sensor.fd);

    // Clean up potential SCD40 states
    scd4x_wake_up();
    scd4x_stop_periodic_measurement();
    scd4x_reinit();

    uint16_t serial_0;
    uint16_t serial_1;
    uint16_t serial_2;
    int error = scd4x_get_serial_number(&serial_0, &serial_1, &serial_2);
    if (error)
    {
        printf("Error executing scd4x_get_serial_number(): %i\n", error);
    }
    else
    {
        printf("serial: 0x%04x%04x%04x\n", serial_0, serial_1, serial_2);
    }

    if (scd4x_get_automatic_self_calibration(&asc_enabled) == NO_ERROR)
    {
        if (asc_enabled == 0)
        {
            enable_asc = 1;
            if (scd4x_set_automatic_self_calibration(enable_asc) == NO_ERROR)
            {
                Log_Debug("scd30 automatic self calibration enabled. Takes 7 days, at least 1 hour/day outside, powered continuously");
            }
        }
    }

    error = scd4x_start_periodic_measurement();
    if (error)
    {
        printf("Error executing scd4x_start_periodic_measurement(): %i\n", error);
    }

    sensirion_i2c_hal_sleep_usec(5000000);

    return 0;
}

#endif

bool co2_read(ENVIRONMENT *telemetry)
{
#ifdef SCD30
    float co2_ppm, temperature, relative_humidity;

    if (scd30_read_measurement(&co2_ppm, &temperature, &relative_humidity) == STATUS_OK)
    {
        if (!isnan(co2_ppm) && !isnan(temperature) && !isnan(relative_humidity))
        {
            telemetry->latest.co2ppm = (int)co2_ppm;
            telemetry->latest.temperature = (int)temperature;
            telemetry->latest.humidity = (int)relative_humidity;
            return true;
        }
    }
    return false;
#else
    uint16_t co2_ppm;
    int32_t temperature;
    int32_t relative_humidity;

    if (scd4x_read_measurement(&co2_ppm, &temperature, &relative_humidity) == NO_ERROR)
    {
        telemetry->latest.co2ppm = co2_ppm;
        telemetry->latest.temperature = temperature / 1000;
        telemetry->latest.humidity = relative_humidity / 1000;
        return true;
    }
    return false;
#endif
}

bool co2_set_altitude(int altitude_in_meters)
{
    bool result = false;

#ifdef SCD30
    // can only set altitude in idle mode
    scd30_stop_periodic_measurement();
    result = scd30_set_altitude((uint16_t)altitude_in_meters) == 0;
    scd30_start_periodic_measurement(0);
#else
    // can only set altitude in idle mode
    scd4x_stop_periodic_measurement();
    result = scd4x_set_sensor_altitude((uint16_t)altitude_in_meters) == 0;
    scd4x_start_periodic_measurement();
#endif

    return result;
}