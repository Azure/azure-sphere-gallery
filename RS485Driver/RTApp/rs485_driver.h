/*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "../common_defs.h"
#include "ringBuffer.h"

//////////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////////
#define DRIVER_ISU						MT3620_UNIT_ISU0
#define DRIVER_ISU_DEFAULT_BAURATE		9600
#define DRIVER_DE_GPIO					42
#define DRIVER_MAX_RX_BUFFER_SIZE		2048
#define DRIVER_MAX_RX_BUFFER_FILL_SIZE  2000

extern ringBuffer_t rs485_rxRingBuffer;


/// <summary>
/// This function initializes the internal RS-485 UART handle, DE GPIO and TX ring buffer.
/// </summary>
/// <param name="baudrate">The baudrate to which the UART should be configured.</param>
/// <param name="rxIrqCallback">A pointer to the function to be called upon an RX interrupt.
/// If NULL the previous setting is retained (useful when just changing the baudrate).</param>
/// <returns>'true' is the initialization succeeds, 'false' otherwise.</returns>
bool Rs485_Init(uint32_t baudrate, void (*rxIrqCallback)(void));

/// <summary>
/// Closes the internal UART handle used by the RS-485 driver.
/// </summary>
/// <param name=""></param>
/// <returns></returns>
void Rs485_Close(void);

/// <summary>
/// This function returns the number of bytes currently buffered for a RS-485 UART.
/// </summary>
/// <returns>Number of bytes available to be read.</returns>
uintptr_t Rs485_ReadAvailable(void);

/// <summary>
/// This function blocks until it has read size bytes from the RS-485 UART.
/// </summary>
/// <param name="data">Start of buffer into which data should be written.</param>
/// <param name="size">Size of data in bytes.</param>
/// <returns>ERROR_NONE on success, or an error code.</returns>
int32_t Rs485_Read(void *data, uintptr_t size);

/// <summary>
/// <para>Buffers the supplied data and asynchronously writes it to the internal RS-485 UART handle.
/// If there is not enough space to buffer the data, then any unbuffered data will be discarded.
/// The size of the buffer is defined by the TX_BUFFER_SIZE macro in lib\UART.c.</para>
/// </summary>
/// <param name="data">A pointer to the data buffer.</param>
/// <param name="size">Size of the data buffer in bytes.</param>
/// <returns>ERROR_NONE on success, or an error code.</returns>
int32_t Rs485_Write(const void *data, uintptr_t size);