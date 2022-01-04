/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "utils.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <applibs/i2c.h>

void delay(int ms)
{
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

void delayMicroseconds(int us)
{
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = us * 1000;
	nanosleep(&ts, NULL);
}

// DumpBuffer stuff.
static char ascBuff[17];
void DisplayLineOffset(size_t offset);

void DumpBuffer(uint8_t* buffer, size_t length)
{
	int LinePos = 0;

	size_t lineOffset = 0;

	DisplayLineOffset(lineOffset);
	memset(ascBuff, 0x20, 16);
	for (size_t x = 0; x < length; x++)
	{
		Log_Debug("%02x ", buffer[x]);
		if (buffer[x] >= 0x20 && buffer[x] <= 0x7f)
			ascBuff[LinePos] = buffer[x];
		else
			ascBuff[LinePos] = '.';

		LinePos++;
		if (LinePos == 0x10)
		{
			Log_Debug("%s", ascBuff);
			Log_Debug("\n");
			lineOffset = lineOffset+16;
			if (x + 1 != length)
			{
				DisplayLineOffset(lineOffset);
			}
			LinePos = 0;
			memset(ascBuff, 0x20, 16);
		}
	}

	if (LinePos != 0)
		Log_Debug("%s", ascBuff);

	Log_Debug("\n");
}

void DisplayLineOffset(size_t offset)
{
	Log_Debug("%04x: ", offset);
}

