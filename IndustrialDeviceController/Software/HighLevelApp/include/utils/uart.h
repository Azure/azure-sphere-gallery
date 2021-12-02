/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <applibs/uart.h>

#include <init/applibs_versions.h>

/**
 * parse uart config from string
 * @param config_string string taken format of "<baudrate>:<block mode>:<data bits>:<parity>:<stop bits>:<flow control>"
 * 
 */
int parse_uart_config_string(const char *config_string, UART_Config *config);

#ifndef TEST
#define UART_read read
#define UART_write write
#define UART_close close
#endif