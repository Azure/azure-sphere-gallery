/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

// This is the maximum message size that the HLApp can send
// to the RS-485 RTApp driver. This header is used by both Apps.
#define MAX_HLAPP_MESSAGE_SIZE	64

// This is essentially the buffering time (in milliseconds),
// after which the real-time RS485 driver will send out 
// any received bytes to the HLApp.
#define RTDRV_SEND_DELAY_MSEC	10