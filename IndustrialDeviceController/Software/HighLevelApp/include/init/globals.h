/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stdbool.h>
#include <hw/board_config.h>
#include <utils/memory.h>


// number of char for data point value
#define TELEMETRY_MAX_KEY_SIZE  256
#define TELEMETRY_MAX_VALUE_SIZE 256

//////////// config for diag task /////////////////
// To support override from UT
#ifndef DIAG_EVENT_REPORT_MS
#define DIAG_EVENT_REPORT_MS 30000
#endif

#ifndef DIAG_TWIN_REPORT_MS
#define DIAG_TWIN_REPORT_MS 30000
#endif

#ifndef DIAG_TELEMETRY_REPORT_MS
#define DIAG_TELEMETRY_REPORT_MS 60000
#endif

#ifndef DIAG_HEARTBEAT_MS
#define DIAG_HEARTBEAT_MS 60000
#endif

#ifndef DIAG_LOG_REPORT_MS
#define DIAG_LOG_REPORT_MS 5000
#endif

#define DIAG_LED_UPDATE_MS 500

#define DIAG_MAX_LOG_HISTORY 50
#define DIAG_MAX_LOG_SIZE 1000
#define DIAG_SYSTEM_BOOT_TIME 10

#define DIAG_PEAK_USERMODE_MEMORY_WATERMARK 250

#define DIAG_OFFLINE_SECOND_TO_REBOOT 3600

//////////// config for iot task /////////////////
#define IOT_MAX_INFLIGHT_MESSAGE_SIZE 40*1024
#define IOT_MESSAGE_EXPIRE_SECOND 30

#ifndef IOT_SETUP_RETRY_MS
#define IOT_SETUP_RETRY_MS 60000
#endif

#ifndef IOT_SETUP_MAX_RETRY_MS
#define IOT_SETUP_MAX_RETRY_MS 120*1000
#endif

#define IOT_PERIODIC_TASK_MS 100

/////////// config for watchdog task ////////////
#define WATCHDOG_WARNING_SEC 60
#define WATCHDOG_WARNING_TIMES 5

//////////// RT Application ID //////////////////
#define RT_APP_COMPONENT_ID "f9676ffa-3060-4c7a-8dc0-a464262bc993"

//////////// MODBUS RTU //////////////////
// When board works in RS485 mode, need to pull high on
// the TX Enable GPIO when sending data and pull low
// on the TX enable GPIO when receiving
// Actually pull high time is the time needed to send
// given packet with current baudrate multiple this factor
#define UART_TX_ENABLE_DELAY_FACTOR    1.0

///////////// PXC36 //////////////////////
// buffer length for single string
#define PXC36_STR_CHUNK_SIZE 512
#define PXC36_MAX_PARAM_NUM 5

#define PXC36_UART_READ_STRING_TIMEOUT_MS 500
#define PXC36_UART_READ_RESPONSE_TIMEOUT_MS 10*1000

////////////// log /////////////////////
#define MODBUS_T35_MS 5
#define MODBUS_T35_ADJUST_STEP 3
#define MODBUS_T35_MAXIMUM_RETRY 10
#define MODBUS_T35_DATAPOINT "MODBUS_RTU_DELAY"

///////////// mutable storage /////////
// diag event file - 10k
#define EVENT_FILE_OFFSET 0
#define EVENT_FILE_SIZE 10000

// local cache of provision - 30k
#define PROVISION_FILE_OFFSET 10100
#define PROVISION_FILE_MAX_SIZE 30000

// property file - 1k
#define PROPERTY_FILE_OFFSET 40200
#define PROPERTY_FILE_SIZE 1000

///////////// edge /////////
#define IOT_EDGE_IP1 "13.66.204.246"
#define IOT_EDGE_IP2 "40.122.45.153"
#define IOT_EDGE_PORT1 8082
#define IOT_EDGE_PORT2 8082

//#define KAFKA_ENDPOINT "topics/tahuya-telemetry"
#define KAFKA_ENDPOINT "topics"

#define ERR_MSG_SIZE 100


#define DEFAULT_INTEGRITY_PERIOD_MS 600000