/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_i2c.h"
#include "hw/co2_monitor.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#ifdef OEM_AVNET
#include "../AzureSphereDrivers/AVNET/HL/imu_temp_pressure.h"
#include "../AzureSphereDrivers/AVNET/HL/light_sensor.h"
#endif

typedef struct {
    int temperature;
    int pressure;
    int humidity;
    int co2ppm;
    int light;
} SENSOR;

typedef struct {
    SENSOR latest;
    SENSOR previous;
    bool updated;
    bool valid;
} ENVIRONMENT;

bool onboard_sensors_read(SENSOR *telemetry);
bool onboard_sensors_init(void);
bool onboard_sensors_close(void);

extern DX_I2C_BINDING i2c_onboard_sensors;