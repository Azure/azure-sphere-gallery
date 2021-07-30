/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "utils.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <applibs/i2c.h>

static char ascBuff[17];
static void DisplayLineOffset(uint16_t offset);

void delay(int ms)
{
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

void DumpBuffer(uint8_t* buffer, uint16_t length)
{
	int LinePos = 0;

	uint16_t lineOffset = 0;

	DisplayLineOffset(lineOffset);
	memset(ascBuff, 0x20, sizeof(ascBuff));
	for (int x = 0; x < length; x++)
	{
		Log_Debug("%02x ", buffer[x]);
		if (buffer[x] >= 0x20 && buffer[x] <= 0x7f)
			ascBuff[LinePos] = buffer[x];
		else
			ascBuff[LinePos] = '.';

		LinePos++;
		if (LinePos == sizeof(ascBuff) - 1)
		{
			Log_Debug("%s\n", ascBuff);
			lineOffset += sizeof(ascBuff)-1;
			if (x + 1 != length)
			{
				DisplayLineOffset(lineOffset);
			}
			LinePos = 0;
			memset(ascBuff, 0x20, sizeof(ascBuff));
		}
	}

	if (LinePos != 0)
		Log_Debug("%s", ascBuff);

	Log_Debug("\n");
}

static void DisplayLineOffset(uint16_t offset)
{
	Log_Debug("%04x: ", offset);
}


