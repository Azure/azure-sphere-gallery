/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "comms_manager.h"
#include "dx_exit_codes.h"
#include "dx_terminate.h"
#include "dx_timer.h"
#include "dx_utilities.h"
#include "mqtt.h"
#include "string.h"
#include <applibs/log.h>

static void publish_message_timer_handler(EventLoopTimer* eventLoopTimer);

typedef struct {
	char message[128];
	size_t message_length;
} MQTT_MESSAGE;

MQTT_MESSAGE mqtt_msg;

const char* sub_topics[] = { "azuresphere/sample/device" };
const char* pub_topic = "azuresphere/sample/host";

// fire the timer evey 5 seconds
static DX_TIMER publish_message_timer = { .period = {1, 0}, .name = "publish_message_timer", .handler = publish_message_timer_handler };
static DX_TIMER* timerSet[] = { &publish_message_timer };

static void publish_message_timer_handler(EventLoopTimer* eventLoopTimer) {
	if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
		dx_terminate(DX_ExitCode_ConsumeEventLoopTimeEvent);
		return;
	}
	if (is_mqtt_connected()) {
		publish_message(mqtt_msg.message, mqtt_msg.message_length, pub_topic);
	}
}

// this function is called when the device receives a new message from the MQTT Broker
static void publish_callback(void** unused, struct mqtt_response_publish* published) {
	char* message;

	if (published->application_message_size <= sizeof(mqtt_msg.message)) {

		memcpy(mqtt_msg.message, published->application_message, published->application_message_size);
		mqtt_msg.message_length = published->application_message_size;

		// Now lets print the message received
		message = (char*)malloc(published->application_message_size + 1);
		memset(message, 0x00, published->application_message_size + 1);

		memcpy(message, published->application_message, published->application_message_size);

		Log_Debug("Message Recieved: %s", message);

		free(message);
	}
}

static void mqtt_connected_cb(void) {
	Log_Debug("Connected to MQTT Broker\n");
}

/// <summary>
///  Initialize peripherals, device twins, direct methods, timers.
/// </summary>
static void InitPeripheralsAndHandlers(void) {
	strncpy(mqtt_msg.message, "Hello from Azure Sphere using the MQTT-C client over wolfSSL ", sizeof(mqtt_msg.message));
	mqtt_msg.message_length = strlen(mqtt_msg.message);

	dx_timerSetStart(timerSet, NELEMS(timerSet));

	initialize_mqtt(publish_callback, mqtt_connected_cb, sub_topics, NELEMS(sub_topics));
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void) {
	dx_timerSetStop(timerSet, NELEMS(timerSet));
	dx_timerEventLoopStop();
}

int main(int argc, char* argv[]) {
	dx_registerTerminationHandler();
	InitPeripheralsAndHandlers();

	// Main loop
	while (!dx_isTerminationRequired()) {
		int result = EventLoop_Run(dx_timerGetEventLoop(), -1, true);
		// Continue if interrupted by signal, e.g. due to breakpoint being set.
		if (result == -1 && errno != EINTR) {
			dx_terminate(DX_ExitCode_Main_EventLoopFail);
		}
	}

	ClosePeripheralsAndHandlers();
	Log_Debug("Application exiting.\n");
	return dx_getTerminationExitCode();
}