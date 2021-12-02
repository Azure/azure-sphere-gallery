/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stdint.h>

#include <init/device_hal.h>

#define MODBUS_MAX_PDU_SIZE 253

// only define supported function code
enum {
    FC_INVALID = 0x00,
    FC_READ_COILS = 0x01,
    FC_READ_DISCRETE_INPUTS = 0x02,
    FC_READ_HOLDING_REGISTERS = 0x03,
    FC_READ_INPUT_REGISTERS = 0x04,

    FC_WRITE_SINGLE_COIL = 0x05,
    FC_WRITE_SINGLE_REGISTER = 0x06,
    FC_READ_EXCEPTION_STATUS = 0x07,
    FC_DIAGNOSTICS = 0x08,
    FC_GET_COMM_EVENT_COUNTER = 0x0B,
    FC_GET_COMM_EVENT_LOG = 0x0C,
    FC_WRITE_COILS = 0x0F,
    FC_WRITE_HOLDING_REGISTERS = 0x10,
    FC_REPORT_SERVER_ID = 0x11,
    FC_READ_FILE_RECORD = 0x14,
    FC_WRITE_FILE_RECORD = 0x15,
    FC_MASK_WRITE_REGISTER = 0x16,
    FC_READ_WRITE_REGISTERS = 0x17,
    FC_READ_FIFO_QUEUE = 0x18,
    FC_MEI = 0x2B,
    FC_READ_DEVICE_IDENTITY = 0x2B

};

struct modbus_device_t;

/**
 * read modbus registers
 * @param modbus instance pointer
 * @param slave_id node slave id
 * @param reg_type register type to read
 * @param addr register address to read
 * @param quantity register quantity to read
 * @param regs out parameter contains value of registers on success
 * @param timeout millisecond protection timer
 * @return DEVICE_OK on succeed or error code
 */
err_code mb_read_register(struct modbus_device_t *modbus, uint8_t slave_id, uint8_t reg_type, uint16_t addr,
                                 uint16_t quantity, uint16_t *regs, int32_t timeout);

/**
 * write modbus registers
 * @param modbus instance pointer
 * @param slave_id node slave id
 * @param reg_type register type to read
 * @param addr register address to read
 * @param quantity register quantity to read
 * @param regs contains value to be written to register
 * @param timeout millisecond protection timer
 * @return DEVICE_OK on succeed or error code
 */
err_code mb_write_register(struct modbus_device_t *modbus, uint8_t slave_id, uint8_t reg_type, uint16_t addr, uint16_t quantity,
                             uint16_t *regs, int32_t timeout);

struct json_token;
/**
 * create modbus point defintion table
 * @param points_def json token contain point defintion for modbus
 * @param npoints out parameter contains number of points parsed on success
 * @param ppoints out parameter contains array of points parsed on success
 * @return DEVICE_OK on succeed or error code
 */
err_code modbus_create_point_table(struct json_token *points_def, int *npoints, data_point_t **ppoints);

/**
 * destroy modbus point defintion table
 * @param points point defintion table to be destroyed
 * @param npoints number of points in table
 */
void modbus_destroy_point_table(data_point_t *points, int npoints);

/**
 * create modbus device driver
 * @param protocol modbus rtu or tcp
 * @param conn_str uart config for rtu, "ip:port" for tcp
 * @return device driver created
 */
device_driver_t *modbus_create_driver(device_protocol_t protocol, const char *conn_str);

/**
 * destroy modbus device driver
 * @param self device driver to be destroyed
 */
void modbus_destroy_driver(device_driver_t *self);
