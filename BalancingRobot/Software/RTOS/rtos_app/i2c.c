#include "i2c.h"
#include "os_hal_i2c.h"
#include "utils.h"
#include <tx_api.h>

extern TX_MUTEX i2cMutex;
#define TXRX_BUFFER_SIZE 32

// Transfers larger than 8 bytes must be in sysram
static __attribute__((section(".sysram"))) uint8_t writeBuffer[TXRX_BUFFER_SIZE];
static __attribute__((section(".sysram"))) uint8_t readBuffer[TXRX_BUFFER_SIZE];

int Native_ReadData(i2c_num i2cBus, uint16_t reg, void* data, size_t len, uint8_t deviceAddress)
{
#ifdef SHOW_DEBUG_MSGS
	Log_Entry();
#endif
	if (len > TXRX_BUFFER_SIZE)
	{
		printf("Native_ReadData - buffer size too large\r\n");
		return -1;
	}

	uint8_t regW = reg & 0xff;
	int ret = 0;
	writeBuffer[0] = regW;

	if (len > 8)
	{
		printf("request size too large...\r\n");
	}

	mtk_os_hal_i2c_write_read(i2cBus, deviceAddress, &writeBuffer[0], &readBuffer[0], 1, len);

	memcpy(data, readBuffer, len);
	return ret;
}

int Native_ReadRegU8(i2c_num i2cBus, uint8_t reg, uint8_t* value, uint8_t deviceAddress)
{
#ifdef SHOW_DEBUG_MSGS
	Log_Entry();
#endif

	int ret = Native_ReadData(i2cBus, reg, value, sizeof(*value), deviceAddress);
	return ret;
}

//Write a byte to a spot
void writeRegister(uint16_t addr, uint8_t val)
{
	uint8_t buffer[3] = { 0 };
	buffer[0] = addr >> 8;
	buffer[1] = addr & 0xff;
	buffer[2] = val;

	__builtin_memcpy(writeBuffer, buffer, 3);

	int fnRes = mtk_os_hal_i2c_write(OS_HAL_I2C_ISU1, 0x29, writeBuffer, 3);
	if (fnRes < 0)
	{
		printf("writeRegister failed\r\n");
	}
}

//Write two bytes to a spot
void writeRegister16(uint16_t addr, uint16_t val)
{
	uint8_t buffer[4] = { 0 };
	buffer[0] = addr >> 8;
	buffer[1] = addr & 0xff;
	buffer[2] = val >> 8;
	buffer[3] = val & 0xff;

	__builtin_memcpy(writeBuffer, buffer, 4);

	int fnRes = mtk_os_hal_i2c_write(OS_HAL_I2C_ISU1, 0x29, writeBuffer, 4);
	if (fnRes < 0)
	{
		printf("writeRegister16 failed\r\n");
	}
}

void writeRegister32(uint16_t reg, uint32_t value)
{
	uint8_t buffer[6];

	buffer[0] = reg >> 8;
	buffer[1] = reg & 0xff;
	buffer[2] = (value >> 24) & 0xFF; // value highest byte
	buffer[3] = (value >> 16) & 0xFF;
	buffer[4] = (value >> 8) & 0xFF;
	buffer[5] = value & 0xFF; // value lowest byte

	__builtin_memcpy(writeBuffer, buffer, 6);

	int fnRes=mtk_os_hal_i2c_write(OS_HAL_I2C_ISU1, 0x29, writeBuffer, 6);
	if (fnRes < 0)
	{
		printf("writeRegister32 failed\r\n");
	}
}

//Reads one byte from a given location
//Returns zero on error
uint8_t readRegister(uint16_t addr)
{
	uint8_t buffer[4] = { 0 };
	buffer[0] = addr >> 8;
	buffer[1] = addr & 0xff;

	__builtin_memcpy(writeBuffer, buffer, 2);

	int result = mtk_os_hal_i2c_write_read(OS_HAL_I2C_ISU1, 0x29, writeBuffer, readBuffer, 2, 1);

	if (result < 0)
	{
		printf("readRegister - Failed mtk_os_hal_i2c_write_read (%d)\r\n", result);
	}

	uint8_t value = 0;

	value = readBuffer[0];       // value lowest byte

	return value;
}

//Reads two consecutive bytes from a given location
//Returns zero on error
uint16_t readRegister16(uint16_t addr)
{
	uint8_t buffer[4] = { 0 };
	buffer[0] = addr >> 8;
	buffer[1] = addr & 0xff;

	__builtin_memcpy(writeBuffer, buffer, 2);

	int result = mtk_os_hal_i2c_write_read(OS_HAL_I2C_ISU1, 0x29, writeBuffer, readBuffer, 2, 2);

	if (result < 0)
	{
		printf("readRegister16 - Failed mtk_os_hal_i2c_write_read (%d)\r\n", result);
	}

	uint16_t value=0;

	value |= (uint16_t)readBuffer[0] << 8;
	value |= readBuffer[1];       // value lowest byte

	return value;
}

// Read a 32-bit register
uint32_t readRegister32(uint16_t reg)
{
	uint8_t buffer[4] = { 0 };
	buffer[0] = reg >> 8;
	buffer[1] = reg & 0xff;

	__builtin_memcpy(writeBuffer, buffer, 2);

	int result = mtk_os_hal_i2c_write_read(OS_HAL_I2C_ISU1, 0x29, writeBuffer, readBuffer, 2, 4);

	if (result < 0)
	{
		printf("readRegister32 - Failed mtk_os_hal_i2c_write_read (%d)\r\n", result);
	}

	uint32_t value;

	value = (uint32_t)readBuffer[0] << 24; // value highest byte
	value |= (uint32_t)readBuffer[1] << 16;
	value |= (uint16_t)readBuffer[2] << 8;
	value |= readBuffer[3];       // value lowest byte

	return value;
}

int readBlockData(uint8_t* txBuffer, uint8_t* rxBuffer, uint16_t writeLen, uint16_t readLen)
{
	__builtin_memcpy(writeBuffer, txBuffer, writeLen);

	int result = mtk_os_hal_i2c_write(OS_HAL_I2C_ISU1, 0x29, writeBuffer, writeLen);
	if (result < 0)
	{
		printf("readBlockData - write failed\r\n");
	}

	for (int x = 0; x < readLen; x++)
	{
		result = mtk_os_hal_i2c_read(OS_HAL_I2C_ISU1, 0x29, readBuffer, 1);
		if (result < 0)
		{
			printf("readBlockData - read failed\r\n");
			break;
		}
		else
		{
			rxBuffer[x] = readBuffer[0];
		}
	}

	return result;
}