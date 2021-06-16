/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stdint.h>
#include <utils.h>
#include "eventloop_timer_utilities.h"

struct M24SR_WifiConfig {
	uint8_t ssidString[32];
	uint8_t networkKeyString[32];
	uint8_t wifiConfigSecurityType;
};

typedef void (*M24SR_GPOCallback) (const struct M24SR_WifiConfig* wifiConfig);
int M24SR_Init(EventLoop *eventLoop, M24SR_GPOCallback callback);
void M24SR_ClosePeripheralsAndHandlers(void);

