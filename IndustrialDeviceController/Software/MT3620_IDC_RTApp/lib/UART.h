/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef AZURE_SPHERE_UART_H_
#define AZURE_SPHERE_UART_H_

#include "Platform.h"
#include "Common.h"
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
 extern "C" {
#endif

/// <summary>Opaque UART handle.</summary>
typedef struct UART UART;

/// <summary>UART parity modes.</summary>
typedef enum {
    /// <summary>No parity bit transmitted.</summary>
    UART_PARITY_NONE       = 0,
    /// <summary>Parity bit 1 when even parity is detected.</summary>
    UART_PARITY_EVEN       = 1,
    /// <summary>Parity bit 1 when odd parity is detected.</summary>
    UART_PARITY_ODD        = 2,
    /// <summary>Parity bit hardcoded to 0.</summary>
    UART_PARITY_STICK_ZERO = 3,
    /// <summary>Parity bit hardcoded to 1.</summary>
    UART_PARITY_STICK_ONE  = 4,
} UART_Parity;

/// <summary>The UART interrupts (and hence callbacks) run at this priority level.</summary>
static const uint32_t UART_PRIORITY = 2;

/// <summary>
/// <para>The application must call this function once before using a given UART.</para>
/// </summary>
/// <param name="unit">Which UART to initialize.</param>
/// <param name="baud">Target baud rate.</param>
/// <param name="parity">Parity mode: <see cref="UART_Parity" />.</param>
/// <param name="stopBits">Number of stop bits, only 1 or 2 are valid.</param>
/// <param name="rxCallback">An optional callback to invoke when the UART receives data.
/// This can be NULL if the application does not want to read any data from the UART. The
/// application should call <see cref="UART_DequeueData" /> to retrieve the data.</param>
UART *UART_Open(
    Platform_Unit unit,
    unsigned      baud,
    UART_Parity   parity,
    unsigned      stopBits,
    void         (*rxCallback)(void));

/// <summary>
/// <para>Releases a handle once it's finished using a given UART interface. 
/// Once released the handle is free to be opened again.</para>
/// </summary>
/// <param name="handle">The UART handle which is to be released.</param>
void UART_Close(UART *handle);

/// <summary>
/// <para>Buffers the supplied data and asynchronously writes it to the supplied UART.
/// If there is not enough space to buffer the data, then any unbuffered data will be discarded.
/// The size of the buffer is defined by the TX_BUFFER_SIZE macro in UART.c.</para>
/// <para>To send a null-terminated string, call <see cref="Uart_EnqueueString" />.
/// To send an integer call <see cref="UART_EnqueueIntegerAsString" /> or
/// <see cref="UART_EnqueueIntegerAsHexString"/>.</para>
/// </summary>
/// <param name="handle">Which UART to write the data to.</param>
/// <param name="data">Start of the data buffer.</param>
/// <param name="size">Size of the data in bytes.</param>
/// <returns>ERROR_NONE on success, or an error code.</returns>
int32_t UART_Write(UART *handle, const void *data, uintptr_t size);

/// <summary>
/// This function checks if the UART's hardware TX buffer is actually empty.
/// </summary>
/// <param name="handle">Which UART to read TX buffer status from.</param>
/// <returns>'true' if the UART's hardware TX buffer is empty, 'false' otherwise.</returns>
bool UART_IsWriteComplete(UART *handle);

/// <summary>
/// This function blocks until it has read size bytes from the UART.
/// </summary>
/// <param name="handle">Which UART to read the data from.</param>
/// <param name="data">Start of buffer into which data should be written.</param>
/// <param name="size">Size of data in bytes.</param>
/// <returns>ERROR_NONE on success, or an error code.</returns>
int32_t UART_Read(UART *handle, void *data, uintptr_t size);

/// <summary>
/// This function returns the number of bytes currently buffered for a UART.
/// </summary>
/// <param name="handle">Which UART to read the read buffer size of.</param>
/// <returns>Number of bytes available to be read.</returns>
uintptr_t UART_ReadAvailable(UART *handle);


#ifdef __cplusplus
 }
#endif

#endif // #ifndef AZURE_SPHERE_UART_H_
