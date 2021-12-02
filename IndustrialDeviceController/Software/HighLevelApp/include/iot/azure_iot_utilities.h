/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <azureiot/iothubtransportmqtt.h>
#include <applibs/networking.h>
#include <azureiot/azure_sphere_provisioning.h>

/**
 * Sets up the client in order to establish the communication channel to Azure IoT Hub.
 * @param quota inflight message quota
 * @param context context to be used for c2d message, twin update and connection status callback from SDK layer
 * @return true if setup request been sent, need to wait for connection status callback to indicate if connection
 * been established.
 */
AZURE_SPHERE_PROV_RESULT azure_iot_setup_client(size_t quota, void *context);


/**
 * destroy iot client
 */
void azure_iot_destroy_client(void);

/**
 * Type of the function callback invoked to report whether the Device Twin properties
 * to the IoT Hub have been successfully delivered.
 * @param delivered whether device twin to iot been delivered
 * @param context user context set when request twin update
 */
typedef void (*device_twin_delivery_confirmation_func_t)(bool delivered, void *context);

/**
 * asynchronously report device twin properties change to iot hub, update result will be indicate in callback
 * @param properties json format properties
 * @param callback callback to be invoked to indicate result
 * @param context user context be pass along with callback.
 * @return 0 if request been successfully queued, -1 otherwise
 */
int azure_iot_twin_report_async(const char* properties, device_twin_delivery_confirmation_func_t callback, void *context);


/**
 * Type of the function callback invoked to report whether a message sent to the IoT Hub has
 * been successfully delivered or not.
 * @param delivered 'true' when the message has been successfully delivered.
 * @param context user context set when request to send d2c message
 */
typedef void (*message_delivery_confirmation_func_t)(bool delivered, void *context);


/**
 * Creates and enqueues a message to be delivered the IoT Hub. The message is not actually sent
 * immediately, but it is sent on the next invocation of AzureIoT_DoPeriodicTasks().
 * @param message The message to send
 * @param message_type The type of the message to send
 * @returns 0 if message been successfully queued, -1 otherwise
 */
int azure_iot_send_message_async(const char *message, const char* message_type, message_delivery_confirmation_func_t callback, void *context);

/**
 * Keeps IoT Hub Client alive by exchanging data with the Azure IoT Hub.
 * This function must to be invoked periodically so that the Azure IoT Hub
 * SDK can accomplish its work (e.g. sending messages, invokation of callbacks, reconnection
 * attempts, and so forth).
 */
void azure_iot_do_periodic_tasks(void);

/**
 * Type of the function callback invoked whenever a message is received from IoT Hub.
 * @param message c2d message been received, not null terminated
 * @param message_len message length received
 * @param message_type message type received
 * @param context user context set when setup client
 */
typedef void (*message_received_func_t)(const char *message, size_t message_len, const char *message_type, void *context);

/**
 * Sets a callback function invoked whenever a message is received from IoT Hub.
 * @param callback The callback function invoked when a c2d message is received
 */
void azure_iot_set_message_received_callback(message_received_func_t callback);

/**
 * Type of the function callback invoked whenever a Device Twin update from the IoT Hub is
 * received.
 * @param properties device twin properties from iot hub, not null terminated
 * @param properties_len properties length
 * @param context user context set when setup client
 */
typedef void (*device_twin_update_func_t)(const char* properties, size_t properties_len, void *context);

/**
 * Sets the function callback invoked whenever a Device Twin update from the IoT Hub is
 * received.
 * @param callback The callback function invoked when a Device Twin update is received
 */
void azure_iot_set_device_twin_update_callback(device_twin_update_func_t callback);

/**
 * Type of the function callback invoked when a Direct Method call from the IoT Hub is
 * received.
 * @param direct_method_name The name of the direct method to invoke
 * @param payload The payload of the direct method call
 * @param payload_size The payload size of the direct method call
 * @param response_payload The payload of the response provided by the callee. It must be
 * either NULL or an heap allocated string
 * @param response_payload_size The size of the response payload provided by the
 * callee.
 * @returns The HTTP status code. e.g. 404 for method not found.
 */
typedef int (*direct_method_call_func_t)(const char *direct_method_name, const char *payload,
                                      size_t payload_size, char **response_payload,
                                      size_t *response_payload_size, void *context);

/**
 * Sets the function to be invoked whenever a Direct Method call from the IoT Hub is received.
 * @param callback The callback function invoked when a Direct Method call is
 * received
 */
void azure_iot_set_direct_method_callback(direct_method_call_func_t callback);

/**
 * Type of the function callback invoked when the IoT Hub connection status changes.
 * @param connected indicate if connected to iot hub been established or lost
 * @param reason connection lost reason
 * @param context user context set when setup client
 */
typedef void (*connection_status_func_t)(bool connected, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void *context);


/**
 * Sets the function to be invoked whenever the connection status to thye IoT Hub changes.
 * @param callback The callback function invoked when Azure IoT Hub network connection
 * status changes.
 */
void azure_iot_set_connection_status_callback(connection_status_func_t callback);


/**
 * Initializes the Azure IoT Hub SDK.
 * @return 'true' if initialization has been successful
 */
bool azure_iot_initialize(void);


/**
 * Deinitializes the Azure IoT Hub SDK
 */
void azure_iot_deinitialize(void);


/**
 * @return whether adapter connected to iot hub
 */
bool azure_iot_is_connected(void);