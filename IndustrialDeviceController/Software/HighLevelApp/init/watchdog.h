/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
  
#pragma once

/**
 * initialize watchdog module
 * watchdo expect watchdog_kick() to be invoked periodically, otherwise it will generate watchdog warning event for a few times
 * and eventually watchdog timeout and restart app. This is to prevent app stucked in a loop.
 */
void watchdog_init(void);

/**
 * kick the dog
 */
void watchdog_kick(void);
