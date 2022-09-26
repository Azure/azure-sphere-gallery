/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
   
#ifndef CURL_LOGS_H
#define CURL_LOGS_H

#include "logs.h"
#include <curl/curl.h>

/// <summary>
///     Logs a cURL multi error.
/// </summary>
/// <param name="message">The message to print</param>
/// <param name="curlErrCode">The cURL error code to describe</param>
void LogCurlMultiError(const char *message, CURLMcode code);

#endif