/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdio.h>
#include "mqtt.h"

// Configuration for connecting to Azure Event Grid.
#define EVENT_GRID_MQTT_PORT                "8883"
#define EVENT_GRID_CA_CERTIFICATE           "Certs/DigiCertGlobalRootG2CA.pem"
#define EVENT_GRID_PUBLISH_TOPIC            "devices/${client.authenticationName}/telemetry"
#define EVENT_GRID_SUBSCRIBE_TOPIC          "devices/${client.authenticationName}/telemetry"
#define AUTHENTICATION_NAME_KEYWORD         "${client.authenticationName}"

// Buffer sizes
#define DATETIME_BUFFER_SIZE                128
#define SEND_BUFFER_SIZE                    512   // SEND_BUFFER_SIZE should be large enough to hold multiple whole MQTT messages
#define RECEIVE_BUFFER_SIZE                 512   // RECEIVE_BUFFER_SIZE should be large enough any whole MQTT message expected to be received
#define DEVICE_ID_BUFFER_SIZE               130
#define MESSAGE_BUFFER_SIZE                 128
#define TOPIC_BUFFER_SIZE                   256
#define MQTT_MESSAGE_QOS                    MQTT_PUBLISH_QOS_1

typedef struct {
    const char *port;
    const char *hostname;
    const char *publishTopic;
    const char *subscribeTopic;
    const char *ca_cert;
    int messageSize;
    int topicSize;
    int messageQOS;
    uint8_t sendbuf[SEND_BUFFER_SIZE];
    uint8_t recvbuf[RECEIVE_BUFFER_SIZE];

} MQTT_Context;

typedef struct {
    char message[MESSAGE_BUFFER_SIZE];
    size_t message_length;
} MQTT_MESSAGE;
