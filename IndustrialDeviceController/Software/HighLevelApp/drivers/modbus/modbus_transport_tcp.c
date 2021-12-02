/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>


#include <init/applibs_versions.h>
#include <applibs/networking.h>

#include <init/device_hal.h>
#include <init/globals.h>
#include <utils/llog.h>
#include <utils/timer.h>
#include <utils/utils.h>
#include <driver/modbus.h>
#include <safeclib/safe_lib.h>

#include "modbus_transport.h"
#include "modbus_transport_tcp.h"

// NOTE: Modbus tcp support simulataneous transcations and transcation id
// must been used.
// Current implementation assume only one outgoing transcation. Possible improvement
// is to send multiple requests out and then wait them all to be completed.

#define MB_TCP_MAX_ADU_SIZE 260

// header not include unit_id
#define MBAP_HEADER_SIZE 7

typedef struct modbus_transport_tcp_t modbus_transport_tcp_t;
struct modbus_transport_tcp_t {
    modbus_transport_t base;
    int sock_fd;
    int port;
    char ip[16]; // only support ipv4
    uint16_t transcation_id;
};


/// <summary>
/// try to send count bytes over tcp connection, timeout in MB_TCP_IO_TIMEOUT_MS
/// </summary>
/// <param name="sockfd">socket fd</param>
/// <param name="buf">buffer to send</param>
/// <param name="count">count of bytes to send</param>
/// <param name="timeout">the value of timer in ms for this operation</param>
/// <returns>error code</returns>
static err_code tcp_send_bytes(int sockfd, uint8_t *buf, int count, int timeout)
{
    int32_t total = 0;
    struct timespec poll_sw;
    timer_stopwatch_start(&poll_sw);

    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLOUT;

    while (total < count) {
        int32_t elapse_ms = timer_stopwatch_stop(&poll_sw);

        if (elapse_ms >= timeout) {
            return DEVICE_E_TIMEOUT;
        }

        int nevents = poll(fds, 1, timeout - elapse_ms);

        if (nevents < 0) {
            LOGE("socket I/O error");
            return DEVICE_E_IO;
        } else if (nevents == 0) {
            LOGE("socket sending timeout");
            return DEVICE_E_TIMEOUT;
        } else {
            if (fds[0].revents & POLLHUP) {
                LOGE("socket connection broken");
                return DEVICE_E_BROKEN;
            } else if (fds[0].revents & POLLERR) {
                LOGE("socket poll error");
                return DEVICE_E_IO;
            } else if (fds[0].revents & POLLOUT) {
                int nsend = send(sockfd, buf + total, count - total, MSG_NOSIGNAL);
                if (nsend <= 0) {
                    LOGE("socket I/O error");
                    return DEVICE_E_BROKEN;
                }
                total += nsend;
            }
        }
    }

    return DEVICE_OK;
}


/// <summary>
/// try to receied count bytes over tcp connection, timeout in MB_TCP_IO_TIMEOUT_MS
/// </summary>
/// <param name="sockfd">socket fd</param>
/// <param name="buf">buffer to receive data</param>
/// <param name="count">number of bytes to receive</param>
/// <param name="timeout">the value of timer in ms for this operation</param>
/// <returns>error code</returns>
static err_code tcp_recv_bytes(int sockfd, uint8_t *buf, int count, int timeout)
{
    int total = 0;
    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    struct timespec poll_sw;
    timer_stopwatch_start(&poll_sw);

    while (total < count) {
        int32_t elapse_ms = timer_stopwatch_stop(&poll_sw);

        if (elapse_ms >= timeout) {
            return DEVICE_E_TIMEOUT;
        }

        int nevents = poll(fds, 1, timeout - elapse_ms);

        if (nevents < 0) {
            LOGE("socket I/O error");
            return DEVICE_E_IO;
        } else if (nevents == 0) {
            LOGE("socket receiving timeout");
            return DEVICE_E_TIMEOUT;
        } else {
            if (fds[0].revents & POLLHUP) {
                LOGE("socket connection broken");
                return DEVICE_E_BROKEN;
            } else if (fds[0].revents & POLLERR) {
                LOGE("socket poll error");
                return DEVICE_E_IO;
            } else if (fds[0].revents & (POLLIN | POLLPRI)) {
                int nrecv = recv(sockfd, buf + total, count - total, MSG_DONTWAIT);
                if (nrecv <= 0) {
                    LOGE("socket I/O error");
                    return DEVICE_E_BROKEN;
                }
                total += nrecv;
            }
        }
    }

    return DEVICE_OK;
}


/// <summary>
/// open tcp connection
/// </summary>
err_code tcp_open(modbus_transport_t *instance, int32_t timeout_ms)
{
    modbus_transport_tcp_t *ctx = (modbus_transport_tcp_t *)instance;
    LOGD("tcp_open %s:%d", ctx->ip, ctx->port);

    // assume ipv4, create a tcp socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        LOGE("Failed to open socket");
        return DEVICE_E_IO;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ctx->ip);
    server.sin_port = htons(ctx->port);

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = timeout_ms % 1000 * 1000;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) != 0) {
        LOGE("Failed to connect to %s:%d", ctx->ip, ctx->port);
        close(sock);
        return DEVICE_E_IO;
    }

    LOGD("Connected to modbus server %s:%d", ctx->ip, ctx->port);

    ctx->sock_fd = sock;
    return DEVICE_OK;
}

/// <summary>
/// close tcp connection
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
err_code tcp_close(modbus_transport_t *instance)
{
    modbus_transport_tcp_t *ctx = (modbus_transport_tcp_t *)instance;
    
    if (ctx->sock_fd >= 0) {
        close(ctx->sock_fd);
        ctx->sock_fd = -1;
    }

    return DEVICE_OK;
}

/// <summary>
/// prepare and send modbus tcp adu
/// </summary>
/// <param name="pdu">pdu to send</param>
/// <param name="pdu_len">pdu length to send</param>
/// <param name="timeout">the value of timer in ms for this operation</param>
/// <returns>error code</returns>
err_code tcp_send_request(modbus_transport_t *instance, uint8_t unit_id, const uint8_t *pdu, int32_t pdu_len,
                          int32_t timeout)
{
    modbus_transport_tcp_t *ctx = (modbus_transport_tcp_t *)instance;
    uint8_t adu[MB_TCP_MAX_ADU_SIZE];

    if (ctx->sock_fd < 0) {
        LOGE("Socket not open yet");
        return DEVICE_E_INTERNAL;
    }

    ctx->transcation_id++;
    adu[0] = (ctx->transcation_id >> 8) & 0xFF;
    adu[1] = ctx->transcation_id & 0xFF;
    adu[2] = 0;           // protocol id hi
    adu[3] = 0;           // protocol id lo
    adu[4] = 0;           // length hi - always 0 as max is 254
    adu[5] = 1 + pdu_len; // one byte unit_id + pdu length
    adu[6] = unit_id;
    memcpy_s(adu + MBAP_HEADER_SIZE, MB_TCP_MAX_ADU_SIZE - MBAP_HEADER_SIZE, pdu, pdu_len);

    int32_t adu_len = MBAP_HEADER_SIZE + pdu_len;

    int err = tcp_send_bytes(ctx->sock_fd, adu, adu_len, timeout);

    if (err) {
        LOGE("Failed to send request:%s", err_str(err));
        return err;
    }

#ifdef DEBUG_TRAFFIC
    LOGD("ADU --> %s", hex(adu, adu_len));
#endif
    return DEVICE_OK;
}

/// <summary>
/// recv response for previously sent request
/// </summary>
/// <param name="pdu">pdu buffer</param>
/// <param name="ppdu_len">pointer to variable that hold pdu len</param>
/// <param name="timeout">the value of timer in ms for this operation</param>
/// <returns>error code</returns>
err_code tcp_recv_response(modbus_transport_t *instance, uint8_t unit_id, uint8_t *pdu, int32_t *ppdu_len,
                           int32_t timeout)
{
    modbus_transport_tcp_t *ctx = (modbus_transport_tcp_t *)instance;

    uint8_t adu[MB_TCP_MAX_ADU_SIZE];
    uint16_t transcation_id = 0;
    int32_t pdu_len = 0;

    if (ctx->sock_fd < 0) {
        LOGE("Socket not open yet");
        return DEVICE_E_INTERNAL;
    }

    struct timespec poll_sw;
    timer_stopwatch_start(&poll_sw);

    do {
        // receive MBAP header first
        int32_t elapse_ms = timer_stopwatch_stop(&poll_sw);

        if (elapse_ms >= timeout) {
            return DEVICE_E_TIMEOUT;
        }
        err_code err = tcp_recv_bytes(ctx->sock_fd, adu, MBAP_HEADER_SIZE, timeout - elapse_ms);

        if (err) {
            LOGE("Failed to receive MBAP header:%s", err_str(err));
            return err;
        }

        transcation_id = (adu[0] << 8) + adu[1];
        pdu_len = (adu[4] << 8) + adu[5] - 1; // exclude one byte unit id

        if ((pdu_len < 0) || (pdu_len > MB_TCP_MAX_ADU_SIZE - MBAP_HEADER_SIZE)) {
            LOGE("Invalid pdu len %d", pdu_len);
            // clean any garbage data then bail out
            uint8_t garbage;

            do {
                elapse_ms = timer_stopwatch_stop(&poll_sw);

                if (elapse_ms >= timeout) {
                    return DEVICE_E_TIMEOUT;
                }
            } while (tcp_recv_bytes(ctx->sock_fd, &garbage, 1, timeout - elapse_ms) == 0);

            return DEVICE_E_PROTOCOL;
        }

        // receive the pdu
        elapse_ms = timer_stopwatch_stop(&poll_sw);

        if (elapse_ms >= timeout) {
            return DEVICE_E_TIMEOUT;
        }

        err = tcp_recv_bytes(ctx->sock_fd, adu + MBAP_HEADER_SIZE, pdu_len, timeout - elapse_ms);

        if (err) {
            LOGE("Failed to receive pdu:%s", err_str(err));
            return err;
        }

#ifdef DEBUG_TRAFFIC
        LOGD("ADU<-- %s", hex(adu, MBAP_HEADER_SIZE + pdu_len));
#endif
        // the response could be delayed, so wait until we received response
        // for current transcation
    } while (transcation_id < ctx->transcation_id);

    if (transcation_id != ctx->transcation_id) {
        LOGE("Expect response for transcation %d, got %d", ctx->transcation_id, transcation_id);
        return DEVICE_E_PROTOCOL;
    }

    if (unit_id != adu[6]) {
        LOGE("Expect unit_id %d, got %d", unit_id, adu[6]);
        return DEVICE_E_PROTOCOL;
    }

    // Assume that the caller passes in the buffer with size of MODBUS_MAX_PDU_SIZE
    memcpy_s(pdu, MODBUS_MAX_PDU_SIZE, adu + MBAP_HEADER_SIZE, pdu_len);
    *ppdu_len = pdu_len;
    return DEVICE_OK;
}


/// <summary>
/// destroy tcp transport instance, free resource
/// </summary>
void modbus_transport_tcp_destroy(modbus_transport_t *instance)
{
    modbus_transport_tcp_t *ctx = (modbus_transport_tcp_t *)instance;

    if (ctx->sock_fd >= 0) {
        tcp_close(instance);
    }

    FREE(ctx);
}

/// <summary>
/// create modbus tcp transportation instance.
/// connection string format is "ip:port", null terminated
/// </summary>
modbus_transport_t *modbus_transport_tcp_create(const char *conn_str)
{
    ASSERT(conn_str);
    
    modbus_transport_tcp_t *tcp = (modbus_transport_tcp_t *)CALLOC(1, sizeof(modbus_transport_tcp_t));

    tcp->base.transport_open = tcp_open;
    tcp->base.transport_close = tcp_close;
    tcp->base.send_request = tcp_send_request;
    tcp->base.recv_response = tcp_recv_response;

    char *colon = strchr(conn_str, ':');

    if (colon) {
        strncpy_s(tcp->ip, sizeof(tcp->ip), conn_str, colon - conn_str);
        tcp->port = strtol(colon + 1, NULL, 10);
    }

    if ((strlen(tcp->ip) == 0) || (!tcp->port)) {
        LOGE("Invalid connection string");
        modbus_transport_tcp_destroy((modbus_transport_t *)tcp);
        return NULL;
    }

    tcp->transcation_id = 0;
    tcp->sock_fd = -1;
    return (modbus_transport_t *)tcp;
}
