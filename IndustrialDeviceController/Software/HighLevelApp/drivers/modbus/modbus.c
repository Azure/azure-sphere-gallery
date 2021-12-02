/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>

#include <init/globals.h>
#include <init/device_hal.h>
#include <utils/utils.h>
#include <utils/llog.h>
#include <utils/timer.h>
#include <driver/modbus.h>

#include <frozen/frozen.h>
#include <safeclib/safe_lib.h>

#include "modbus_transport.h"

#define MAX_FIELD_LENGTH 100

#define MODBUS_MAX_BIT_PER_READ 0x7D0
#define MODBUS_MAX_WORD_PER_READ 0x7D
#define MODBUS_MAX_BIT_PER_WRITE 0x7B0
#define MODBUS_MAX_WORD_PER_WRITE 0x7B

#define MODBUS_READ_REQUEST_FRAME_LENGTH 5

// data point definition array field sequence
enum {
    MODBUS_SCHEMA_FIELD_KEY,
    MODBUS_SCHEMA_FIELD_NUMBER,
    MODBUS_SCHEMA_FIELD_TYPE,
    MODBUS_SCHEMA_FIELD_BIT,
    MODBUS_SCHEMA_FIELD_MULTIPLIER,
    MODBUS_SCHEMA_FIELD_OFFSET,
    MODBUS_SCHEMA_FIELD_LAST
};


// intentionally to make it align with the address number.
// register number  Data Addresses  Table Name
// 000001 - 065536	0000 - FFFF 	Discrete Output Coils
// 100001 - 165536	0000 - FFFF 	Discrete Input Contacts
// 300001 - 365536	0000 - FFFF 	Analog Input Registers
// 400001 - 465536	0000 - FFFF 	Analog Output Holding Registers
enum {
    COIL = 0,
    DISCRETE_INPUT = 1,
    INVALID = 2,
    INPUT_REGISTER = 3,
    HOLDING_REGISTER= 4
};

static const char *REG_NAMES[] =
{
    "COIL",
    "DISCRETE_INPUT",
    "INVALID",
    "INPUT_REGISTER",
    "HOLDING_REGISTER"
};

// modbus protocol only defined two data type, bit (bool) or register (uint16)
// OEM defined more data type based on register like int32,uint32, float etc.
// This require additional decoding when getting a data point.
// In this code, we refer to raw bit/register value as "value" while deoded data
// as "point"
enum {
    TYPE_INVALID,
    TYPE_BIT,
    TYPE_BYTE,
    TYPE_INT16,
    TYPE_UINT16,
    TYPE_INT32_BE, // 0x1234 as [0] = 0x12, [1] = 0x34
    TYPE_INT32_LE, // 0x1234 as [0] = 0x34, [1] = 0x12
    TYPE_UINT32_BE,
    TYPE_UINT32_LE,
    TYPE_FLOAT_BE,
    TYPE_FLOAT_LE,
    TYPE_INT64_BE, //0x12345678 as [0]=0x12, [1]=0x34, [2]=0x56, [3]=0x78
    TYPE_INT64_LE, //0x12345678 as [0]=0x78, [1]=0x56, [2]=0x34, [3]=0x12
};

// key - standard data point name, we don't care vendor name
// reg_type - coil, discrete, input register, holding register as defined in modbus protocol
// data_type - as defined above, bit, byte, uint16, int16, uint32, int32, float
// addr - register offset part only, leading digit been extracted to reg_type already
// bit - Some vendor use one bit of input/holding register to represent binary value, bit offset
//       this indicate the bit offset in the register, for bit data type, one bit, for byte data
//       type, assume consecutive 8 bits. Not allow byte to spread across registers.
// scale & offset is to represent float data point value in integer on device does not support float
// A scaled integer value is calculated using the following equation:
// register_value = measured_value * scale + value_offset
// measured_value = (register_value - value_offset) / scale
// this only apply to integer, note the point datatype only defined how we interpret the register value
// the measured value could be anything after calculated with multiplier and value_offset.

// implement data_point_t interface, so make sure
// key, value, next is at the head

typedef struct modbus_device_t modbus_device_t;
struct modbus_device_t {
    struct device_driver_t base;
    device_protocol_t protocol;
    bool opened;
    modbus_transport_t *transport;
};


struct register_buffer_t {
    uint8_t reg_type;
    uint16_t begin_addr;
    uint16_t end_addr;
    uint16_t buf[2000];
};


/// <summary>
/// parse the response for modbus request to extract value
/// </summary>
/// <param name="modbus">this</param>
/// <param name="request">request pdu</param>
/// <param name="response">response pdu</param>
/// <param name="len_rsp">length ofresponse pdu</param>
/// <param name="regs">buffer to hold parsed value</param>
/// <returns>0 on success, or error code if failed</returns>
static err_code parse_read_response(modbus_device_t *modbus, uint8_t *request, uint8_t *response, int16_t len_rsp,
                                    uint16_t *regs)
{
    // minimum 2 bytes, even in error case
    if (len_rsp < 2) {
        LOGE("response less than 2 bytes");
        return DEVICE_E_PROTOCOL;
    }

    uint8_t function_req = request[0];
    uint8_t function_rsp = response[0];
    uint16_t quantity_req = (request[3] << 8) + request[4];
    uint8_t function = response[0];

    // if succeed, server echo back function code
    if (function_rsp == function_req) {
        uint8_t byte_count = response[1];
        uint8_t *payload = response + 2;

        if (byte_count + 2 != len_rsp) {
            LOGE("byte count not match header");
            return DEVICE_E_PROTOCOL;
        }

        if ((function == FC_READ_INPUT_REGISTERS) || (function == FC_READ_HOLDING_REGISTERS)) {
            // check bytes received match requested
            if (byte_count != 2 * quantity_req) {
                LOGE("byte count not match requested");
                return DEVICE_E_PROTOCOL;
            }

            for (uint16_t i = 0; i < quantity_req; i++) {
                // modbus is "big-endian" protocol
                regs[i] = (payload[i * 2] << 8) + payload[i * 2 + 1];
            }
        } else if (function == FC_READ_COILS || function == FC_READ_DISCRETE_INPUTS) {
            // check bytes received match requested
            if (byte_count != (quantity_req + 7) / 8) {
                LOGE("byte count not match request");
                return DEVICE_E_PROTOCOL;
            }

            for (uint16_t i = 0; i < quantity_req; i++) {
                regs[i] = (payload[i / 8] & (0x01u << (i % 8))) ? 1 : 0;
            }
        }
    } else if (function_rsp == (function_req | 0x80)) {
        // server echo back function code with LSB set when something wrong
        uint8_t exception_code = response[1];
        LOGW("Exception code: %d", exception_code);
        return DEVICE_E_PROTOCOL;
    } else {
        LOGW("Invalid server response: PDU:%s", hex(response, len_rsp));
        return DEVICE_E_PROTOCOL;
    }

    return DEVICE_OK;
}


/// <summary>
/// Build and send read request pdu, then receive and parse response
/// </summary>
/// <param name="modbus">this</param>
/// <param name="function_code">funtion code</param>
/// <param name="addr">start address</param>
/// <param name="quantity">number of register request</param>
/// <param name="regs">out buffer to hold register value</param>
/// <param name="timeout">the value of timer in ms for this operation</param>
/// <returns>0 on success, or error code on failure</returns>

static err_code handle_read_request(modbus_device_t *modbus, uint8_t slave_id, uint8_t function_code, uint16_t addr,
                                    uint16_t quantity, uint16_t *regs, int32_t timeout)
{
    uint8_t request[5];
    uint8_t response[MODBUS_MAX_PDU_SIZE];

    request[0] = function_code;          // MODBUS FUNCTION CODE
    request[1] = (addr >> 8) & 0xFF;     // START REGISTER (Hi)
    request[2] = addr & 0xFF;            // START REGISTER (Lo)
    request[3] = (quantity >> 8) & 0xFF; // NUMBER OF REGISTERS (Hi)
    request[4] = quantity & 0xFF;        // NUMBER OF REGISTERS (Lo)

    // send request
    struct timespec poll_sw;
    timer_stopwatch_start(&poll_sw);
    int err = modbus->transport->send_request(modbus->transport, slave_id, request, MODBUS_READ_REQUEST_FRAME_LENGTH,
                                              timeout);
    if (err) {
        LOGE("Failed to send request:%s", err_str(err));
        return err;
    }

    // recv response
    int32_t len_rsp;
    // sending is not a blocked operation, so only use timeout for receiving
    int32_t elapse_ms = timer_stopwatch_stop(&poll_sw);

    if (elapse_ms >= timeout) {
        return DEVICE_E_TIMEOUT;
    }

    err = modbus->transport->recv_response(modbus->transport, slave_id, response, &len_rsp, timeout - elapse_ms);
    if (err) {
        LOGE("Failed to receive response:%s", err_str(err));
        return err;
    }

    // parse reponse
    return parse_read_response(modbus, request, response, len_rsp, regs);
}


/// <summary>
/// parse the response of write request
/// </summary>
/// <param name="modbus">this</param>
/// <param name="request">request pdu</param>
/// <param name="response">response pdu</param>
/// <param name="len_rsp">length of response pdu</param>
/// <returns>0 on success, or error code if failed</returns>
static err_code parse_write_response(modbus_device_t *modbus, uint8_t *request, uint8_t *response, int16_t len_rsp)
{
    // minimum 2 bytes, even in error case
    if (len_rsp < 2) {
        LOGE("reponse less than 2 bytes");
        return DEVICE_E_PROTOCOL;
    }

    uint8_t function_req = request[0];
    uint8_t function_rsp = response[0];

    // if succeed, server echo back function code
    if (function_rsp == function_req) {
        uint16_t addr_req = (request[1] << 8) + request[2];
        uint16_t addr_rsp = (response[1] << 8) + response[2];
        uint16_t quantity_req = (request[3] << 8) + request[4];
        uint16_t quantity_rsp = (response[3] << 8) + response[4];

        if ((addr_req != addr_rsp) || (quantity_req != quantity_rsp)) {
            LOGE("Invalid packet received\n");
            return DEVICE_E_PROTOCOL;
        }
    } else if (function_rsp == (function_req | 0x80)) {
        // server echo back error code
        uint8_t exception_code = response[1];
        LOGW("Exception code: %d", exception_code);
        return DEVICE_E_PROTOCOL;
    } else {
        LOGW("Don't understand server response");
        return DEVICE_E_PROTOCOL;
    }

    return DEVICE_OK;
}

/// <summary>
/// Build and send read request pdu, then receive and parse response
// </summary>
/// <param name="modbus">this</param>
/// <param name="function_code">funtion code</param>
/// <param name="addr">start address</param>
/// <param name="quantity">number of register request</param>
/// <param name="regs">in buffer to hold register value to write</param>
/// <param name="timeout">the value of timer in ms for this operation</param>
/// <returns>0 on success, or error code on failure</returns>
static err_code handle_write_request(modbus_device_t *modbus, uint8_t slave_id, uint8_t function_code, uint16_t addr,
                                     uint16_t quantity, uint16_t *regs, int32_t timeout)
{
    uint8_t request[MODBUS_MAX_PDU_SIZE];
    uint8_t response[MODBUS_MAX_PDU_SIZE];

    memset(request, 0, sizeof(request));
    memset(response, 0, sizeof(response));

    request[0] = function_code;          // MODBUS FUNCTION CODE
    request[1] = (addr >> 8) & 0xFF;     // START REGISTER (Hi)
    request[2] = addr & 0xFF;            // START REGISTER (Lo)
    request[3] = (quantity >> 8) & 0xFF; // NUMBER OF REGISTERS (Hi)
    request[4] = quantity & 0xFF;        // NUMBER OF REGISTERS (Lo)

    if (function_code == FC_WRITE_COILS) {
        request[5] = (quantity + 7) / 8; // BYTE COUNT

        for (uint16_t i = 0; i < quantity; i++) {
            uint16_t byte_i = i / 8;
            uint16_t bit_i = i % 8;

            if (regs[i]) {
                request[6 + byte_i] |= (1 << bit_i);
            }
            else {
                request[6 + byte_i] &= ~(1 << bit_i);
            }
        }
    } else if (function_code == FC_WRITE_HOLDING_REGISTERS) {
        request[5] = quantity * 2; // BYTE COUNT
        unsigned char *output = (request + 6);

        for (int i = 0; i < quantity; i++) {
            output[2 * i] = (regs[i] >> 8);
            output[2 * i + 1] = regs[i] & 0xFF;
        }
    }

    // all write request has 6 bytes header plus addtional data
    struct timespec poll_sw;
    timer_stopwatch_start(&poll_sw);
    uint16_t len_req = 6 + request[5];
    err_code err = modbus->transport->send_request(modbus->transport, slave_id, request, len_req, timeout);
    if (err) {
        LOGE("Failed to send request:%s", err_str(err));
        return err;
    }

    int32_t len_rsp;
    int32_t elapse_ms = timer_stopwatch_stop(&poll_sw);
    if (elapse_ms >= timeout) {
        return DEVICE_E_TIMEOUT;
    }
    err = modbus->transport->recv_response(modbus->transport, slave_id, response, &len_rsp, timeout - elapse_ms);
    if (err) {
        LOGE("Failed to receive response:%s", err_str(err));
        return err;
    }

    return parse_write_response(modbus, request, response, len_rsp);
}


err_code mb_read_register(modbus_device_t *modbus, uint8_t slave_id, uint8_t reg_type, uint16_t addr,
                                 uint16_t quantity, uint16_t *regs, int32_t timeout)
{
    uint8_t fc = FC_INVALID;
    switch (reg_type) {
    case COIL:
        fc = FC_READ_COILS;
        break;
    case DISCRETE_INPUT:
        fc = FC_READ_DISCRETE_INPUTS;
        break;
    case INPUT_REGISTER:
        fc = FC_READ_INPUT_REGISTERS;
        break;
    case HOLDING_REGISTER:
        fc = FC_READ_HOLDING_REGISTERS;
    }

    if (fc != FC_INVALID) {
        return handle_read_request(modbus, slave_id, fc, addr, quantity, regs, timeout);
    }
    else {
        return DEVICE_E_INVALID;
    }
}

err_code mb_write_register(modbus_device_t *modbus, uint8_t slave_id, uint8_t reg_type, uint16_t addr, uint16_t quantity,
                             uint16_t *regs, int32_t timeout)
{
    uint8_t fc = FC_INVALID;
    switch (reg_type) {
    case COIL:
        fc = FC_WRITE_COILS;
        break;
    case HOLDING_REGISTER:
        fc = FC_WRITE_HOLDING_REGISTERS;
        break;
    }
    if (fc != FC_INVALID) {
        return handle_write_request(modbus, slave_id, fc, addr, quantity, regs, timeout);
    }
    else {
        return DEVICE_E_INVALID;
    }
}


// -------------------------- data representation layer --------------------------

/// <summary>
/// get number of register for a data points, e.g. FLOAT need 2 register
/// </summary>
/// <param name="mp">modbus point to be decoded</param>
/// <return>number of register to decode given data point</return>
static uint8_t num_reg(modbus_point_t *mp)
{
    if (mp->data_type <= TYPE_UINT16) {
        return 1;
    }
    else if (mp->data_type <= TYPE_FLOAT_LE) {
        return 2;
    }
    else if (mp->data_type <= TYPE_INT64_LE) {
        return 4;
    }
    else {
        return 0;
    }
}

/// <summary>
/// encode one data point value from measured value to register value
/// </summary>
/// <param name="mp">point to be encoded</param>
/// <param name="value_str">value to be encoded into point</param>
/// <returns>0 on success, or error code on failure</returns>
static err_code encode_point(modbus_device_t *modbus, modbus_point_t *mp, const char *str_value, uint16_t *reg)
{
    switch (mp->data_type) {
    case TYPE_BIT: {
        // default to 0
        uint8_t value = strtol(str_value, NULL, 10);

        if (mp->reg_type == COIL) {
            reg[0] = value ? 1 : 0;
        } else if (mp->reg_type == HOLDING_REGISTER) {
            int bit_offset = mp->bit_offset;

            if (value) {
                reg[0] |= 1 << bit_offset;
            }
            else {
                reg[0] &= ~(1 << bit_offset);
            }
        }
        break;
    }
    case TYPE_BYTE: {
        uint8_t value = strtod(str_value, NULL) * mp->scale + mp->value_offset;

        if (mp->reg_type == HOLDING_REGISTER) {
            for (int i = mp->bit_offset; i < mp->bit_offset + 8; i++) {
                if (value & (1 << (i - mp->bit_offset))) {
                    reg[0] |= 1 << i;
                }
                else {
                    reg[0] &= ~(1 << i);
                }
            }
        }
        break;
    }
    case TYPE_INT16:
    case TYPE_UINT16: {
        reg[0] = strtod(str_value, NULL) * mp->scale + mp->value_offset;
        break;
    }
    case TYPE_INT32_BE:
    case TYPE_UINT32_BE: {
        uint32_t value = strtod(str_value, NULL) * mp->scale + mp->value_offset;
        reg[0] = (value >> 16);
        reg[1] = value & 0xFFFF;
        break;
    }
    case TYPE_INT32_LE:
    case TYPE_UINT32_LE: {
        uint32_t value = strtod(str_value, NULL) * mp->scale + mp->value_offset;
        reg[1] = (value >> 16);
        reg[0] = value & 0xFFFF;
        break;
    }
    case TYPE_FLOAT_BE: {
        float value_f = strtof(str_value, NULL) * mp->scale + mp->value_offset;
        uint32_t value_u32 = *((uint32_t *)&value_f);
        reg[0] = (value_u32 >> 16);
        reg[1] = value_u32 & 0xFFFF;
        break;
    }
    case TYPE_FLOAT_LE: {
        float value_f = strtof(str_value, NULL) * mp->scale + mp->value_offset;
        uint32_t value_u32 = *((uint32_t *)&value_f);
        reg[1] = (value_u32 >> 16);
        reg[0] = value_u32 & 0xFFFF;
        break;
    }
    case TYPE_INT64_BE: {
        int64_t rv = 0;
        if (mp->scale != 1) {
            rv = strtod(str_value, NULL) * mp->scale + mp->value_offset;
        }
        else {
            rv = strtoll(str_value, NULL, 10) + mp->value_offset;
        }
        reg[0] = (rv >> 48) & 0xFFFF;
        reg[1] = (rv >> 32) & 0xFFFF;
        reg[2] = (rv >> 16) & 0xFFFF;
        reg[3] = rv & 0xFFFF;
        break;
    }
    case TYPE_INT64_LE: {
        int64_t rv = 0;
        if (mp->scale != 1) {
            rv = strtod(str_value, NULL) * mp->scale + mp->value_offset;
        }
        else {
            rv = strtoll(str_value, NULL, 10) + mp->value_offset;
        }
        reg[3] = (rv >> 48) & 0xFFFF;
        reg[2] = (rv >> 32) & 0xFFFF;
        reg[1] = (rv >> 16) & 0xFFFF;
        reg[0] = rv & 0xFFFF;
        break;
    }
    default:
        return DEVICE_E_PROTOCOL;
    }
    return DEVICE_OK;
}


/// <summary>
/// decode data point from register value to measured value
/// </summary>
/// <param name="point">point to be decoded</param>
/// <param name="value_str">buffer hold the decoded value</param>
/// <param name="value_str">size of buffer</param>
/// <returns>0 on success, or error code on failure</returns>
static err_code decode_point(modbus_device_t *modbus, modbus_point_t *mp, uint16_t *regs, double *value)
{
    // parse data
    switch (mp->data_type) {
    case TYPE_BIT: {
        if ((mp->reg_type == INPUT_REGISTER) || (mp->reg_type == HOLDING_REGISTER)) {
            uint8_t bit_offset = mp->bit_offset;
            *value = (regs[0] & (1 << bit_offset)) ? 1 : 0;
        } else if ((mp->reg_type == COIL) || (mp->reg_type == DISCRETE_INPUT)) {
            *value = regs[0] ? 1 : 0;
        }

        *value -= mp->value_offset;
        break;
    }

    case TYPE_BYTE: {
        uint8_t rv = (regs[0] >> mp->bit_offset) & 0xFF;
        *value = (double)(rv - mp->value_offset);
        break;
    }

    case TYPE_INT16: {
        int16_t rv = regs[0];
        *value = ((double)rv - mp->value_offset);
        break;
    }

    case TYPE_UINT16: {
        uint16_t rv = regs[0];
        *value = ((double)rv - mp->value_offset);
        break;
    }

    case TYPE_UINT32_BE: {
        uint32_t rv = (regs[0] << 16) + regs[1];
        *value = ((double)rv - mp->value_offset);
        break;
    }

    case TYPE_UINT32_LE: {
        uint32_t rv = (regs[1] << 16) + regs[0];
        *value = ((double)rv - mp->value_offset);
        break;
    }

    case TYPE_INT32_BE: {
        int32_t rv = ((regs[0]) << 16) + regs[1];
        *value = ((double)rv - mp->value_offset);
        break;
    }

    case TYPE_INT32_LE: {
        int32_t rv = ((regs[1]) << 16) + regs[0];
        *value = ((double)rv - mp->value_offset);
        break;
    }

    case TYPE_FLOAT_BE: {
        uint32_t u32 = (regs[0] << 16) + regs[1];
        float rv = *((float *)&u32);
        *value = ((double)rv - mp->value_offset);
        break;
    }

    case TYPE_FLOAT_LE: {
        uint32_t u32 = (regs[1] << 16) + regs[0];
        float rv = *((float *)&u32);
        *value = ((double)rv - mp->value_offset);
        break;
    }

    case TYPE_INT64_BE: {
        int64_t rv = regs[0];
        rv = (rv << 16) + regs[1];
        rv = (rv << 16) + regs[2];
        rv = (rv << 16) + regs[3];
        *value = rv - mp->value_offset;
        break;
    }

    case TYPE_INT64_LE: {
        int64_t rv = regs[3];
        rv = (rv << 16) + regs[2];
        rv = (rv << 16) + regs[1];
        rv = (rv << 16) + regs[0];
        *value = rv - mp->value_offset;
        break;
    }

    default:
        return DEVICE_E_INVALID;
    }

    if (!is_double_equal(mp->scale, 1)) {
        *value /= mp->scale;
        // If value is -0, set to be 0
        if (is_double_equal(*value, -0))
            *value = 0;
    }

    return DEVICE_OK;
}


static err_code parse_point_definition(const char *ptr, int16_t len, data_point_t *p)
{
    if (!ptr || len <= 0 || !p) {
        return DEVICE_E_CONFIG;
    }

    char buf[MAX_FIELD_LENGTH + 1];

    modbus_point_t *mp = &(p->d.modbus);
    // set default value
    mp->value_offset = 0;
    mp->scale = 1;
    mp->bit_offset = 0;

    int16_t field_num = 0;

    for (int16_t b = 0, e = 0; b < len; b = e + 1, e = b) {
        while (e < len && ptr[e] != ':') {
            e++;
        }

        if (e - b > MAX_FIELD_LENGTH) {
            return DEVICE_E_CONFIG;
        }

        switch (field_num) {
        case MODBUS_SCHEMA_FIELD_KEY:
            strncpy_s(p->key, e - b + 1, ptr + b, e - b);
            break;

        case MODBUS_SCHEMA_FIELD_NUMBER: {
            strncpy_s(buf, sizeof(buf), ptr + b, e - b);
            errno = 0;
            uint32_t number = strtol(buf, NULL, 10);
            mp->reg_type = number / 100000;
            mp->addr = number % 100000 - 1; // number start from 1, but address is from 0

            if (errno || (mp->reg_type != 0 && mp->reg_type != 1 && mp->reg_type != 3 && mp->reg_type != 4)) {
                return DEVICE_E_CONFIG;
            };
            break;
        }

        case MODBUS_SCHEMA_FIELD_TYPE: {
            strncpy_s(buf, sizeof(buf), ptr + b, e - b);
            errno = 0;
            mp->data_type = strtol(buf, NULL, 10);

            if (errno || mp->data_type == TYPE_INVALID || mp->data_type > TYPE_INT64_LE) {
                return DEVICE_E_CONFIG;
            }
            break;
        }
        case MODBUS_SCHEMA_FIELD_BIT: {
            strncpy_s(buf, sizeof(buf), ptr + b, e - b);
            errno = 0;
            mp->bit_offset = strtol(buf, NULL, 10);

            if (errno || mp->bit_offset > 15) {
                return DEVICE_E_CONFIG;
            }
            break;
        }
        case MODBUS_SCHEMA_FIELD_MULTIPLIER: {
            strncpy_s(buf, sizeof(buf), ptr + b, e - b);
            errno = 0;
            double multiplier = strtod(buf, NULL);
            if (errno || multiplier == 0) {
                return DEVICE_E_CONFIG;
            }
            mp->scale = 1 / multiplier;
            break;
        }

        case MODBUS_SCHEMA_FIELD_OFFSET: {
            strncpy_s(buf, sizeof(buf), ptr + b, e - b);
            errno = 0;
            mp->value_offset = strtol(buf, NULL, 10);
            if (errno) {
                return DEVICE_E_CONFIG;
            }
            break;
        }
        }

        field_num++;
    }

    // minimum is key, number, type
    if (field_num < 3) {
        return DEVICE_E_CONFIG;;
    } else {
        return DEVICE_OK;
    }
}

// same type of point, address same (bit) or increasing
// allow overlap does not allow hole
static bool can_combine(modbus_point_t *current, modbus_point_t *next)
{
    // Check integer overflow
    uint32_t endAddr = current->addr + num_reg(current);

    return (next && (next->reg_type == current->reg_type) && (next->addr >= current->addr) &&
            (endAddr <= 0x00FFFF && next->addr <= endAddr));
}


static int find_modbus_point_index_by_key(data_schema_t *schema, const char *key)
{
    for (int i = 0; i < schema->num_point; i++) {
        if (strcmp(schema->points[i].key, key) == 0) {
            return i;
        }
    }
    return -1;
}


static bool is_register_value_in_buffer(modbus_point_t *mp, struct register_buffer_t *rbuf)
{
    return (mp->reg_type == rbuf->reg_type) && (mp->addr >= rbuf->begin_addr) && (mp->addr <= rbuf->end_addr);
}


static size_t calc_register_quantity_to_read(int mpi, data_schema_t *schema)
{
    modbus_point_t *mp = &schema->points[mpi].d.modbus;
    int quantity = 0;
    int max_quantity =
        (mp->reg_type == COIL || mp->reg_type == DISCRETE_INPUT) ? MODBUS_MAX_BIT_PER_READ : MODBUS_MAX_WORD_PER_READ;
    if (schema->flags & FLAG_NO_BATCH) {
        int j = mpi + 1;

        while ((j < schema->num_point) && can_combine(&schema->points[j - 1].d.modbus, &schema->points[j].d.modbus)) {
            j++;
        }

        modbus_point_t *mp_end = &schema->points[j - 1].d.modbus;
        quantity = MIN(max_quantity, (mp_end->addr + num_reg(mp_end) - mp->addr));
    } else {
        quantity = num_reg(mp);

        for (int j = mpi + 1; j < schema->num_point; j++) {
            modbus_point_t *next = &schema->points[j].d.modbus;
            if (next->reg_type == mp->reg_type) {
                int nreg = next->addr - mp->addr + num_reg(next);
                if (nreg <= max_quantity) {
                    quantity = nreg;
                    continue;
                }
            }
            break;
        }
    }
    return quantity;
}

// ------------------------ public interface --------------------------------


// setup connection with device and verify communication of given channel (slave_id)
err_code modbus_open(void *instance, uint32_t unit_id, int32_t timeout)
{
    modbus_device_t *self = (modbus_device_t*)instance;
    if (self->opened) {
        return DEVICE_OK;
    }

    struct timespec poll_sw;
    timer_stopwatch_start(&poll_sw);

    if (self->transport->transport_open(self->transport, timeout) != 0) {
        return DEVICE_E_IO;
    }

    int32_t elapse_ms = timer_stopwatch_stop(&poll_sw);

    if (elapse_ms >= timeout) {
        return DEVICE_E_TIMEOUT;
    }

    self->opened = true;
    return DEVICE_OK;
}


err_code modbus_close(void *instance)
{
    modbus_device_t *self = (modbus_device_t*)instance;
    ASSERT(self);

    LOGD("modbus_close");

    if (!self->opened) {
        self->opened = false;
        self->transport->transport_close(self->transport);
    }

    return DEVICE_OK;
}


err_code modbus_get_point(void *instance, uint32_t unit_id, const char *key,
                          data_schema_t *schema, telemetry_t *telemetry, int32_t timeout)
{
    modbus_device_t *self = (modbus_device_t*)instance;

    if (!self->opened) {
        return DEVICE_E_BROKEN;
    }

    int index = find_modbus_point_index_by_key(schema, key);

    if (index < 0) {
        LOGE("Can't read invalid data point %s", key);
        return DEVICE_E_INVALID;
    }
    modbus_point_t *mp = &schema->points[index].d.modbus;

    uint16_t regs[2];
    memset(regs, 0, sizeof(regs));
    err_code err =
        mb_read_register(self, unit_id, mp->reg_type, mp->addr + schema->offset, num_reg(mp), regs, timeout);

    if (err) {
        LOGW("Failed to read point '%s':%s:%d", schema->points[index].key, REG_NAMES[mp->reg_type], mp->addr);
        return err;
    }

    double new_value = NAN;
    err = decode_point(self, mp, regs, &new_value);

    if (err) {
        LOGW("Failed to decode data point %s", schema->points[index].key);
        return err;
    }

    clear_mask(telemetry->str_mask, index);
    if (is_double_equal(telemetry->values[index].num, new_value)) {
        clear_mask(telemetry->cov_mask, index);
    } else {
        telemetry->values[index].num = new_value;
        set_mask(telemetry->cov_mask, index);
    }

    return DEVICE_OK;
}


err_code modbus_get_point_list_internal(void *instance, uint32_t unit_id,
                               data_schema_t *schema, telemetry_t *telemetry, int32_t timeout)
{
    modbus_device_t *self = (modbus_device_t*)instance;

    if (!self->opened) {
        return DEVICE_E_BROKEN;
    }

    struct register_buffer_t rbuf = {.reg_type = INVALID};
    struct timespec poll_sw;
    timer_stopwatch_start(&poll_sw);

    for (int i = 0; i < schema->num_point; i++) {
        modbus_point_t *mp = &schema->points[i].d.modbus;
        int32_t elapse_ms = timer_stopwatch_stop(&poll_sw);

        if (elapse_ms >= timeout) {
            return DEVICE_E_TIMEOUT;
        }

        if (!is_register_value_in_buffer(mp, &rbuf)) {
            int quantity = calc_register_quantity_to_read(i, schema);
            LOGV("Read [%s:%d+%d]", REG_NAMES[mp->reg_type], mp->addr, quantity);

            err_code err = mb_read_register(self, unit_id, mp->reg_type, mp->addr + schema->offset, quantity, rbuf.buf,
                                            timeout - elapse_ms);
            if (err) {
                LOGE("Failed to read registers: %s", err_str(err));
                return err;
            }
            rbuf.begin_addr = mp->addr;
            rbuf.end_addr = mp->addr + quantity;
            rbuf.reg_type = mp->reg_type;
        }

        double new_value = NAN;

        if (decode_point(self, mp, &rbuf.buf[mp->addr - rbuf.begin_addr], &new_value) != 0) {
            LOGW("Failed to decode data point %s", schema->points[i].key);
            return DEVICE_E_PROTOCOL;
        }

        set_telemetry_number_value(telemetry, i, new_value);

        LOGV("%s=%.2f", schema->points[i].key, new_value);
    }

    return DEVICE_OK;
}


err_code modbus_get_point_list(void *instance, uint32_t unit_id,
                               data_schema_t *schema, telemetry_t *telemetry, int32_t timeout)
{
    modbus_device_t *self = (modbus_device_t*)instance;

    if (!self->opened) {
        return DEVICE_E_BROKEN;
    }

    return modbus_get_point_list_internal(instance, unit_id, schema, telemetry, timeout);
}


err_code modbus_set_point(void *instance, uint32_t unit_id, const char *key, const char *value,
                          data_schema_t *schema, int32_t timeout)
{
    modbus_device_t *self = (modbus_device_t*)instance;

    if (!self->opened) {
        return DEVICE_E_BROKEN;
    }

    int index = find_modbus_point_index_by_key(schema, key);

    if (index < 0) {
        LOGE("Can't write invalid data point");
        return DEVICE_E_INVALID;
    }

    modbus_point_t *mp = &schema->points[index].d.modbus;

    if (mp->reg_type != COIL && mp->reg_type != HOLDING_REGISTER) {
        LOGE("Can't write invalid data point %s", key);
        return DEVICE_E_INVALID;
    }

    uint16_t regs[4] = {0, 0, 0, 0};
    err_code err = encode_point(self, mp, value, regs);

    if (err) {
        LOGE("Failed to encode point %s=%s:%s", key, value, err_str(err));
        return err;
    }

    err = mb_write_register(self, unit_id, mp->reg_type, mp->addr + schema->offset, num_reg(mp), regs, timeout);

    if (err) {
        LOGE("Failed to write data %s=%s", key, value, err_str(err));
        return err;
    }

    return DEVICE_OK;
}

device_protocol_t modbus_get_protocol(void *instance)
{
    return ((modbus_device_t*)instance)->protocol;
}


err_code modbus_create_point_table(struct json_token *points_def, int *npoints, data_point_t **ppoints)
{
    if (!points_def || !points_def->ptr || points_def->len == 0) {
        LOGW("schema has no points defintion");
        *npoints = 0;
        *ppoints = NULL;
        return DEVICE_OK;
    }

    const char *ptr = points_def->ptr;
    int len = points_def->len;
    data_point_t *points = NULL;
    int num_point = 0;
    size_t total_key_len = 0;

    for (int b = 0, e = 0; b < len; b = e + 1, e = b) {
        while (e < len && ptr[e] != ':') {
            e++;
        }
        total_key_len += e - b;

        while (e < len && ptr[e] != ',') {
            e++;
        }
        num_point++;
    }

    points = CALLOC(num_point, sizeof(data_point_t));
        // All keys been put in a single memory block to avoid fragment
    char *keys = MALLOC(total_key_len + num_point);

    for (int b = 0, e = 0, i=0; b < len && i< num_point; b = e + 1, e = b, i++) {
        while (e < len && ptr[e] != ',') {
            e++;
        }

        data_point_t *p = &points[i];
        p->key = keys;
        if (parse_point_definition(ptr + b, e - b, p) != DEVICE_OK) {
            LOGE("Failed to parse definition: [%.*s]", e-b, ptr+b);
            modbus_destroy_point_table(points, num_point);
            return DEVICE_E_CONFIG;
        }

        keys += strlen(p->key) + 1;  // make sure to include null terminate
    }

    *npoints = num_point;
    *ppoints = points;
    return DEVICE_OK;
}


void modbus_destroy_point_table(data_point_t *points, int npoint)
{
    ASSERT(points);
   // All keys been put in a single memory block to avoid fragment
    FREE(points[0].key);
    FREE(points);
}



// modbus connection string will be in two format
// for modbus tcp, "unitid,<ip>""
// for modbus rtu, "unitid,<uart config>""
device_driver_t *modbus_create_driver(device_protocol_t protocol, const char *conn_str)
{
    // modbus rtu require conn_str as uart_config
    // modbus tcp require conn_str as ip:port
    if (!conn_str) {
        return NULL;
    }

    modbus_device_t *modbus = (modbus_device_t *)CALLOC(1, sizeof(modbus_device_t));

    modbus->base.driver_open = modbus_open;
    modbus->base.driver_close = modbus_close;
    modbus->base.get_point = modbus_get_point;
    modbus->base.get_point_list = modbus_get_point_list;
    modbus->base.set_point = modbus_set_point;
    modbus->base.get_protocol = modbus_get_protocol;
    modbus->protocol = protocol;

    modbus->opened = false;

    modbus->transport = modbus_create_transport(protocol, conn_str);

    if (!modbus->transport) {
        LOGE("Failed to create transport");
        modbus_destroy_driver((device_driver_t*)modbus);
        return NULL;
    }

    LOGI("Created modbus driver: protocol=%s, connection=%s", protocol2str(protocol), conn_str);

    return (device_driver_t*)modbus;
}


void modbus_destroy_driver(device_driver_t *instance)
{
    modbus_device_t *self = (modbus_device_t*)instance;
    ASSERT(self);

    LOGI("Destroy modbus driver");

    if (self->transport) {
        modbus_destroy_transport(self->protocol, self->transport);
    }
    FREE(self);
}
