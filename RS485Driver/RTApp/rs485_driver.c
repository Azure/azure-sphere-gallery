/*
* Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/

#include <stdint.h>
#include <stdbool.h>

#include "lib/mt3620/uart.h"
#include "lib/GPIO.h"
#include "lib/UART.h"
#include "rs485_driver.h"

static Platform_Unit driverISU = DRIVER_ISU;
static unsigned driverIsuBaudrate = DRIVER_ISU_DEFAULT_BAURATE;
static uint8_t driverEnableGPIO = DRIVER_DE_GPIO;
static UART *uart_handle = NULL;
static void (*uart_rxIrq_callback)(void);

uint8_t rxBuffer[DRIVER_MAX_RX_BUFFER_SIZE];
ringBuffer_t rs485_rxRingBuffer;

bool Rs485_Init(uint32_t baudrate, void (*rxIrqCallback)(void))
{
	if (baudrate == 0)
		return false;

	if (NULL != uart_handle)
	{
		UART_Close(uart_handle);
	}

	driverIsuBaudrate = baudrate;

	// Initialize the RS-485 UART
	if (NULL != rxIrqCallback)
	{
		uart_rxIrq_callback = rxIrqCallback;
	}
	uart_handle = UART_Open(driverISU, driverIsuBaudrate, UART_PARITY_NONE, 1, uart_rxIrq_callback);
	if (!uart_handle) {
		return false;
	}

	// Setup the RX message queue to be sent to the HLApp
	ring_buffer_init(&rs485_rxRingBuffer, rxBuffer, sizeof(rxBuffer));

	// Setup DE/!RE driver GPIO
	GPIO_ConfigurePinForOutput(driverEnableGPIO);
	GPIO_Write(driverEnableGPIO, false);

	return true;
}

inline void Rs485_Close(void)
{
	UART_Close(uart_handle);
	uart_handle = NULL;
}

inline uintptr_t Rs485_ReadAvailable(void)
{
	return UART_ReadAvailable(uart_handle);
}

inline int32_t Rs485_Read(void *data, uintptr_t size)
{
	return UART_Read(uart_handle, data, size);
}

int32_t Rs485_Write(const void *data, uintptr_t size)
{
	int32_t res = ERROR_BUSY;

	bool state;
	if (GPIO_Read(driverEnableGPIO, &state) == ERROR_NONE && state == false)
	{
		// Enable the transceiver's TX driver (DE), consequently the transceiver's RX driver (!RE) 
		// shall be disabled in hardware (i.e. connected together to DE as !RE is active low).
		GPIO_Write(driverEnableGPIO, true);

		// Write to the RS-485 transceiver
		int32_t res = UART_Write(uart_handle, data, size);

		if (ERROR_NONE == res)
		{
			// Wait for the UART's hardware TX buffer to empty
			uint32_t retries = 0xFFFF;
			while (retries && !UART_IsWriteComplete(uart_handle)) retries--;

			if (retries == 0)
			{
				res = ERROR_TIMEOUT;
			}
			else
			{
				// This is fine-tuned with a scope to achieve a minimal delay, 
				// so to fully include the STOP bit after the TX of the last byte
				for (int i = 300; i > 0; i--) __asm__("nop");
			}
		}

		// Disable the transceiver's TX driver (DE), consequently the transceiver's RX driver (!RE) 
		// shall be enabled in hardware (i.e. connected together to DE as !RE is active low)..
		GPIO_Write(driverEnableGPIO, false);
	}

	return res;
}