/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include <init/applibs_versions.h>
#include <applibs/log.h>
#include <applibs/uart.h>
#include <applibs/gpio.h>

#include <frozen/frozen.h>
#include <init/globals.h>
#include <iot/iot.h>
#include <utils/llog.h>
#include <utils/timer.h>
#include <utils/utils.h>

#define SERIAL_LOG_PORT BOARD_UART
#define SERIAL_LOG_BAUDRATE 115200

static const char LOG_TAGS[] = {'N', 'F', 'E', 'W', 'I', 'D', 'V'};

typedef struct log_entry_t log_entry_t;
struct log_entry_t {
    char *body;
    log_entry_t *next;
};

typedef struct log_chunk_t log_chunk_t;
struct log_chunk_t {
    log_entry_t *head;
    log_entry_t *tail;
    int len;
};

typedef struct log_t log_t;
struct log_t {
    pthread_mutex_t lock;
    int endpoint;
    int level;
    log_chunk_t chunks;
    int uart_fd;
    int tx_enable_fd;

};


static log_t s_log;


static int printf_logs(struct json_out *out, va_list *ap)
{
    int len = 0;
    struct log_entry_t *p = va_arg(*ap, struct log_entry_t *);
    int i = 0;
    while (p) {
        if (i > 0) {
            json_printf(out, ",");
        }
        len += json_printf(out, "%s", p->body);
        p = p->next;
        i++;
    }
    return len;
}


// add log entry into a fixed size list
static void llog_iothub(const char * fmt, va_list args)
{
    char message[DIAG_MAX_LOG_SIZE];

    if (vsnprintf(message, sizeof(message), fmt, args) == DIAG_MAX_LOG_SIZE - 1) {
        // ensure message always end with CRLF
        message[DIAG_MAX_LOG_SIZE - 2] = '\n';
    }

    log_entry_t *p = NULL;
    // if we are at capacity, remove the oldest entry and reuse it at the tail.
    if (s_log.chunks.len >= DIAG_MAX_LOG_HISTORY) {
        p = s_log.chunks.head;
        s_log.chunks.head = p->next;

        if (p->body) {
            FREE(p->body);
        }

        s_log.chunks.len--;
    } else {
        p = CALLOC(1, sizeof(log_entry_t));
    }

    p->body = STRNDUP(message, DIAG_MAX_LOG_SIZE);
    p->next = NULL;
    s_log.chunks.len++;

    if (!s_log.chunks.tail) {
        s_log.chunks.head = s_log.chunks.tail = p;
    } else {
        s_log.chunks.tail->next = p;
        s_log.chunks.tail = p;
    }
}

static void disable_iothub_endpoint(void)
{
    while (s_log.chunks.head) {
        log_entry_t *p = s_log.chunks.head;
        s_log.chunks.head = p->next;
        FREE(p->body);
        FREE(p);
    }
    s_log.chunks.head = s_log.chunks.tail = NULL;
    s_log.chunks.len = 0;
}

#ifdef ENABLE_SERIAL_LOG
static int enable_serial_endpoint(void)
{
    UART_Config config;
    UART_InitConfig(&config);
    config.baudRate = SERIAL_LOG_BAUDRATE;
    if ((s_log.uart_fd = UART_Open(SERIAL_LOG_PORT, &config)) < 0) {
        perror("failed to open uart");
        return -1;
    }

    s_log.tx_enable_fd = GPIO_OpenAsOutput(BOARD_UART_TX_ENABLE, GPIO_OutputMode_PushPull, GPIO_Value_Low);
    if (s_log.tx_enable_fd <=0) {
        perror("failed to open tx-enable for log");
        return -1;
    }
    return 0;
}

static void disable_serial_endpoint(void)
{
    if (s_log.uart_fd >= 0) {
        close(s_log.uart_fd);
    }
    if (s_log.tx_enable_fd >= 0) {
        close(s_log.tx_enable_fd);
    }
}

static void tcdrain(int baudrate, size_t count)
{
    // assume 8N1 uart config
    uint32_t bytes_per_second = baudrate / 10;
    uint32_t tx_enable_us = 1e6 * count / bytes_per_second;
    tx_enable_us += 200; // assume 200us processing time
    struct timespec delay;
    delay.tv_sec = tx_enable_us / 1e6;
    delay.tv_nsec = (tx_enable_us % 1000000) * 1000;
    nanosleep(&delay, NULL);
}

static void llog_serial(const char * fmt, va_list args)
{
    char message[DIAG_MAX_LOG_SIZE];
    size_t len = vsnprintf(message, sizeof(message), fmt, args);

    if (len == DIAG_MAX_LOG_SIZE - 1) {
        // ensure message always end with CRLF
        message[DIAG_MAX_LOG_SIZE - 2] = '\n';
    }
    fprintf(stderr, "%s", message);

    GPIO_SetValue(s_log.tx_enable_fd, GPIO_Value_High);
    write(s_log.uart_fd, message, len);
    tcdrain(SERIAL_LOG_BAUDRATE, len);
    GPIO_SetValue(s_log.tx_enable_fd, GPIO_Value_Low);
}
#endif

static void update_endpoint(int endpoint)
{
    if (s_log.endpoint == LOG_ENDPOINT_IOTHUB) {
        disable_iothub_endpoint();
#ifdef ENABLE_SERIAL_LOG
    } else if (s_log.endpoint == LOG_ENDPOINT_SERIAL) {
        disable_serial_endpoint();
    }

    if (endpoint == LOG_ENDPOINT_SERIAL) {
        if (enable_serial_endpoint() != 0) return;
#endif
    }

    s_log.endpoint = endpoint;
}
// ------------------------ public interface ------------------------------

#ifdef TEST

void llog(int level, const char *file, const char *func, const char *fmt, ...)
{
    if (func) {
        printf("%s %c %s: %s:", timespec2str(now()), LOG_TAGS[level], file, func);
    } else {
        printf("%s %c %s:", timespec2str(now()), LOG_TAGS[level], file);
    }

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

#else

void llog(int level, const char *file, const char *func, const char *fmt, ...)
{
    if ((s_log.endpoint == LOG_ENDPOINT_NULL) || (level > s_log.level)) return;

    // avoid reentry
    if (pthread_mutex_trylock(&s_log.lock) != 0) return;

    char newfmt[DIAG_MAX_LOG_SIZE];

    if (func) {
        snprintf(newfmt, sizeof(newfmt), "%s %c %s: %s: %s\n", timespec2str(now()), LOG_TAGS[level], file, func, fmt);
    } else {
        snprintf(newfmt, sizeof(newfmt), "%s %c %s: %s\n", timespec2str(now()), LOG_TAGS[level], file, fmt);
    }

    va_list args;
    va_start(args, fmt);

    if (s_log.endpoint == LOG_ENDPOINT_CONSOLE) {
        Log_DebugVarArgs(newfmt, args);
    } else if (s_log.endpoint == LOG_ENDPOINT_IOTHUB) {
        llog_iothub(newfmt, args);
#ifdef ENABLE_SERIAL_LOG
    } else if (s_log.endpoint == LOG_ENDPOINT_SERIAL) {
        llog_serial(newfmt, args);
#endif
    }

    va_end(args);
    pthread_mutex_unlock(&s_log.lock);

}
#endif // TEST


int llog_init()
{
    if (pthread_mutex_init(&s_log.lock, NULL) != 0) {
        perror("mutex init has failed");
        return -1;
    }

    s_log.endpoint = LOG_ENDPOINT_CONSOLE;
    s_log.level = LOG_LEVEL;
    s_log.chunks.head = s_log.chunks.tail = NULL;
    s_log.chunks.len = 0;

    return 0;
}

void llog_deinit(void)
{
    pthread_mutex_destroy(&s_log.lock);
}


void llog_config(int endpoint, int level)
{
    if (level != s_log.level) {
        s_log.level = level;
    }

    if (endpoint != s_log.endpoint) {
        update_endpoint(endpoint);
    }
}

bool llog_islog(int level)
{
    return level <= s_log.level;
}

bool llog_remote_log_enabled(void)
{
    return s_log.endpoint == LOG_ENDPOINT_IOTHUB && s_log.level > LOG_NONE;
}

void llog_upload(void)
{
    if (s_log.chunks.len == 0) return;

    char *iot_message = json_asprintf("[%M]", printf_logs, s_log.chunks.head);
    iot_send_message_async(iot_message, IOT_MESSAGE_TYPE_DIAG_DEBUG, NULL, NULL);
    FREE(iot_message);

    while (s_log.chunks.head) {
        log_entry_t *p = s_log.chunks.head;
        s_log.chunks.head = p->next;
        FREE(p->body);
        FREE(p);
    }

    s_log.chunks.head = s_log.chunks.tail = NULL;
    s_log.chunks.len = 0;
}
