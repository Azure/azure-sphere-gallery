/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>

/// <summary>
/// Data structure for a DNS instance details.
/// This should be created with <see cref="ProcessServiceInstanceDetailsResponse"/> and freed with
/// <see cref="FreeServiceInstanceDetails"/>.
/// </summary>
typedef struct {
    /// <summary>Service instance name</summary>
    char *name;
    /// <summary>Service host name</summary>
    char *host;
    /// <summary>IPv4 address</summary>
    struct in_addr ipv4Address;
    /// <summary>Network port</summary>
    uint16_t port;
    /// <summary>DNS TXT data</summary>
    char *txtData;
    /// <summary>DNS TXT data length</summary>
    uint16_t txtDataLength;
} ServiceInstanceDetails;

/// <summary>
/// Send a service discovery query
/// </summary>
/// <param name="dName">Domain to query</param>
/// <param name="answerBuf">Pointer to a buffer to receive the response</param>
/// <param name="answerBufSize">Size of answerBuf</param>
/// <returns>The number of bytes in the response if succeeded, -1 if an error occurred.</returns>
int SendServiceDiscoveryQuery(const char *dName, char* answerBuf, int answerBufSize);

/// <summary>
/// Send a service instance details query
/// </summary>
/// <param name="instanceName">The instance name to query details for</param>
/// <param name="answerBuf">Pointer to a buffer to receive the response</param>
/// <param name="answerBufSize">Size of answerBuf</param>
/// <returns>The number of bytes in the response if succeeded, -1 if an error occurred.</returns>
int SendServiceInstanceDetailsQuery(const char *instanceName, char* answerBuf, int answerBufSize);

/// <summary>
/// Process data from a service discovery response
/// </summary>
/// <param name="instanceDetails">The instance details if it is available in the response</param>
/// <param name="answerBuf">Pointer to a buffer containing the response</param>
/// <param name="answerBufSize">Size of answerBuf</param>
/// <returns>0 if succeeded, -1 if an error occurred.</returns>
int ProcessDnsResponse(ServiceInstanceDetails **instanceDetails, char* answerBuf, int answerBufSize);

/// <summary>
/// Free memory used by a ServiceInstanceDetails
/// </summary>
/// <param name="instance">The ServiceInstanceDetails struct to free</param>
void FreeServiceInstanceDetails(const ServiceInstanceDetails *instance);