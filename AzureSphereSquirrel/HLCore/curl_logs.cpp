/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
   
#include "curl_logs.h"

void LogCurlMultiError(const char *message, CURLMcode code)
{
    Log_Debug(message);
    Log_Debug(" (curl multi err=%d, '%s')\n", code, curl_multi_strerror(code));
}