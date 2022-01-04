/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include "common.h"

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
    // <summary>DNS Aliases
    char *alias;
} ServiceInstanceDetails;

/// <summary>
/// Send a service discovery query
/// </summary>
/// <param name="svcName">Fully-qualified domain name of the DNS server</param>
/// <param name="fd">The socket file descriptor to send the DNS query to</param>
/// <returns>0 if succeeded, -1 if an error occurred.</returns>
int SendServiceDiscoveryQuery(const char *svcName, int fd);

/// <summary>
/// Send a service instance details query
/// </summary>
/// <param name="instanceName">The instance name to query details for</param>
/// <param name="fd">The socket file descriptor to send the DNS query to</param>
/// <returns>0 if succeeded, -1 if an error occurred.</returns>
int SendServiceInstanceDetailsQuery(const char *instanceName, int fd);

/// <summary>
/// Send a type a record query
/// </summary>
/// <param name="domainName">The domain name to query details for</param>
/// <param name="fd">The socket file descriptor to send the DNS query to</param>
/// <returns>0 if succeeded, -1 if an error occurred.</returns>
int SendARecordQuery(const char *domainName, int fd);

/// <summary>
/// Process pending data from a service discovery request
/// </summary>
/// <param name="fd">The socket file descriptor to receive the DNS response from</param>
/// <param name="instanceDetails">The instance details if it is available in the response</param>
/// <returns>0 if succeeded, -1 if an error occurred.</returns>
int ProcessDnsResponse(int fd, ServiceInstanceDetails **instanceDetails);

/// <summary>
/// Free memory used by a ServiceInstanceDetails
/// </summary>
/// <param name="instance">The ServiceInstanceDetails struct to free</param>
void FreeServiceInstanceDetails(ServiceInstanceDetails *instance);

/// <summary>
///     Set up SIGTERM termination handler and event handlers.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
ExitCode InitializeDNS(void);

/// <summary>
///     Check whether the required network connection status has been met.
/// </summary>
/// <param name="interface">The network interface to perform the check on.</param>
/// <param name="ipAddressAvailable">The result of the connection status check.</param>
/// <returns>0 on success, or -1 on failure.</returns>
int IsConnectionReady(const char *interface, bool *ipAddressAvailable);

/// <summary>
///     Handle DNS service discover response received event.
///     This function follows the <see cref="EventLoopIoCallback" /> signature.
/// </summary>
/// <param name="el"> The EventLoop in which the callback was registered </param>
/// <param name="fd"> The descriptor for which the event fired </param>
/// <param name="events"> The bitmask of events fired </param>
/// <param name="context"> An optional context pointer that was passed in the registration. </param>
void HandleReceivedDnsDiscoveryResponse(EventLoop *el, int fd, EventLoop_IoEvents events,
                                        void *context);

/// <summary>
///     The timer event handler to check whether network connection is ready.
/// </summary>
void ConnectionTimerEventHandler(EventLoopTimer *timer);

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="fdName">File descriptor name to use in error message</param>
void CloseFdAndPrintError(int fd, const char *fdName);

/// <summary>
///     Clean up the resources previously allocated for DNS resolver test.
/// </summary>
void DNSResolverCleanUp(void);

/// <summary>
///     Run DNS resolver diagnostic test.
/// </summary>
bool RunDNSDiagnostic(void);

/// <summary>
///     Print DNS resolver diagnostic summary.
/// </summary>
void PrintDNSSummary(void);