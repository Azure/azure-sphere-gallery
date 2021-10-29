/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include "Onboard/onboard_sensors.h"

#ifdef SCD30
#include "AzureSphereDrivers/EmbeddedScd30/scd30/scd30.h"
#else
#include "scd4x_i2c.h"
#include "sensirion_i2c_hal.h"
#include "sensirion_common.h"
#endif

extern DX_I2C_BINDING i2c_co2_sensor;

/// <summary>
/// Initialize the CO2 sensor
/// </summary>
/// <param name=""></param>
/// <returns></returns>
bool co2_initialize(void);

/// <summary>
/// Read CO2 sensor telemetry
/// </summary>
/// <param name="telemetry"></param>
/// <returns></returns>
bool co2_read(ENVIRONMENT *telemetry);

/// <summary>
/// Set the CO2 sensor altitude
/// </summary>
/// <param name="altitude_in_meters"></param>
/// <returns></returns>
bool co2_set_altitude(int altitude_in_meters);
