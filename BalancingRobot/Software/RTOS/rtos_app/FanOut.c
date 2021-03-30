#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "utils.h"
#include "FanOut.h"
#include "os_hal_i2c.h"
#include "tx_api.h"

bool SelectFanoutChannel(uint8_t channelNumber)
{
	uint8_t command[1];
	command[0] = channelNumber;

	mtk_os_hal_i2c_write(OS_HAL_I2C_ISU1, pca9546aAddress, command, 1);
	delay(2);

	return true;
}
