/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stdint.h>
#include <utils.h>
#include "eventloop_timer_utilities.h"

struct WifiConfig {
	uint8_t ssidString[32];
	uint8_t networkKeyString[32];
	uint8_t wifiConfigSecurityType;
};

typedef void (*GPOCallback) (const struct WifiConfig* wifiConfig);
int M24SR_Init(EventLoop *eventLoop, GPOCallback callback);
void M24SR_ClosePeripheralsAndHandlers(void);

