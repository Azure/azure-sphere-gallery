/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdio.h>
#include <stdlib.h>

#include <applibs/networking.h>
#include <applibs/application.h>
#include <tlsutils/deviceauth.h>

#include "exitcodes.h"
#include "eventgrid_config.h"
#include "eventloop_timer_utilities.h"
#include "mqtt_connection.h"

static char deviceId[DEVICE_ID_BUFFER_SIZE]; // Device ID is 128 bytes
static const char *deviceCertPath = NULL;

// Function to run the next time an IO event occurs.
static ExitCode (*nextHandler)(void);

static ExitCode HandleTlsHandshake(void);
static void HandleMqttConnection(void);
static void HandleSockEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);
static void ClientRefresherHandler(EventLoop *el, int fd, EventLoop_IoEvents events, void *context);
static ExitCode HandleWolfsslSetup(void);
static void MqttPingHandler(EventLoopTimer* eventLoopTimer);
static void MqttReconnectHandler(EventLoopTimer* eventLoopTimer);
static void MqttSetSubscriptions(const char *topic, size_t topicSize);
static void ReconnectClient(struct mqtt_client* client, void** reconnect_state_vptr);
static void StartOneShotTimer(EventLoopTimer *timer, const struct timespec *delay);
static ExitCode LoadDeviceCertPathAndDeviceID(WOLFSSL_CTX *ctx);
static ExitCode FormatTopic(const char *topic, size_t topicSize, char *formattedTopicBuffer,
                                 size_t formattedTopicBufferSize);
static bool IsNetworkReady(void);
static void FreeResources(void);

static ExitCode_CallbackType failureCallbackFunction = NULL;
static EventLoopTimer *mqttReconnectTimer = NULL;
static EventLoopTimer *mqttPingTimer = NULL;

static WOLFSSL_CTX *wolfSslCtx = NULL;
static WOLFSSL *wolfSslSession = NULL;
static bool wolfSslInitialized = false;
static int sockFd = -1;
static EventRegistration *sockReg = NULL;
static Mqtt_Reconnect_State *mqttReconnectStatePtr = NULL;

static bool isMqttConnected = false;
static char formattedPublishTopicBuffer[TOPIC_BUFFER_SIZE] = {0};
static char formattedSubscribeTopicBuffer[TOPIC_BUFFER_SIZE] = {0};
static EventLoop *eventLoopRef = NULL;
static MQTT_Context *mqttClientContext = NULL;
static struct mqtt_client mqttClient;

/// <summary>
/// Function to check if networking is ready.
/// </summary>
/// <param name=""></param>
/// <returns></returns>
bool IsNetworkReady(void)
{
    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) != 0) {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno, strerror(errno));
        failureCallbackFunction(ExitCode_IsNetworkingReady_Failed);
    }

    return isNetworkReady;
}

static void StartOneShotTimer(EventLoopTimer *timer, const struct timespec *delay)
{
    if (SetEventLoopTimerOneShot(timer, delay) == -1) {
        Log_Debug("ERROR: Failed to start provisioning timeout timer: %s (%d)\n", strerror(errno),
                  errno);
        failureCallbackFunction(ExitCode_Reconnect_CreateTimer);
        return;
    }
}

void DisconnectMqtt(void) {
    mqtt_disconnect(&mqttClient);

    isMqttConnected = false;
    FreeResources();
}

static void MqttPingHandler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
        failureCallbackFunction(ExitCode_MqttPingTimer_Consume);
        return;
    }
    if (isMqttConnected) {
        mqtt_ping(&mqttClient);
    }
}

const char *GetPublishTopicName(void)
{
    return formattedPublishTopicBuffer;
}

/// <summary>
/// Publish message to Azure Event Grid
/// </summary>
/// <param name="data">Message to publish</param>
/// <param name="data_length">Length of message to publish</param>
/// <param name="topic">Topic to publish the message on</param>
void SendTelemetry(const void *data, size_t data_length, const char *topic)
{
    if (!IsNetworkReady()) {
        Log_Debug("Network is not ready. Cannot send telemetry.\n");
        return;
    }

    if (!isMqttConnected) {
        Log_Debug("Not connected to Azure Event Grid. Not sending telemetry.\n");
        return;
    }

    if (!topic || !strlen(topic)) {
        Log_Debug("Publish topic is null or empty. Not sending telemetry.\n");
        failureCallbackFunction(ExitCode_SendTelemetry_NullTopic);
    }

    mqtt_mq_clean(&mqttClient.mq);

    if (mqttClient.mq.curr_sz >= data_length) {
        mqtt_publish(&mqttClient, topic, data, data_length, MQTT_MESSAGE_QOS);
    }

    mqtt_sync(&mqttClient);
}

/// <summary>
/// Function to replace "${client.authenticationName}" keyword (if it is present) with the actual 
/// device ID.
/// </summary>
/// <param name="topic">Topic to format</param>
/// <param name="formattedTopicBuffer">Buffer to hold the topic after formatting</param>
static ExitCode FormatTopic(const char *topic, size_t topicSize, char *formattedTopicBuffer,
                                 size_t formattedTopicBufferSize)
{
    char* authName = NULL;

    if (!topic || !topicSize || !strnlen(topic, topicSize)) {
        Log_Debug("FormatTopic: Topic is null or empty.\n");
        return ExitCode_FormatTopic_NullTopic;
    }

    if (!formattedTopicBuffer) {
        Log_Debug("FormatTopic: formattedTopicBuffer is null.\n");
        return ExitCode_FormatTopic_NullFormattedTopic;
    }

    memset(formattedTopicBuffer, 0, formattedTopicBufferSize);
    authName = strstr(topic, AUTHENTICATION_NAME_KEYWORD);

    // Keyword is not present
    if (authName == NULL) {
        memcpy(formattedTopicBuffer, topic, strlen(topic));
        return ExitCode_Success;
    }

    // Keyword is present. Need to replace keyword with device ID.

    if (deviceId == NULL) {
        Log_Debug("FormatTopic: Device ID is null.\n");
        return ExitCode_FormatTopic_DeviceID;
    }

    size_t authNameKeywordStartPos = (size_t)(authName - topic);
    strncpy(formattedTopicBuffer, topic, authNameKeywordStartPos);
    strncat(formattedTopicBuffer, deviceId, DEVICE_ID_BUFFER_SIZE);

    size_t remainingTopicLen =
        strnlen(topic, topicSize) - authNameKeywordStartPos -
        strlen(AUTHENTICATION_NAME_KEYWORD);

    if (remainingTopicLen > 0) {
        const char *remainingTopicStartOffset =
            topic + authNameKeywordStartPos + strlen(AUTHENTICATION_NAME_KEYWORD);

        strncat(formattedTopicBuffer, remainingTopicStartOffset, remainingTopicLen);
    }

    // Check that formatted topic length does not exceed the size of the buffer.
    if (strnlen(formattedTopicBuffer, formattedTopicBufferSize) >= formattedTopicBufferSize) {
        Log_Debug("ERROR: Formatted topic for '%s' is longer than the buffer size for the formatted topic.\n",
            topic);
        return ExitCode_FormatTopic_Size;
    }

    return ExitCode_Success;
}

/// <summary>
/// Check status of connection to MQTT Broker
/// </summary>
static void MqttReconnectHandler(EventLoopTimer *eventLoopTimer)
{
    if (ConsumeEventLoopTimerEvent(eventLoopTimer) != 0) {
        failureCallbackFunction(ExitCode_ReconnectTimer_Consume);
        return;
    }

    ReconnectClient(&mqttClient, &mqttClient.reconnect_state);
    return;
}

static void ClientRefresherHandler(EventLoop* el, int fd, EventLoop_IoEvents events, void* context) {
    if (IsNetworkReady()) {
        mqtt_sync(&mqttClient);
    } else {
        const struct timespec reconnectTime = {.tv_sec = 1, .tv_nsec = 0};
        StartOneShotTimer(mqttReconnectTimer, &reconnectTime);
    }
}


/// <summary>
///     This function is called from the event loop when a read or write event occurs
///     on the underlying socket. It calls the function whose address is in nextHandler.
/// </summary>
static void HandleSockEvent(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
    int retExitCode = nextHandler();
    if (retExitCode != ExitCode_Success) {
        failureCallbackFunction(retExitCode);
    }
}

/// <summary>
///     Open an AF_INET socket and starts an asynchronous connection
///     to the server's HTTPS port.
/// </summary>
static ExitCode ConnectRawSocketToServer(const char *hostname)
{
    struct addrinfo hints;
    int rc = 0;
    struct sockaddr_in addr;
    struct addrinfo *result = NULL;

    if (hostname == NULL) {
        Log_Debug("ConnectRawSocketToServer: Hostname is null.\n");
        return ExitCode_ConnectRaw_InvalidHostName;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    rc = getaddrinfo(hostname, NULL, &hints, &result);
    if (rc == 0) {
        struct addrinfo *res = result;

        if (result == NULL) {
            return ExitCode_ConnectRaw_GetAddrInfo_Result;
        }

        while (res) {
            if (res->ai_family == AF_INET) {
                break;
            }
            res = res->ai_next;
        }
        if (res) {
            addr.sin_port = htons((uint16_t)atoi(EVENT_GRID_MQTT_PORT));
            addr.sin_family = AF_INET;
            addr.sin_addr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr;
        } else {
            rc = -1;
        }
        freeaddrinfo(result);
    }
    if (rc != 0) {
        return ExitCode_ConnectRaw_GetAddrInfo;
    }

    sockFd = socket(addr.sin_family, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockFd == -1) {
        return ExitCode_ConnectRaw_Socket;
    }

    sockReg = EventLoop_RegisterIo(eventLoopRef, sockFd, EventLoop_Output, HandleSockEvent,
                                   /* context */ NULL);
    if (sockReg == NULL) {
        return ExitCode_ConnectRaw_EventReg;
    }

    int r = connect(sockFd, (struct sockaddr *)&addr, sizeof(addr));
    if (r != 0 && errno != EINPROGRESS) {
        return ExitCode_ConnectRaw_Connect;
    }

    nextHandler = HandleWolfsslSetup;

    return ExitCode_Success;
}

/// <summary>
///     Called from the event loop when socket connection has completed,
///     successfully or otherwise. If the connection was successful, then
///     uses wolfSSL to start the SSL handshake. Otherwise, set exitCode to
///     the appropriate value.
/// </summary>
static ExitCode HandleWolfsslSetup(void)
{
    int ret;
    // Check whether the connection succeeded.
    int error;
    socklen_t errSize = sizeof(error);
    int r = getsockopt(sockFd, SOL_SOCKET, SO_ERROR, &error, &errSize);
    if (!(r == 0 && error == 0)) {
        Log_Debug("ERROR: Socket connection failed\n");
        return ExitCode_HandleWolfSslSetup_Failed;
    }

    // Connection was made successfully, so allocate wolfSSL session and context.
    r = wolfSSL_Init();
    if (r != WOLFSSL_SUCCESS) {
        Log_Debug("ERROR: wolfSSL_init failed\n");
        return ExitCode_HandleWolfSslSetup_Init;
    }
    wolfSslInitialized = true;

    WOLFSSL_METHOD *wolfSslMethod = wolfTLSv1_3_client_method();
    if (wolfSslMethod == NULL) {
        Log_Debug("ERROR: failed to create WOLFSSL METHOD\n");
        return ExitCode_HandleWolfSslSetup_Method;
    }

    wolfSslCtx = wolfSSL_CTX_new(wolfSslMethod);
    if (wolfSslCtx == NULL) {
        Log_Debug("ERROR: failed to create WOLFSSL_CTX\n");
        return ExitCode_HandleWolfSslSetup_Context;
    }

    if (deviceCertPath == NULL) {
        Log_Debug("HandleWolfsslSetup: Device cert path is null.\n");
        return ExitCode_HandleWolfSslSetup_DeviceCertPath;
    }

    // Use device certificate for authentication
    if ((ret = wolfSSL_CTX_use_certificate_file(wolfSslCtx, deviceCertPath,
                                                WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        Log_Debug("ERROR: failed to use device certificate\n");
        return ExitCode_HandleWolfSslSetup_CertPath;
    }

    // Specify the root certificate which is used to validate the Azure Event Grid.
    char *certPathAbs =
        Storage_GetAbsolutePathInImagePackage(mqttClientContext->ca_cert);
    if (certPathAbs == NULL) {
        Log_Debug("ERROR: failed to get path to CA certificate\n");
        return ExitCode_HandleWolfSslSetup_CertPath;
    }

    if ((ret = wolfSSL_CTX_load_verify_locations(wolfSslCtx, certPathAbs, NULL)) != WOLFSSL_SUCCESS) {
        Log_Debug("ERROR: failed to load ca certificate\n");
        free(certPathAbs);
        certPathAbs = NULL;
        return ExitCode_HandleWolfSslSetup_VerifyLocations;
    }
    free(certPathAbs);
    certPathAbs = NULL;
    
    // Use Server Name Identification (SNI) as Azure Event Grid uses SNI. 
    ret = wolfSSL_CTX_UseSNI(wolfSslCtx, WOLFSSL_SNI_HOST_NAME, mqttClientContext->hostname,
                             (short unsigned int)strlen(mqttClientContext->hostname));

        if (ret != WOLFSSL_SUCCESS)
    {
        // sni usage failed
        Log_Debug("SNI usage failed\n");
        return ExitCode_HandleWolfSslSetup_UseSni;
    }

    wolfSslSession = wolfSSL_new(wolfSslCtx);
    if (wolfSslSession == NULL) {
        Log_Debug("ERROR: Failed to open new WOlfSsl session\n");
        return ExitCode_HandleWolfSslSetup_Session;
    }

    // Check domain name of peer certificate.
    r = wolfSSL_check_domain_name(wolfSslSession, mqttClientContext->hostname);
    if (r != WOLFSSL_SUCCESS) {
        Log_Debug("ERROR: wolfSSL_check_domain_name %d\n", r);
        return ExitCode_HandleWolfSslSetup_CheckDomainName;
    }

    // Associate socket with wolfSSL session.
    r = wolfSSL_set_fd(wolfSslSession, sockFd);
    if (r != WOLFSSL_SUCCESS) {
        Log_Debug("ERROR: wolfSSL_set_fd %d\n", r);
        return ExitCode_HandleWolfSslSetup_SetFd;
    }

    // Perform TLS handshake.
    // Asynchronous handshakes require repeated calls to wolfSSL_connect, so jump to the
    // handler to avoid repeating code.
    ret = HandleTlsHandshake();
    if (ret != ExitCode_Success) {
        return ret;
    }

    return ExitCode_Success;

}

/// <summary>
///     This function is called to start the TLS handshake. When an IO event occurs, the event loop
///     calls this function again to check whether the handshake has completed.
///     If the handshake completes successfully, this function then initiates the MQTT.
///     connection. If a fatal error occurs, sets exitCode to the appropriate value.
/// </summary>
static ExitCode HandleTlsHandshake(void)
{
    int r = EventLoop_ModifyIoEvents(eventLoopRef, sockReg, EventLoop_Input | EventLoop_Output);
    if (r != 0) {
        return ExitCode_TlsHandshake_ModifyEvents;
    }

    r = wolfSSL_connect(wolfSslSession);
    if (r != WOLFSSL_SUCCESS) {
        // If the handshake is in progress, exit to the event loop.
        const int uniqueError = wolfSSL_get_error(wolfSslSession, r);
        if (uniqueError == WOLFSSL_ERROR_WANT_READ || uniqueError == WOLFSSL_ERROR_WANT_WRITE) {
            nextHandler = HandleTlsHandshake;
            return ExitCode_Success;
        }

        // Unexpected error, so terminate.
        Log_Debug("ERROR: wolfSSL_connect %d\n", uniqueError);
        return ExitCode_TlsHandshake_UnexpectedError;
    }

    // Handshake completed, now handle mqtt connection.
    HandleMqttConnection();

    return ExitCode_Success;
}

/// <summary>
///     This function is called after the TLS handshake is successful, to initiate an MQTT 
///     connection and set the subscriptions.
/// </summary>
static void HandleMqttConnection(void)
{
    if (wolfSslSession == NULL) {
        const struct timespec reconnectTime = {.tv_sec = 2, .tv_nsec = 0};
        StartOneShotTimer(mqttReconnectTimer, &reconnectTime);
        Log_Debug("Failed to open socket: ");
        return;
    }

    int r = EventLoop_UnregisterIo(eventLoopRef, sockReg);
    if (r != 0) {
        failureCallbackFunction(ExitCode_MqttConnection_UnregisterIO);
        return;
    }

    sockReg = EventLoop_RegisterIo(eventLoopRef, sockFd, EventLoop_Input, ClientRefresherHandler, NULL);
    if (sockReg == NULL) {
        failureCallbackFunction(ExitCode_MqttConnection_RegisterIO);
        return;
    }

    // Reinitialize the client.
    mqtt_reinit(&mqttClient, wolfSslSession, mqttReconnectStatePtr->sendbuf,
                mqttReconnectStatePtr->sendbufsz, mqttReconnectStatePtr->recvbuf,
                mqttReconnectStatePtr->recvbufsz);

    // Send connection request to the broker.
    uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
    mqtt_connect(&mqttClient, deviceId, NULL, NULL, 0, deviceId, NULL, connect_flags, 30);

    // Subscribe to the desired topic.
    MqttSetSubscriptions(formattedSubscribeTopicBuffer, sizeof(formattedSubscribeTopicBuffer));
}

/// <summary>
///     Function to free all resources
/// </summary>
static void FreeResources(void)
{
    if (wolfSslSession != NULL) {
        wolfSSL_free(wolfSslSession);
        wolfSslSession = NULL;
    }
    if (wolfSslCtx != NULL) {
        wolfSSL_CTX_free(wolfSslCtx);
        wolfSslCtx = NULL;
    }
    if (wolfSslInitialized) {
        wolfSSL_Cleanup();
        wolfSslInitialized = false;
    }
    if (sockFd != -1) {
        close(sockFd);
        sockFd = -1;
    }

    if (sockReg != NULL) {
        EventLoop_UnregisterIo(eventLoopRef, sockReg);
        sockReg = NULL;
    }
}

/// <summary>
///     Function to dispose the timers created for the MQTT connection.
/// </summary>
void DisposeMqttTimers(void)
{
    DisposeEventLoopTimer(mqttReconnectTimer);
    DisposeEventLoopTimer(mqttPingTimer);
}

/// <summary>
///     Function to load the device certificate and device ID. Device ID is needed as the username
///     for the MQTT connection. Device certificate is needed to authenticate the device to the
///     Event Grid (MQTT broker).
/// </summary>
static ExitCode LoadDeviceCertPathAndDeviceID(WOLFSSL_CTX *ctx)
{
    WOLFSSL_X509 *deviceCert = NULL;
    bool isDeviceAuthReady = false;
    char localDeviceId[134] = {0};


    if (Application_IsDeviceAuthReady(&isDeviceAuthReady) != 0) {
        Log_Debug("ERROR: Device authentication could not be checked: %d (%s)\n", errno,
                  strerror(errno));
        goto cleanupLabel;
    }

    if (!isDeviceAuthReady) {
        Log_Debug("ERROR: Device has not authenticated: %d (%s)\n", errno, strerror(errno));
        goto cleanupLabel;
    }

    deviceCertPath = DeviceAuth_GetCertificatePath();
    if (deviceCertPath == NULL) {
        Log_Debug("ERROR: DeviceAuth_GetCertificatePath: %d (%s)\n", errno, strerror(errno));
        goto cleanupLabel;
    }

    deviceCert = wolfSSL_X509_load_certificate_file(deviceCertPath, WOLFSSL_FILETYPE_PEM);
    if (deviceCert == NULL) {
        Log_Debug("wolfSSL_X509_load_certificate_file error %d (%s)\n", errno, strerror(errno));
        goto cleanupLabel;
    }

    WOLFSSL_X509_NAME *subjectName = wolfSSL_X509_get_subject_name(deviceCert);
    if (subjectName == NULL) {
        Log_Debug("ERROR: invalid subject name: %d (%s)\n", errno, strerror(errno));
        goto cleanupLabel;
    }

    if (wolfSSL_X509_NAME_oneline(subjectName, (char *)&localDeviceId, sizeof(localDeviceId)) < 0) {
        Log_Debug("ERROR: Failed to get device id: %d (%s)\n", errno, strerror(errno));
        goto cleanupLabel;
    }

    snprintf(deviceId, DEVICE_ID_BUFFER_SIZE, "%s", localDeviceId + 4);

    if (deviceCert != NULL) {
        wolfSSL_X509_free(deviceCert);
        deviceCert = NULL;
    }

    return ExitCode_Success;

cleanupLabel:
    if (deviceCert != NULL) {
        wolfSSL_X509_free(deviceCert);
        deviceCert = NULL;
    }

    return ExitCode_LoadDeviceCertificate;
}

/// <summary>
///     Creates an MQTT client connection to Event Grid and sets the subscriptions.
///     APIs are called in the following order:
///      - ConnectRawSocketToServer
///      - HandleWolfsslSetup
///      - HandleTlsHandshake
///      - HandleMqttConnection
///      - MqttSetSubscriptions
///     This function is also called as a callback from MQTT-C library to connect/reconnect
///     to Event Grid.
/// </summary>
static void ReconnectClient(struct mqtt_client *client, void **reconnect_state_vptr)
{
    int retExitCode = -1;

    mqttReconnectStatePtr = *((Mqtt_Reconnect_State **)reconnect_state_vptr);

    if (!IsNetworkReady()) {
        const struct timespec reconnectTime = {.tv_sec = 2, .tv_nsec = 0};
        StartOneShotTimer(mqttReconnectTimer, &reconnectTime);
        Log_Debug("Network not ready.\n");
        return;
    }

    /* Perform error handling here. */
    if (client->error != MQTT_ERROR_INITIAL_RECONNECT) {
        Log_Debug("reconnect_client: called while client was in error state \"%s\"\n",
            mqtt_error_str(client->error)
        );
    }

    FreeResources();

    retExitCode = ConnectRawSocketToServer(mqttClientContext->hostname);
    if (retExitCode != ExitCode_Success) {
        Log_Debug("ERROR: ConnectRawSocketToServer: exitcode= %d, %d (%s)\n", retExitCode, errno,
                  strerror(errno));
        failureCallbackFunction(retExitCode);
        return;
    }

    return;
}

static void MqttSetSubscriptions(const char* topic, size_t topicSize)
{
    isMqttConnected = false;

    if (!topic || !topicSize || !strnlen(topic, topicSize)) {
        Log_Debug("Subscribe topic is null or empty.\n");
        failureCallbackFunction(ExitCode_SetSubscription_NullTopic);
    }

    if (mqttClient.error == MQTT_OK) {
        mqtt_subscribe(&mqttClient, topic, MQTT_MESSAGE_QOS);
     
        isMqttConnected = true;
        Log_Debug("Connected to MQTT Broker\n");
    }
}


/// <summary>
///     Init MQTT connection and subscribe to desired topics
/// </summary>
ExitCode InitializeMqtt(EventLoop *eventLoop,
                     void (*publish_callback)(void **unused, struct mqtt_response_publish *published),
                   ExitCode_CallbackType failureCallback,
                   MQTT_Context *mqttContext)
{
    static Mqtt_Reconnect_State reconnect_state = {.hostname = NULL,
                                                   .port = NULL,
                                                   .recvbuf = NULL,
                                                   .recvbufsz = 0,
                                                   .sendbuf = NULL,
                                                   .sendbufsz = 0,
                                                   .subtopic = NULL};

    eventLoopRef = eventLoop;
    failureCallbackFunction = failureCallback;
    mqttClientContext = mqttContext;

    int ret = LoadDeviceCertPathAndDeviceID(wolfSslCtx);
    if (ret != ExitCode_Success) {
        return ret;
    }

    // Format the publish and subscribe topic spaces to replace "${client.authenticationName}"
    // with the device ID.
    ret = FormatTopic(EVENT_GRID_PUBLISH_TOPIC, strlen(EVENT_GRID_PUBLISH_TOPIC),
                      formattedPublishTopicBuffer,
                      sizeof(formattedPublishTopicBuffer));
    if (ret != ExitCode_Success) {
        return ret;
    }
    FormatTopic(EVENT_GRID_SUBSCRIBE_TOPIC, strlen(EVENT_GRID_PUBLISH_TOPIC),
                formattedSubscribeTopicBuffer,
                sizeof(formattedSubscribeTopicBuffer));
    if (ret != ExitCode_Success) {
        return ret;
    }

    // Build the reconnect_state structure which will be passed to reconnect
    reconnect_state.hostname = mqttClientContext->hostname;
    reconnect_state.port = mqttClientContext->port;
    reconnect_state.subtopic = formattedSubscribeTopicBuffer;
    reconnect_state.sendbuf = mqttClientContext->sendbuf;
    reconnect_state.sendbufsz = sizeof(mqttClientContext->sendbuf);
    reconnect_state.recvbuf = mqttClientContext->recvbuf;
    reconnect_state.recvbufsz = sizeof(mqttClientContext->recvbuf);

    mqtt_init_reconnect(&mqttClient, ReconnectClient, &reconnect_state, publish_callback);

    return ExitCode_Success;
}


void ConnectMqtt(void)
{
    ReconnectClient(&mqttClient, &mqttClient.reconnect_state);
}

void CreateMqttTimers(void)
{
    mqttReconnectTimer = CreateEventLoopDisarmedTimer(eventLoopRef, MqttReconnectHandler);

    if (mqttReconnectTimer == NULL) {
        Log_Debug("ERROR: Failed to create provisioning timeout timer: %s (%d)\n",
                    strerror(errno), errno);
        failureCallbackFunction(ExitCode_Init_MqttPingTimer);
    }

    struct timespec pingTimerPeriod = {.tv_sec = 30, .tv_nsec = 0};
    mqttPingTimer = CreateEventLoopPeriodicTimer(eventLoopRef, &MqttPingHandler, &pingTimerPeriod);
    if (mqttPingTimer == NULL) {
        Log_Debug("ERROR: Failed to create provisioning timeout timer: %s (%d)\n", strerror(errno),
                  errno);
        failureCallbackFunction(ExitCode_Init_ReconnectTimer);
    }

    return;
}
