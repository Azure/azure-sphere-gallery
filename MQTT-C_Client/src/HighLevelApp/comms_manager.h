/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "dx_exit_codes.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "mqtt.h"
#include <applibs/log.h>
#include <applibs/storage.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <sys/select.h>
#include <wolfssl/ssl.h>

void initialize_mqtt(void (*publish_callback)(void** unused, struct mqtt_response_publish* published), void (*mqtt_connected_cb)(void),
	const char** sub_topics, size_t sub_topic_count);
void publish_message(const void* data, size_t data_length, const char* topic);
bool is_mqtt_connected(void);
