/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <string.h>

#include <init/globals.h>
#include <utils/uart.h>
#include <utils/utils.h>

// config_string - null terminated uart config
int parse_uart_config_string(const char *config_string, UART_Config *config)
{
    ASSERT(config_string);
    ASSERT(config);

    char *ptr = (char *)config_string;
    char *endptr = NULL;

    do {
        UART_InitConfig(config);
        config->baudRate = strtol(ptr, &endptr, 10);

        if (ptr == endptr || *endptr == '\0') {
            break;
        }

        ptr = endptr + 1;
        config->blockingMode = (UART_BlockingMode_Type)strtol(ptr, &endptr, 10);

        if (ptr == endptr || *endptr == '\0' || config->blockingMode != UART_BlockingMode_NonBlocking) {
            break;
        }

        ptr = endptr + 1;
        config->dataBits = (UART_DataBits_Type)strtol(ptr, &endptr, 10);

        if (ptr == endptr || *endptr == '\0') {
            break;
        }

        ptr = endptr + 1;
        config->parity = (UART_Parity_Type)strtol(ptr, &endptr, 10);

        if (ptr == endptr || *endptr == '\0') {
            break;
        }

        ptr = endptr + 1;
        config->stopBits = (UART_StopBits_Type)strtol(ptr, &endptr, 10);

        if (ptr == endptr || *endptr == '\0') {
            break;
        }

        ptr = endptr + 1;
        config->flowControl = (UART_FlowControl_Type)strtol(ptr, &endptr, 10);

        if (ptr == endptr) {
            break;
        }

        return 0;
    } while (0);

    return -1;
}