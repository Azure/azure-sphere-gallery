/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

/// <summary>
/// Termination codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_Init_EventLoop = 2,
    ExitCode_Main_EventLoopFail = 3,

    ExitCode_IsNetworkingReady_Failed = 4,

    ExitCode_Init_PublishMessageTimer = 5,
    ExitCode_Init_MqttPingTimer = 6,
    ExitCode_Init_ReconnectTimer = 7,

    ExitCode_PublishMessageTimer_Consume = 8,
    ExitCode_MqttPingTimer_Consume = 9,
    ExitCode_ReconnectTimer_Consume = 10,

    ExitCode_ConnectRaw_InvalidHostName = 11,
    ExitCode_ConnectRaw_GetAddrInfo = 12,
    ExitCode_ConnectRaw_GetAddrInfo_Result = 13,
    ExitCode_ConnectRaw_Socket = 14,
    ExitCode_ConnectRaw_EventReg = 15,
    ExitCode_ConnectRaw_Connect = 16,


    ExitCode_Reconnect_CreateTimer = 17,
    ExitCode_LoadDeviceCertificate = 18,

    ExitCode_HandleWolfSslSetup_Failed = 19,
    ExitCode_HandleWolfSslSetup_Init = 20,
    ExitCode_HandleWolfSslSetup_Method = 21,
    ExitCode_HandleWolfSslSetup_Context = 22,
    ExitCode_HandleWolfSslSetup_CertPath = 23,
    ExitCode_HandleWolfSslSetup_DeviceCertPath = 24,
    ExitCode_HandleWolfSslSetup_VerifyLocations = 25,
    ExitCode_HandleWolfSslSetup_UseSni = 26,
    ExitCode_HandleWolfSslSetup_Session = 27,
    ExitCode_HandleWolfSslSetup_CheckDomainName = 28,
    ExitCode_HandleWolfSslSetup_SetFd = 29,

    ExitCode_TlsHandshake_ModifyEvents = 30,
    ExitCode_TlsHandshake_Fail = 31,
    ExitCode_TlsHandshake_UnexpectedError = 32,

    ExitCode_MqttConnection_ModifyEvents = 33,
    ExitCode_MqttConnection_UnregisterIO = 34,
    ExitCode_MqttConnection_RegisterIO = 35,

    ExitCode_Validate_Hostname = 36,

    ExitCode_FormatTopic_DeviceID = 37,
    ExitCode_FormatTopic_Size = 38,
    ExitCode_FormatTopic_NullTopic = 39,
    ExitCode_FormatTopic_NullFormattedTopic = 40,

    ExitCode_SetSubscription_NullTopic = 41,

    ExitCode_SendTelemetry_NullTopic = 42

} ExitCode;

/// <summary>
/// Callback type for a function to be invoked when a fatal error with exit code needs to be raised.
/// </summary>
/// <param name="exitCode">Exit code representing the failure.</param>
typedef void (*ExitCode_CallbackType)(ExitCode exitCode);
