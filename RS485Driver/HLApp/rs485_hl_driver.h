#pragma once

#include <stdbool.h>
#include <stdint.h>


typedef void Rs485ReceiveCallback(int bytesReceived);	// The receive callback, called upon any receive event from the real-time RS-485 driver (RTApp).
extern const char rtAppComponentId[];			// The corresponding RTApp's ComponentId, to be defined externally (i.e. in main.c).

/// <summary>
///	Initializes the connection to the real-time RS-485 driver (RTApp).
/// </summary>
/// <param name="eventLoop">A pointer to the EventLoop to which register the inter-core communication with the real-time RS-485 driver (RTApp).</param>
/// <param name="rxBuffer">A pointer to a byte-buffer into which the RS-485 driver will store the bytes received from the real-time RS-485 driver (RTApp).</param>
/// <param name="rxBufferSize">The size in bytes of the given byte-buffer.</param>
/// <param name="callback">A pointer to a 'Rs485ReceiveCallback'-typed callback function, which the HL-Core RS-485 driver invokes upon receive events.</param>
/// <returns>'0'</returns>
int Rs485_Init(EventLoop *eventLoop, uint8_t *rxBuffer, size_t rxBufferSize, Rs485ReceiveCallback *callback);

/// <summary>
///	Closes the internal handles managing the connection to the real-time RS-485 driver (RTApp).
/// </summary>
/// <param name=""></param>
void Rs485_Close(void);

/// <summary>
/// Sends a byte-buffer to the real-time RS-485 driver (RTApp).
/// The maximum size of the data buffer is defined by the MAX_HLAPP_MESSAGE_SIZE macro in common_defs.h.
/// </summary>
/// <param name="data">A pointer to the data buffer.</param>
/// <param name="size">Size of the data buffer in bytes.</param>
/// <returns>The number of bytes sent or -1 on error.</returns>
int Rs485_Send(const void *data, size_t dataLen);