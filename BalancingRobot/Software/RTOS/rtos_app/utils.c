#include <stdint.h>
#include <stdbool.h>
#include "utils.h"
#include <tx_api.h>
#include "i2c.h"

unsigned long millis(void)
{
	return tx_time_get();
}

void EnumI2CDevices(i2c_num driver)
{
    printf("Enumerate I2C Devices\r\n");
    uint16_t devAddr = 0;

    uint8_t data[5];
    for (unsigned int x = 0; x < 0x80; x++)
    {
        devAddr = x;
        
        int result = Native_ReadRegU8(driver, 0, &data[0], devAddr);
        
        if (result >= 0)
        {
            printf("0x%02x\r\n", x);
        }
    }
}

static char ascBuff[32];

void DisplayLineOffset(uint16_t offset)
{
	printf("%04x: ", offset);
}

void DumpBuffer(uint8_t* buffer, uint16_t length)
{
	int LinePos = 0;

	uint16_t lineOffset = 0;

	DisplayLineOffset(lineOffset);
	memset(ascBuff, 0x20, 16);
	for (int x = 0; x < length; x++)
	{
		printf("%02x ", buffer[x]);
		if (buffer[x] >= 0x20 && buffer[x] <= 0x7f)
			ascBuff[LinePos] = buffer[x];
		else
			ascBuff[LinePos] = '.';

		LinePos++;
		if (LinePos == 0x10)
		{
			printf("%s", ascBuff);
			printf("\n");
			lineOffset += 16;
			if (x + 1 != length)
			{
				DisplayLineOffset(lineOffset);
			}
			LinePos = 0;
			memset(ascBuff, 0x20, 16);
		}
	}

	if (LinePos != 0)
		printf("%s", ascBuff);

	printf("\n");
}
