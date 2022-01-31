/// \file uart_config_v1.h
/// \brief This header contains the v1 definition of the UART configuration struct and
/// associated types.
///
/// You should not include this header directly; include applibs/uart.h instead.
#pragma once

#include <stdint.h>
#include <stdlib.h>

typedef uint32_t UART_BaudRate_Type;
typedef uint8_t UART_BlockingMode_Type;
typedef uint8_t UART_DataBits_Type;
typedef uint8_t UART_Parity_Type;
typedef uint8_t UART_StopBits_Type;
typedef uint8_t UART_FlowControl_Type;

/// <summary>
/// The configuration options for a UART. Call UART_InitConfig to initialize an instance.
///
/// After you define UART_STRUCTS_VERSION, you can use the UART_Config alias to access this
/// structure.
/// </summary>
struct z__UART_Config_v1 {
    /// <summary>A unique identifier of the struct type and version. Do not edit.</summary>
    uint32_t z__magicAndVersion;
    /// <summary>The baud rate of the UART.</summary>
    UART_BaudRate_Type baudRate;
    /// <summary>The blocking mode setting for the UART.</summary>
    UART_BlockingMode_Type blockingMode;
    /// <summary>The data bits setting for the UART.</summary>
    UART_DataBits_Type dataBits;
    /// <summary>The parity setting for the UART.</summary>
    UART_Parity_Type parity;
    /// <summary>The stop bits setting for the UART.</summary>
    UART_StopBits_Type stopBits;
    /// <summary>The flow control setting for the UART.</summary>
    UART_FlowControl_Type flowControl;
};

/// <summary>
/// The valid values for UART blocking or non-blocking modes.
/// </summary>
typedef enum {
    /// <summary>Reads and writes to the file handle are non-blocking and return an
    /// error if the call blocks. Reads may return less data than requested.</summary>
    UART_BlockingMode_NonBlocking = 0,
} UART_BlockingMode;

/// <summary>
/// The valid values for UART data bits.
/// </summary>
typedef enum {
    /// <summary>Five data bits.</summary>
    UART_DataBits_Five = 5,
    /// <summary>Six data bits.</summary>
    UART_DataBits_Six = 6,
    /// <summary>Seven data bits.</summary>
    UART_DataBits_Seven = 7,
    /// <summary>Eight data bits.</summary>
    UART_DataBits_Eight = 8
} UART_DataBits;

/// <summary>
/// The valid values for UART parity.
/// </summary>
typedef enum {
    /// <summary>No parity bit.</summary>
    UART_Parity_None = 0,
    /// <summary>Even parity bit.</summary>
    UART_Parity_Even = 1,
    /// <summary>Odd parity bit.</summary>
    UART_Parity_Odd = 2
} UART_Parity;

/// <summary>
/// The valid values for UART stop bits.
/// </summary>
typedef enum {
    /// <summary>One stop bit.</summary>
    UART_StopBits_One = 1,
    /// <summary>Two stop bits.</summary>
    UART_StopBits_Two = 2
} UART_StopBits;

/// <summary>
/// The valid values for flow control settings.
/// </summary>
typedef enum {
    /// <summary>No flow control.</summary>
    UART_FlowControl_None = 0,
    /// <summary>Enable RTS/CTS hardware flow control.</summary>
    UART_FlowControl_RTSCTS = 1,
    /// <summary>Enable XON/XOFF software flow control.</summary>
    UART_FlowControl_XONXOFF = 2
} UART_FlowControl;
