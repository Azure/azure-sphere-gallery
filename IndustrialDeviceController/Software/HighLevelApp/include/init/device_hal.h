/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <frozen/frozen.h>

// bitmap flags
#define FLAG_NONE         0x0
#define FLAG_NO_BATCH     0x00000001u
#define FLAG_CE_TIMESTAMP 0x00000002u
#define FLAG_COV          0x00000004u

#define FLAG_NO_BATCH_STR     "no_batch"
#define FLAG_CE_TIMESTAMP_STR "ce_timestamp"
#define FLAG_COV_STR          "cov"


#define IS_COV(telemetry, index) test_mask((telemetry)->cov_mask, index)
#define IS_STR_VALUE(telemetry, index) test_mask((telemetry)->str_mask, index)
#define IS_NUM_VALUE(telemetry, index) (!test_mask((telemetry)->str_mask, index))

typedef enum err_code err_code;
enum err_code {
    DEVICE_OK,
    DEVICE_E_INVALID,  // invalid operation
    DEVICE_E_IO,       // IO error
    DEVICE_E_BROKEN,   // connection broken
    DEVICE_E_PROTOCOL, // protocol error, bad pdu, out of sync
    DEVICE_E_TIMEOUT,  // timeout
    DEVICE_E_INTERNAL, // internal logic error, ASSERT
    DEVICE_E_CONFIG,   // device configuration error
    DEVICE_E_BUSY,     // Garbage data on link
    DEVICE_E_NO_DATA,  // data not avaiable
    DEVICE_E_LAST
};

typedef enum device_protocol_t {
    DEVICE_PROTOCOL_INVALID,
    DEVICE_PROTOCOL_MODBUS_TCP,
    DEVICE_PROTOCOL_MODBUS_RTU
} device_protocol_t;


typedef struct modbus_point_t modbus_point_t;
struct modbus_point_t {
    int32_t value_offset;
    float scale;
    uint16_t addr;
    uint8_t reg_type : 4;
    uint8_t data_type : 4;
    uint8_t bit_offset : 4;
};

typedef struct data_point_t data_point_t;
struct data_point_t {
    char *key;
    union {
        modbus_point_t modbus;
    } d;
};


typedef struct data_schema_t data_schema_t;
struct data_schema_t {
    char *name;
    int interval;
    int timeout;
    uint32_t flags;
    int offset;
    device_protocol_t protocol;
    data_point_t *points;
    int32_t num_point;
    int32_t integrity_period_ms;
    data_schema_t *next;
};

typedef union telemetry_value_t telemetry_value_t;
union telemetry_value_t {
        double num;
        char *str;
};

typedef struct telemetry_t telemetry_t;
struct telemetry_t {
    uint8_t *cov_mask;
    uint8_t *str_mask;
    telemetry_value_t *values;
    int num_values;
};

struct device_driver_t;
typedef struct ce_device_t ce_device_t;
struct ce_device_t {
    // scheduled time for next poll
    struct timespec ts_schedule;

    // ms of last poll
    int32_t poll_duration;

    // which protocol device speake
    device_protocol_t protocol;

    // unique device name that identify a device, e.g. MWH01_SIMENSE_PXC36_AHU
    char* name;

    // protocol specific format
    data_schema_t* schema;

    // to support modbus schema address offset
    int32_t schema_offset;

    // report interval in ms
    int32_t interval;

    // poll timeout in ms
    int32_t timeout;

    // The location of the CE device, in the format of "BY22:COLO1:tile"
    char* location;

    // error code for last polling
    err_code err;

    // connection string for the CE device which will override downlink data from adapter
    char* connection;

    // device id, assume unit32 value unless there is new requirement
    uint32_t id;

    telemetry_t *telemetry;

    struct timespec last_flush_ts;

    ce_device_t* next;    
};


typedef struct device_driver_t device_driver_t;
struct device_driver_t {
/**
 * open device driver
 * @param self pointer to driver to be opened
 * @param id driver channel to be opened
 * @param timeout protection timer in ms
 * @return DEVICE_OK on succeed or error code
 */
    err_code (*driver_open)(void *instance, uint32_t id, int32_t timeout);

/**
 * close device driver
 * @param self pointer to driver to be closed
 * @return DEVICE_OK on succeed or error code
 */
    err_code (*driver_close)(void *instance);

/**
 * query a single data point value
 * @param self point to driver to be queried
 * @param id channel to be queried
 * @param key data point key
 * @param schema data schema which contain defintion of data point
 * @param telemetry out parameter which contain queried value on succeed, only value of given data points will be updated
 * @param timeout protection timer in ms
 * @return DEVICE_OK on succeed or error code
 */
    err_code (*get_point)(void *instance, uint32_t id, const char *key, data_schema_t *schema, telemetry_t *telemetry, int32_t timeout);

/**
 * query all data point value defined in schema
 * @param self point to driver to be queried
 * @param id channel to be queried
 * @param schema data schema which contain defintion of data points
 * @param telemetry out parameter which contain queried values on succeed
 * @param timeout protection timer in ms
 * @return DEVICE_OK on succeed or error code
 */
    err_code (*get_point_list)(void *instance, uint32_t id, data_schema_t *schema, telemetry_t *telemetry, int32_t timeout);

/**
 * set data point to a new value
 * @param self point to driver to be used
 * @param id channel to be use
 * @param key data point name to be set
 * @param value data point value to be set
 * @param schema data schema which contain defintion of data points
 * @param timeout protection timer in ms
 * @return DEVICE_OK on succeed or error code
 */
    err_code (*set_point)(void *instance, uint32_t id, const char *key, const char *value, data_schema_t *schema,
                          int32_t timeout);

/**
 * get protocol of current driver
 * @param instance driver instance
 * @return device protocol of current driver
 */
    device_protocol_t (*get_protocol)(void *instance);
};

/**
 * set ith bit of mask byte array
 * @param mask byte array to be updated
 * @param i ith bit to be set
 */
static inline void set_mask(uint8_t *mask, int i)
{
    mask[i/8] |= 1 << (i%8);
}

/**
 * clear ith bit of mask byte array
 * @param mask byte array to be updated
 * @param i ith bit to be cleared
 */
static inline void clear_mask(uint8_t *mask, int i)
{
    mask[i/8] &= ~(1 << (i % 8));
}

/**
 * test ith bit of mask byte array
 * @param mask byte array to be tested
 * @param i ith bit to be tested
 */
static inline bool test_mask(uint8_t *mask, int i)
{
    return mask[i/8] & (1 << (i%8));
}

/**
 * convert error code to error string
 * @param err error code
 * @return error string
 */
const char *err_str(err_code err);

/**
 * parse protocol string value to protocol enum value
 * @param str - protocol string value
 * @return protocol enum value
 */ 
device_protocol_t str2protocol(const char *str);

/**
 * convert protocol enum value to protocol string value
 * @param proto - protocol enum value
 * @return protocol string value
 */ 
const char *protocol2str(device_protocol_t proto);

/**
 * factory method to create point defintiion table
 * @param protocol protocol enum of table to be parsed
 * @param points_def json token contains table to be parsed
 * @param npoints out parameter contains number of point parsed when succeed
 * @param ppoints out parameter contains points array parsed when succeed
 * @return DEVICE_OK on succeed or error code
 */
err_code create_point_table(device_protocol_t protocol, struct json_token *points_def, int *npoints, data_point_t **ppoints);

/**
 * destroy point defintion table
 * @param protocol protocol enum value of table to be destroyed
 * @param points point array to be destroyed
 * @param npoints number of points in array
 */
void destroy_point_table(device_protocol_t protocol, data_point_t *points, int npoints);

/**
 * factory method to create device driver
 * @param protocol protocol enum value of driver to be created
 * @param conn_str connection string for driver to be created, format is driver type specific
 * @return driver created or NULL if failed
 */
device_driver_t *create_driver(device_protocol_t protocol, const char *conn_str);

/**
 * destroy device driver
 * @param driver driver to be destroyed
 */
void destroy_driver(device_driver_t *driver);

/**
 * update ith value of telemetry and als taken care of clear/set str and cov mask
 * @param telemetry telemetry to be updated
 * @param index ith element to be updated
 * @param str_value string value to be updated
 */
void update_telemetry_value(telemetry_t *telemetry, int index, const char *str_value);


/**
 * set ith value of telemetry to a number value
 * @param telemetry telemetry to be updated
 * @param index ith element to be updated
 * @param num_value number value to be updated
 */
void set_telemetry_number_value(telemetry_t *telemetry, int index, double num_value);

/**
 * set ith value of telemetry to a string value
 * @param telemetry telemetry to be updated
 * @param index ith element to be updated
 * @param str_value string value to be updated, old value will be freed and new value will be
 * malloc and copied over
 */
void set_telemetry_string_value(telemetry_t *telemetry, int index, char *str_value);