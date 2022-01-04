#ifndef _I2C_H_
#define _I2C_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "os_hal_i2c.h"

#define I2C_BUFFER_LENGTH 32

int Native_ReadRegU8(i2c_num i2cBus, uint8_t reg, uint8_t* value, uint8_t deviceAddress);
int Native_ReadData(i2c_num i2cBus, uint16_t reg, void* data, size_t len, uint8_t deviceAddress);

// VL53L1X functions.
void writeRegister(uint16_t addr, uint8_t val);
void writeRegister16(uint16_t addr, uint16_t val);
void writeRegister32(uint16_t reg, uint32_t value);

uint8_t readRegister(uint16_t addr);
uint16_t readRegister16(uint16_t addr);
uint32_t readRegister32(uint16_t reg);

int readBlockData(uint8_t* txBuffer, uint8_t* rxBuffer, uint16_t writeLen, uint16_t readLen);

#endif
