/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "exitcodes.h"

typedef struct {
    char uuid[128];
    char filename[256];
    char url[1024];
    unsigned int size;
    unsigned int retries;
} DeviceUpdateRequest;

// This header describes a backend-agnostic interface to a cloud platform.
// An implementation of this header should implement logic to translate between business domain
// concepts and the specifics of the cloud backend.

typedef void (*Cloud_DesiredRecipeChangedCallbackType)(DeviceUpdateRequest request);

/// <summary>
/// Callback type for a function to be invoked when the cloud backend indicates that the
/// telemetry upload enabled state has changed.
/// </summary>
/// <param name="status">A boolean indicating whether telemetry upload should be enabled.</param>
typedef void (*Cloud_TelemetryUploadEnabledChangedCallbackType)(bool status);

/// <summary>
/// Callback type for a function to be invoked when the cloud backend requests and alert be
/// displayed.
/// </summary>
/// <param name="alertMessage">A NULL-terminated string containing the alert message.</param>
typedef void (*Cloud_DisplayAlertCallbackType)(const char *alertMessage);

/// <summary>
/// Callback type for a function to be invoked when the cloud backend indicates the connection
/// status has changed.
/// </summary>
/// <param name="connected">A boolean indicating whether the cloud connection is available.</param>
typedef void (*Cloud_ConnectionChangedCallbackType)(bool connected);

/// <summary>
/// An enum indicating possible result codes when performing cloud-related operations
/// </summary>
typedef enum {
    /// <summary>
    /// The operation succeeded
    /// </summary>
    Cloud_Result_OK = 0,

    /// <summary>
    /// The operation could not be performed as no network connection was available
    /// </summary>
    Cloud_Result_NoNetwork,

    /// <summary>
    /// The operation failed for another reason not explicitly listed
    /// </summary>
    Cloud_Result_OtherFailure
} Cloud_Result;

/// <summary>
/// Initialize the cloud connection.
/// </summary>
/// <param name="el">An EventLoop that events may be registered to.</param>
/// <param name="backendContext">
///     Backend-specific context to be passed to the backend during initialization.
/// </param>
/// <param name="failureCallback">Function called on unrecoverable failure.</param>
/// <param name="telemetryUploadEnabledChangedCallback">
///     Function called by the cloud backend to indicates the telemetry upload enabled status
///     has changed.
/// </param>
/// <param name="displayAlertCallback">
///     Function called when the cloud backend requests an alert be displayed.
/// </param>
/// <param name="connectionChangedCallback">
///     Function called when the status of the connection to the cloud backend changes.
/// </param>
/// <returns>An <see cref="ExitCode" /> indicating success or failure.</returns>
ExitCode Cloud_Initialize(
    EventLoop *el, void *backendContext, ExitCode_CallbackType failureCallback,
    Cloud_DesiredRecipeChangedCallbackType desiredRecipeChangedCallback,
    // Cloud_TelemetryUploadEnabledChangedCallbackType telemetryUploadEnabledChangedCallback,
    Cloud_DisplayAlertCallbackType displayAlertCallback,
    Cloud_ConnectionChangedCallbackType connectionChangedCallback);

/// <summary>
/// Disconnect and cleanup the cloud connection.
/// </summary>
void Cloud_Cleanup(void);

Cloud_Result Cloud_SendRecipeCampaignChangedEvent(DeviceUpdateRequest request, bool failed);

/// <summary>
/// Queue sending device details to the cloud
/// </summary>
/// <param name="serialNumber">The device's serial number</param>
/// <returns>A <see cref="Cloud_Result" /> indicating success or failure.</returns>
Cloud_Result Cloud_SendDeviceDetails(const char *serialNumber);
