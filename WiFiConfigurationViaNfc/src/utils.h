/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <applibs/log.h>

typedef enum {
	ExitCode_Success = 0,
	ExitCode_TermHandler_SigTerm = 1,
	ExitCode_Init_EventLoop = 2,
	ExitCode_Main_EventLoopFail = 3,
	ExitCode_Init_Failed = 4,
	ExitCode_GPOTimer_Consume = 5,
	ExitCode_GPOTimer_GetState = 6
} ExitCode;

#define delay_ms delay

void delay(int ms);
void delayMicroseconds(int us);
void DumpBuffer(uint8_t* buffer, size_t length);
