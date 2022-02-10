/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
  
#pragma once
#include <stdbool.h>
#include <stdarg.h>


#ifndef LOG_LEVEL
#ifdef DEBUG
#define LOG_LEVEL LOG_DEBUG
#else
#define LOG_LEVEL LOG_NONE
#endif
#endif


// print all logs with log level higher/equal than LOG_LEVEL
enum {
    LOG_NONE = 0,
    LOG_FATAL,   // fatal, nothing can do except abort
    LOG_ERROR,   // should not happen, but recoverable
    LOG_WARN,    // rare, but possible
    LOG_INFO,    // just for your information
    LOG_DEBUG,   // debugging purpose
    LOG_VERBOSE  // chatty
};

enum {
    LOG_ENDPOINT_NULL,
    LOG_ENDPOINT_CONSOLE,
    LOG_ENDPOINT_IOTHUB,
#ifdef ENABLE_SERIAL_LOG
    LOG_ENDPOINT_SERIAL,
#endif
};

/**
 * initialize log module
 * 
 * log module support different log level and support redirect log to iothub to support
 * remote debugging log.
 * @return 0 if succeed, negative value otherwise
 */
int llog_init(void);

/**
 * deinitialize log module
 */
void llog_deinit(void);

/**
 * config endpoint and level of log, refere to above enum for allowed value
 * @param endpoint where to send log, console, iothub or not send
 * @param level any log with lower or equal log level with be printed out
 */
void llog_config(int endpoint, int level);


/**
 * log
 * @param level log level of this piece of log
 * @param file which file
 * @param func which function
 * @param fmt printf like format, note, line break will be append automatically
 */
void llog(int level, const char *file, const char *func, const char *fmt,...);

/**
 * check if certain level log will be printed
 * so we can check log level first before start time consuming log generation. e.g. (decode/encode)
 * @param level log level to check
 * @return whether given log level will be printed
 */
bool llog_islog(int level);

/**
 * check if log been redirected to remote (iothub)
 * @return whether log been redirected to remote
 */
bool llog_remote_log_enabled(void);

/**
 * upload log to iothub
 * log module maintain a buffer which can hold certain lines of log, upload log will flush
 * log from memory. when buffer full, additional log will be discarded
 */
void llog_upload(void);

#define LOGV(...) llog(LOG_VERBOSE, __FILE__, NULL, __VA_ARGS__)

#define LOGD(...) llog(LOG_DEBUG, __FILE__, NULL, __VA_ARGS__)

#define LOGI(...) llog(LOG_INFO,  __FILE__, NULL, __VA_ARGS__)

#define LOGW(...) llog(LOG_WARN,  __FILE__,  __FUNCTION__, __VA_ARGS__)

#define LOGE(...) llog(LOG_ERROR, __FILE__, __FUNCTION__, __VA_ARGS__)

