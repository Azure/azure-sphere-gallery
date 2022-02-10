/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <applibs/gpio.h>
#include <applibs/application.h>
#include <math.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <init/device_hal.h>
#include <init/globals.h>
#include <init/ipc.h>
#include <iot/diag.h>
#include <utils/llog.h>
#include <utils/timer.h>
#include <utils/uart.h>
#include <utils/utils.h>
#include <driver/modbus.h>
#include <safeclib/safe_lib.h>

#include "modbus_transport.h"
#include "modbus_transport_rtu.h"

// Note: Modbus RTU only support one outgoing transcation, so everything
// is serialized, for Sphere to talking to multiple modbus rtu device (e.g. two
// device A, B, the sequence will be
// req1A, resp1A, req2A, resp2A, req1B, resp1B, req2B, resp2B.

typedef struct modbus_transport_rtu_t modbus_transport_rtu_t;
struct modbus_transport_rtu_t {
    modbus_transport_t base; // must be first
    int rtcore_socket_fd;
    int uart_fd;
    int uart_port;
    int uart_tx_enable_fd;
    int t35_ms;
    int t35_adjust_times;
    UART_Config uart_config;
};

static int rtu_ensure_idle(modbus_transport_rtu_t* ctx, int32_t timeout)
{
    struct timespec poll_sw;
    timer_stopwatch_start(&poll_sw);

    struct pollfd fds[1];
    fds[0].fd = ctx->rtcore_socket_fd;
    fds[0].events = POLLIN;

    bool quit = false;
    uint8_t garbage[MB_RTU_MAX_ADU_SIZE];

    while (!quit) {
        int32_t elapse_ms = timer_stopwatch_stop(&poll_sw);

        if (elapse_ms >= timeout) {
            quit = true;
            continue;
        }

        int nevents = poll(fds, 1, ctx->t35_ms);
        if (nevents == 0) {
            // no data on rtu
            return 0;
        } else if ((nevents == 1) && (fds[0].revents & POLLIN)) {
            // Read garbage data in uart buffer up to one frame of 256 bytes.
            int count = UART_read(ctx->uart_fd, garbage, MB_RTU_MAX_ADU_SIZE);
            LOGD("Consume garbage data: read %d byes of garbage on RTU", count);
            LOGD("Garbage data <--%s", hex(garbage, count));
        } else {
            LOGE("Uart poll error in rtu_ensure_idle");
            quit = true;
        }
    }

    return -1;
}

static uint16_t crc16(const uint8_t *buffer, int32_t len)
{
    uint16_t temp, flag;
    temp = 0xFFFF;
    for (int32_t i = 0; i < len; i++) {
        temp = temp ^ buffer[i];

        for (int j = 1; j <= 8; j++) {
            flag = temp & 0x0001;
            temp >>= 1;

            if (flag) {
                temp ^= 0xA001;
            }
        }
    }

    return temp;
}

/// <summary>
/// try to write RTU frame over uart connection, timeout in MB_RTU_WRITE_TIMEOUT_MS
/// </summary>
/// <param name="ctx">RTU transportion instance</param>
/// <param name="buf">buffer to send</param>
/// <param name="count">count of bytes to send</param>
/// <param name="timeout">the value of timer in ms for this operation</param>
/// <returns>error code</returns>
static err_code rtu_write_frame(modbus_transport_rtu_t *ctx, uint8_t *buf, int32_t count, int32_t timeout)
{
    err_code result = ipc_execute_command(ctx->rtcore_socket_fd, IPC_WRITE_UART, buf, count);
    if (DEVICE_OK != result) {
        LOGE("ERROR: Could not write bytes to UART on the real-time core with erro code: %d", result);
    }

    return result;
}

/// <summary>
/// try to find pdu len from first two bytes of pdu
/// </summary>
/// <param name="pdu">pdu</param>
/// <returns>pdu length or 0 if unknown</returns>
static int32_t modbus_find_pdu_len(uint8_t *pdu)
{
    // it's error response, pdu is two bytes
    if (pdu[0] & 0x80) {
        return 2;
    }

    int32_t length = 0;

    switch (pdu[0]) {
    case FC_READ_COILS:
    case FC_READ_DISCRETE_INPUTS:
    case FC_READ_HOLDING_REGISTERS:
    case FC_READ_INPUT_REGISTERS:
        // 1 byte function code + 1 byte byte count + n bytes values
        length = 2 + pdu[1];
        break;

    case FC_WRITE_COILS:
    case FC_WRITE_HOLDING_REGISTERS:
    case FC_WRITE_SINGLE_COIL:
    case FC_WRITE_SINGLE_REGISTER:
        // 1 byte function code + 2 bytes start addr, 2 byte quantity
        length = 5;
        break;

    case FC_READ_EXCEPTION_STATUS:
        // 1 byte function code + 1 byte output data
        length = 2;
        break;

    case FC_DIAGNOSTICS:
        // 1 byte function code, 2 byte sub-function , 2 bytes data
        length = 5;
        break;

    case FC_GET_COMM_EVENT_COUNTER:
        // 1 byte function code, 2 byte status, 2 bytes event count
        length = 5;
        break;

    case FC_GET_COMM_EVENT_LOG:
        length = 2 + pdu[1];
        break;

    case FC_REPORT_SERVER_ID:
        length = 5;
        break;

    case FC_READ_FILE_RECORD:
    case FC_WRITE_FILE_RECORD:
        length = 2 + pdu[1];
        break;

    case FC_MASK_WRITE_REGISTER:
        length = 7;
        break;

    case FC_READ_WRITE_REGISTERS:
        length = 2 + pdu[1];
        break;

    case FC_READ_FIFO_QUEUE:
        length = 2 + pdu[1];
        break;

    case FC_MEI:
        // FC_MEI is not supported now as the pdu lenth cann't be found
        // in pdu and not fixed.
    default:
        length = -1;
        break;
    }

    return length;
}

/// <summary>
/// try to receied count bytes over uart connection, timeout in MB_RTU_IO_TIMEOUT_MS
/// </summary>
/// <param name="ctx">RTU transportion instance</param>
/// <param name="buf">buffer to receive data</param>
/// <param name="count">count of bytes to receive</param>
/// <param name="timeout">the value of timer in ms for this operation</param>
/// <returns>error code</returns>
static err_code rtu_read_bytes(modbus_transport_rtu_t *ctx, uint8_t *buf, int32_t length, int32_t *count, int32_t timeout)
{
    err_code err = DEVICE_OK;
    int total = 0;
    struct timespec poll_sw;
    struct pollfd fds[1];
    fds[0].fd = ctx->rtcore_socket_fd;
    fds[0].events = POLLIN;
    timer_stopwatch_start(&poll_sw);

    while (total < *count) {
        int32_t elapse_ms = timer_stopwatch_stop(&poll_sw);

        if (elapse_ms >= timeout) {
            err = DEVICE_E_TIMEOUT;
            break;
        }

        int nevents = poll(fds, 1, timeout - elapse_ms);

        if (nevents < 0) {
            LOGE("uart poll in error");
            err = DEVICE_E_IO;
            break;
        } else if (nevents == 0) {
            err = DEVICE_E_TIMEOUT;
            break;
        } else {
            if (fds[0].revents & POLLHUP) {
                LOGE("uart connection broken");
                err = DEVICE_E_BROKEN;
                break;
            } else if (fds[0].revents & POLLERR) {
                LOGE("uart poll error");
                err = DEVICE_E_IO;
                break;
            } else if (fds[0].revents & POLLIN) {
                int nread = recv(fds[0].fd, buf + total, length - total, 0);

                if (nread < 0) {
                    LOGE("uart read error");
                    err = DEVICE_E_IO;
                    break;
                }

                total += nread;
            }
        }
    }

    *count = total;
    return err;
}

/// <summary>
/// try to receie one RTU frame over uart connection
/// </summary>
/// <param name="ctx">RTU transportion instance</param>
/// <param name="buf">buffer to receive one frame of MB_RTU_MAX_ADU_SIZE bytes</param>
/// <param name="fbytes">bytes of received frame</param>
/// <param name="timeout">the value of timer in ms for this operation</param>
/// <returns>error code</returns>
static err_code rtu_read_frame(modbus_transport_rtu_t *ctx, uint8_t *buf, int32_t length, int32_t *frame_bytes, int32_t timeout)
{
    // Find pdu len is tricky as we need to parse pdu to figure out
    // read first three bytes of adu
    struct timespec poll_sw;
    timer_stopwatch_start(&poll_sw);
    int32_t received_bytes = MB_RTU_HEADER_SIZE;
    err_code err = rtu_read_bytes(ctx, buf, length, &received_bytes, timeout);

    if (err) {
        return err;
    }

    LOGD("PDU header <--%s", hex(buf, MB_RTU_HEADER_SIZE));
    int32_t pdu_len = modbus_find_pdu_len(buf + 1);

    if ((pdu_len <= 0) || (pdu_len > MB_RTU_MAX_ADU_SIZE - 3)) {
        LOGE("Invalid pdu len %d", pdu_len);
        return DEVICE_E_PROTOCOL;
    }

    // 1 byte slave_id + pdu + 2 bytes crc
    *frame_bytes = 1 + pdu_len + 2;
    int32_t elapse_ms = timer_stopwatch_stop(&poll_sw);

    if (elapse_ms >= timeout) {
        return DEVICE_E_TIMEOUT;
    }

    if (*frame_bytes == received_bytes) {
        return DEVICE_OK;
    } else {
        pdu_len = *frame_bytes - received_bytes;
        return rtu_read_bytes(ctx, buf + received_bytes, length - received_bytes, &pdu_len, timeout - elapse_ms);
    }
}

/// <summary>
/// Adjust t35 timer if fail to get modbus response as time out.
/// </summary>
/// <param name="ctx">RTU transportion instance</param>
/// <param name="code">error code</param>
static void rtu_adjust_t35(modbus_transport_rtu_t* ctx, err_code code)
{
    // only adjust t35 if time out
    if (DEVICE_E_TIMEOUT == code && ctx->t35_adjust_times > 0) {
        ctx->t35_ms += MODBUS_T35_ADJUST_STEP;
        ctx->t35_adjust_times -= 1;

        LOGD("Try T3.5=%dms for next request", ctx->t35_ms);
    }
}

// ------------------------ public interface --------------------------------

/// <summary>
/// open uart connection
/// </summary>
err_code rtu_open(modbus_transport_t *instance, int32_t timeout_ms)
{
    modbus_transport_rtu_t *ctx = (modbus_transport_rtu_t *)instance;
    // don't try to open again
    if (ctx->rtcore_socket_fd >= 0) {
        LOGW("rtore socket already opened");
        return DEVICE_OK;
    }

    LOGD("IPC open");
    ctx->rtcore_socket_fd = Application_Connect(RT_APP_COMPONENT_ID);
    if (ctx->rtcore_socket_fd == -1) {
        LOGW("ERROR: Unable to create socket: %d (%s)\n", errno, strerror(errno));
        return DEVICE_E_IO;
    }

    // Set timeout, to handle case where real-time capable application does not respond.
    struct timeval rt_timeout = {.tv_sec = 0, .tv_usec = timeout_ms * 1000 / 4};
    setsockopt(ctx->rtcore_socket_fd, SOL_SOCKET, SO_SNDTIMEO, &rt_timeout, sizeof(rt_timeout));
    setsockopt(ctx->rtcore_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &rt_timeout, sizeof(rt_timeout));

    uint8_t data[6];
    int32_t size = sizeof(data);
    // serialize uart baudrate, parity and stopbits into message data.
    serialize_uint32(data, ctx->uart_config.baudRate);
    data[4] = ctx->uart_config.parity;
    data[5] = ctx->uart_config.stopBits;
    err_code result = ipc_execute_command(ctx->rtcore_socket_fd, IPC_OPEN_UART, data, size);
    if (DEVICE_OK != result) {
        LOGE("ERROR: Could not open UART on the real-time core with error code: %d", result);
    }

    return result;
}

/// <summary>
/// close uart connection
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
err_code rtu_close(modbus_transport_t *instance)
{
    LOGD("rtu close");

    modbus_transport_rtu_t *ctx = (modbus_transport_rtu_t *)instance;

    if (ctx->uart_fd >= 0) {
        UART_close(ctx->uart_fd);
        ctx->uart_fd = -1;
    }

    if (ctx->rtcore_socket_fd >= 0) {
        ipc_execute_command(ctx->rtcore_socket_fd, IPC_CLOSE_UART, NULL, 0);
        close(ctx->rtcore_socket_fd);
        ctx->rtcore_socket_fd = -1;
    }

    return DEVICE_OK;
}

/// <summary>
/// send request pdu
/// </summary>
/// <param name="pdu_len">pdu length to send</param>
/// <returns>error code</returns>
err_code rtu_send_request(modbus_transport_t *instance, uint8_t slave_id, const uint8_t *pdu, int32_t pdu_len,
                          int32_t timeout)
{
    modbus_transport_rtu_t *ctx = (modbus_transport_rtu_t *)instance;

    uint8_t adu[MB_RTU_MAX_ADU_SIZE];
    adu[0] = slave_id;
    memcpy_s(adu + 1, MB_RTU_MAX_ADU_SIZE - 1, pdu, pdu_len);

    // crc for the entrie adu
    uint16_t crc = crc16(adu, pdu_len + 1);
    adu[1 + pdu_len] = crc & 0xFF;
    adu[1 + pdu_len + 1] = (crc >> 8) & 0xFF;

    int32_t adu_len = pdu_len + 3; // 1 byte slave id + pdu + 2 bytes crc
    struct timespec poll_sw;
    timer_stopwatch_start(&poll_sw);

    if (rtu_ensure_idle(ctx, timeout) != 0) {
        return DEVICE_E_BUSY;
    }

    int32_t elapse_ms = timer_stopwatch_stop(&poll_sw);

    err_code err = rtu_write_frame(ctx, adu, adu_len, timeout - elapse_ms);

    if (err) {
        LOGE("Failed to write request:%s", err_str(err));
        return err;
    }

    LOGV("ADU-->%s", hex(adu, adu_len));
    return DEVICE_OK;
}


/// <summary>
/// recv response for previously sent request
/// </summary>
/// <param name="pdu">pdu buffer</param>
/// <param name="ppdu_len">pointer to variable to hold pdu len</param>
/// <param name="timeout">the value of timer in ms for this operation</param>
/// <returns>error code</returns>
err_code rtu_recv_response(modbus_transport_t *instance, uint8_t slave_id, uint8_t *pdu, int32_t *ppdu_len,
                           int32_t timeout)
{
    modbus_transport_rtu_t *ctx = (modbus_transport_rtu_t *)instance;
    uint8_t adu[MB_RTU_MAX_ADU_SIZE];

    int32_t adu_len = 0;
    // adu must have enough space to receive MB_RTU_MAX_ADU_SIZE bytes
    int err = rtu_read_frame(ctx, adu, MB_RTU_MAX_ADU_SIZE, &adu_len, timeout);

    diag_log_value(MODBUS_T35_DATAPOINT, ctx->t35_ms);
    if (err != 0) {
        LOGE("Failed to read adu:%s", err_str(err));
        rtu_adjust_t35(ctx, err);
        return err;
    }

    LOGV("ADU<--%s", hex(adu, adu_len));

    if (adu[0] != slave_id) {
        LOGE("Discard unexpected frame from slave %d, expected %d", adu[0], slave_id);
        return DEVICE_E_PROTOCOL;
    }

    uint16_t crc1 = (adu[adu_len - 1] << 8) + adu[adu_len - 2];
    uint16_t crc2 = crc16(adu, adu_len - 2);

    if (crc1 != crc2) {
        LOGE("CRC error, recv=%x calc=%x", crc1, crc2);
        return DEVICE_E_PROTOCOL;
    }

    // 1 byte slave_id + pdu + 2 bytes crc
    int32_t pdu_len = adu_len - 3;
    // Assume that the caller passes in the buffer with size of MODBUS_MAX_PDU_SIZE
    memcpy_s(pdu, MODBUS_MAX_PDU_SIZE, adu + 1, pdu_len);
    *ppdu_len = pdu_len;
    return DEVICE_OK;
}


/// <summary>
/// destroy rtu transport instance when no more reference
/// </summary>
/// <param name="id"></param>
/// <returns>0 on success, or -1 on failure</returns>
void modbus_transport_rtu_destroy(modbus_transport_t *instance)
{
    ASSERT(instance);

    LOGD("rtu destory");
    rtu_close(instance);
    FREE(instance);
}


/// <summary>
/// create modbus rtu transport instance, if connection configuration
/// is the same, then reuse old connection, increase reference count
/// </summary>
/// <param name="connection">connection config json value</param>
/// <param name="ptransport">pointer to variable hold transportion instance</param>
/// <returns>0 on success, or error code</returns>
modbus_transport_t *modbus_transport_rtu_create(const char *conn_str)
{
    ASSERT(conn_str);

    UART_Config config;

    if (parse_uart_config_string(conn_str, &(config)) != 0) {
        LOGE("modbus rtu invalid uart config");
        return NULL;
    }

    LOGD("rtu create");
    modbus_transport_rtu_t *rtu = (modbus_transport_rtu_t *)CALLOC(1, sizeof(modbus_transport_rtu_t));
    rtu->base.transport_open = rtu_open;
    rtu->base.transport_close = rtu_close;
    rtu->base.send_request = rtu_send_request;
    rtu->base.recv_response = rtu_recv_response;

    rtu->uart_port = BOARD_UART;
    memcpy_s(&(rtu->uart_config), sizeof(UART_Config), &(config), sizeof(UART_Config));
    rtu->uart_fd = -1;
    rtu->rtcore_socket_fd = -1;
    rtu->uart_tx_enable_fd = -1;

    if (rtu->uart_config.baudRate >= 19200) {
        rtu->t35_ms = MODBUS_T35_MS;
    } else {
        // 1 start bit + dataBits + parity + stopBits
        uint32_t bits_per_byte = 1 + rtu->uart_config.dataBits
            + (rtu->uart_config.parity == UART_Parity_None ? 0 : 1)
            + rtu->uart_config.stopBits;

        float bytes_per_second = (float)(rtu->uart_config.baudRate) / bits_per_byte;
        rtu->t35_ms = ceil(1000 * 3.5 / bytes_per_second);
    }
    rtu->t35_adjust_times = MODBUS_T35_MAXIMUM_RETRY;

    return (modbus_transport_t *)rtu;
}
