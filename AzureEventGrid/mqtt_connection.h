/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <applibs/eventloop.h>
#include <applibs/log.h>
#include <applibs/storage.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <wolfssl/ssl.h>

#include "mqtt.h"
#include "exitcodes.h"

/// <summary>
/// 
/// </summary>
typedef struct {
    const char *hostname;
    const char *port;
    const char *subtopic;
    uint8_t *sendbuf;
    size_t sendbufsz;
    uint8_t *recvbuf;
    size_t recvbufsz;
} Mqtt_Reconnect_State;

/// <summary>
/// Function to initialize all parameters needed for connecting to Azure Event Grid.
/// </summary>
/// <param name="eventLoop">Event loop reference</param>
/// <param name="publish_callback">Function to call when a message is received on subscribe topic</param>
/// <param name="failureCallback"></param>
/// <param name="mqttContext">MQTT context which has all configuration parameters needed for MQTT
/// connection</param>
/// <returns></returns>
ExitCode InitializeMqtt(EventLoop *eventLoop,
                        void (*publish_callback)(void **unused,
                                                 struct mqtt_response_publish *published),
                   ExitCode_CallbackType failureCallback,
                   MQTT_Context *mqttContext);

/// <summary>
/// Send telemetry message to the publish topic. Sends the message only if MQTT is connected.
/// If network is not ready, logs a message and disconnects.
/// </summary>
/// <param name="data">Telemetry message to send</param>
/// <param name="data_length">Length of message to send</param>
/// <param name="topic">Topic to publish the message on</param>
void SendTelemetry(const void *data, size_t data_length, const char *topic);

/// <summary>
/// Disconnect the MQTT connection. Called when application is exiting, or if network is lost.
/// </summary>
/// <param name=""></param>
void DisconnectMqtt(void);

/// <summary>
/// Start the MQTT connection.
/// </summary>
/// <param name=""></param>
void ConnectMqtt(void);

/// <summary>
/// Dispose the timers created for the MQTT connection.
/// </summary>
/// <param name=""></param>
void DisposeMqttTimers(void);

/// <summary>
/// Create the timers needed for the MQTT connection.
/// </summary>
/// <param name=""></param>
void CreateMqttTimers(void);

/// <summary>
/// Get the topic to publish to.
/// </summary>
/// <param name=""></param>
const char *GetPublishTopicName(void);
