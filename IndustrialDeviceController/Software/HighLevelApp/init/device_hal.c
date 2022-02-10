/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include <init/globals.h>
#include <init/device_hal.h>
#include <frozen/frozen.h>
#include <utils/llog.h>
#include <utils/utils.h>
#include <driver/modbus.h>

// must align with device_protocol_t enum
static const char *protocol_str[] = {
    "INVALID",
    "MODBUS_TCP",
    "MODBUS_RTU"
};

static const char *error_name[] = {
    "DEVICE_OK",
    "DEVICE_E_INVALID",
    "DEVICE_E_IO",
    "DEVICE_E_BROKEN",
    "DEVICE_E_PROTOCOL",
    "DEVICE_E_TIMEOUT",
    "DEVICE_E_INTERNAL",
    "DEVICE_E_CONFIG",
    "DEVICE_E_BUSY",
    "DEVICE_E_NO_DATA"
};

const char *err_str(err_code err)
{
    return (err < DEVICE_E_LAST) ? error_name[err] : NULL;
}


err_code create_point_table(device_protocol_t protocol, struct json_token *points_def, int* npoints, data_point_t **ppoints)
{
    switch (protocol) {
    case DEVICE_PROTOCOL_MODBUS_RTU:
    case DEVICE_PROTOCOL_MODBUS_TCP:
        return modbus_create_point_table(points_def, npoints, ppoints);

    default:
        LOGE("Invalid protocol:%d", protocol);
        return DEVICE_E_INVALID;
    }
}


void destroy_point_table(device_protocol_t protocol, data_point_t *points, int npoints)
{
    ASSERT(points);

    switch (protocol) {
    case DEVICE_PROTOCOL_MODBUS_RTU:
    case DEVICE_PROTOCOL_MODBUS_TCP:
        modbus_destroy_point_table(points, npoints);
        break;

    default:
        LOGE("Invalid protocol:%d", protocol);
    }
}


device_driver_t *create_driver(device_protocol_t protocol, const char *conn_str)
{
    switch(protocol) {
    case DEVICE_PROTOCOL_MODBUS_RTU:
    case DEVICE_PROTOCOL_MODBUS_TCP:
        return modbus_create_driver(protocol, conn_str);

    default:
        LOGE("Invalid protocol:%d", protocol);
        return NULL;
    }
    return NULL;
}


void destroy_driver(device_driver_t *driver)
{
    ASSERT(driver);

    switch (driver->get_protocol(driver)) {
    case DEVICE_PROTOCOL_MODBUS_RTU:
    case DEVICE_PROTOCOL_MODBUS_TCP:
        modbus_destroy_driver(driver);
        break;

    default:
        LOGE("Invalid protocol");
    }
}


device_protocol_t str2protocol(const char *str)
{
    ASSERT(str);

    int nprotocols = sizeof(protocol_str) / sizeof(protocol_str[0]);

    for (int i = 0; i < nprotocols; i++) {

        if (strcmp(str, protocol_str[i]) == 0)
            return i;
    }

    return DEVICE_PROTOCOL_INVALID;
}

const char *protocol2str(const device_protocol_t protocol)
{
    unsigned int nprotocols = sizeof(protocol_str) / sizeof(protocol_str[0]);

    if (protocol >= 0 && protocol < nprotocols) {
        return protocol_str[protocol];
    } else {
        return protocol_str[DEVICE_PROTOCOL_INVALID];
    }
}

void update_telemetry_value(telemetry_t *telemetry, int index, const char *str_value)
{
    double num_value = NAN;
    bool is_str = false;
    if (str_value) {
        char *endptr = NULL;
        num_value = strtod(str_value, &endptr);
        is_str = (str_value == endptr);
    }
    bool was_str = test_mask(telemetry->str_mask, index);

    if (!is_str && !was_str) {
        if (is_double_equal(num_value, telemetry->values[index].num)) {
            clear_mask(telemetry->cov_mask, index);
        } else {
            telemetry->values[index].num = num_value;
            set_mask(telemetry->cov_mask, index);
        }
    } else if (is_str && was_str) {
        if (strcmp(str_value, telemetry->values[index].str) == 0) {
            clear_mask(telemetry->cov_mask, index);
        } else {
            FREE(telemetry->values[index].str);
            telemetry->values[index].str = STRDUP(str_value);
            set_mask(telemetry->cov_mask, index);
        }
    } else if (is_str && !was_str) {
        set_mask(telemetry->str_mask, index);
        telemetry->values[index].str = STRDUP(str_value);
        set_mask(telemetry->cov_mask, index);
    } else if (!is_str && was_str) {
        clear_mask(telemetry->str_mask, index);
        FREE(telemetry->values[index].str);
        telemetry->values[index].num = num_value;
        set_mask(telemetry->cov_mask, index);
    }
}

void set_telemetry_number_value(telemetry_t *telemetry, int index, double num_value)
{
    clear_mask(telemetry->str_mask, index);
    if (is_double_equal(telemetry->values[index].num, num_value)) {
        clear_mask(telemetry->cov_mask, index);
    } else {
        set_mask(telemetry->cov_mask, index);
        telemetry->values[index].num = num_value;
    }
}

void set_telemetry_string_value(telemetry_t *telemetry, int index, char *str_value)
{
    set_mask(telemetry->str_mask, index);
    if (strequal(telemetry->values[index].str, str_value)) {
        clear_mask(telemetry->cov_mask, index);
    } else {
        set_mask(telemetry->cov_mask, index);
        FREE(telemetry->values[index].str);
        telemetry->values[index].str = STRDUP(str_value);
    }
}
