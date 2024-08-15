/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <applibs/log.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "eventgrid_config.h"
#include "eventloop_timer_utilities.h"
#include "exitcodes.h"
#include "mqtt_connection.h"
#include "mqtt.h"
#include "options.h"

static volatile sig_atomic_t exitCode = ExitCode_Success;

// Timer / polling
static EventLoop *eventLoop = NULL;
static EventLoopTimer *publishMessageTimer = NULL;

static void PublishMessageTimerHandler(EventLoopTimer *eventLoopTimer);
static void ExitCodeCallbackHandler(ExitCode ec);
static void UpdateTelemetry(char *mqttMessageToPublish, size_t messageBufferSize);
static void PublishCallback(void **unused, struct mqtt_response_publish *published);
static bool BuildUtcDateTimeString(char *outputBuffer, size_t outputBufferSize, time_t t);


static MQTT_MESSAGE mqtt_msg;
static MQTT_Context mqttContext = {.hostname = NULL,
                            .port = "8883",
                            .publishTopic = EVENT_GRID_PUBLISH_TOPIC,
                            .subscribeTopic = EVENT_GRID_SUBSCRIBE_TOPIC,
                            .ca_cert = EVENT_GRID_CA_CERTIFICATE,
                            .messageSize = 128,
                            .messageQOS = MQTT_PUBLISH_QOS_1,
                            .topicSize = 256 };

/// <summary>
///     This function publishes a message to Azure Event Grid on configured topic
/// </summary>
static void PublishMessageTimerHandler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
        exitCode = ExitCode_PublishMessageTimer_Consume;
        return;
    }

    UpdateTelemetry(mqtt_msg.message, sizeof(mqtt_msg.message));
    mqtt_msg.message_length = strnlen(mqtt_msg.message, sizeof(mqtt_msg.message));

    SendTelemetry(mqtt_msg.message, mqtt_msg.message_length, GetPublishTopicName());
}

/// <summary>
///     This function is called when the device receives a new message from Azure Event Grid
/// </summary>
static void PublishCallback(void **unused, struct mqtt_response_publish *published)
{
    char *message = NULL;

    // Print the message received
    message = (char *)malloc(published->application_message_size + 1);
    if (!message) {
        Log_Debug("Error: Cannot print received message. Memory allocation failed.\n");
        return;
    }

    memset(message, 0x00, published->application_message_size + 1);
    memcpy(message, published->application_message, published->application_message_size);

    Log_Debug("Message Received: %s", message);

    free(message);
}

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

static void ExitCodeCallbackHandler(ExitCode ec)
{
    exitCode = ec;
}

/// <summary>
///     Add date and time and simulated temperature to the message that is to be published.
/// </summary>
static void UpdateTelemetry(char *mqttMessageToPublish, size_t messageBufferSize)
{
    static float temperature = 50.f;
    char dateTimeBuffer[DATETIME_BUFFER_SIZE] = {0};

    if (!mqttMessageToPublish || !messageBufferSize) {
        Log_Debug("Error: Publish message buffer is null or empty.\n");
        return;
    }

    memset(mqttMessageToPublish, 0, messageBufferSize);

    time_t now;
    time(&now);

    if (now != -1) {
        BuildUtcDateTimeString(dateTimeBuffer, sizeof(dateTimeBuffer), now);
    }

    // Generate a simulated temperature.
    float delta = ((float)(rand() % 41)) / 20.0f - 1.0f; // between -1.0 and +1.0
    temperature += delta;

    snprintf(mqttMessageToPublish, messageBufferSize, "%s: Temperature %f\n",
             dateTimeBuffer,
             temperature);
}

/// <summary>
///     Helper function to build the UTC date time string.
/// </summary>
static bool BuildUtcDateTimeString(char *outputBuffer, size_t outputBufferSize, time_t t)
{
    // Format string to create an ISO 8601 time.  This corresponds to the DTDL datetime schema item.
    static const char *ISO8601Format = "%Y-%m-%dT%H:%M:%SZ";

    bool result;

    struct tm *currentTimeTm;
    currentTimeTm = gmtime(&t);

    if (strftime(outputBuffer, outputBufferSize, ISO8601Format, currentTimeTm) == 0) {
        Log_Debug("ERROR: strftime: %s (%d)\n", errno, strerror(errno));
        result = false;
    } else {
        result = true;
    }

    return result;
}

/// <summary>
///     Initialize peripherals, device twins, direct methods, timers.
/// </summary>
static ExitCode InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    // Publish telemetry message every one second.
    struct timespec publishMessagePeriod = {.tv_sec = 1, .tv_nsec = 0};
    publishMessageTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &PublishMessageTimerHandler,
                                                       &publishMessagePeriod);
    if (publishMessageTimer == NULL) {
        return ExitCode_Init_PublishMessageTimer;
    }

    mqttContext.hostname = Options_GetAzureEventGridHostname();
    int ret = InitializeMqtt(eventLoop, PublishCallback, ExitCodeCallbackHandler,
                             &mqttContext);
    if (ret != ExitCode_Success) {
        return ret;
    }

    // Create timers needed for MQTT connection.
    CreateMqttTimers();

    return ExitCode_Success;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    DisconnectMqtt();
    DisposeMqttTimers();
    DisposeEventLoopTimer(publishMessageTimer);
    EventLoop_Close(eventLoop);
}

int main(int argc, char *argv[])
{
    Log_Debug("Azure Event Grid Application starting.\n");

    exitCode = Options_ParseArgs(argc, argv);
    if (exitCode != ExitCode_Success) {
        return exitCode;
    }

    exitCode = InitPeripheralsAndHandlers();
    
    if (exitCode == ExitCode_Success) {
        ConnectMqtt();
    }

    // Main loop
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");
    return exitCode;
}