/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "dns-helper.h"
#include <resolv.h>

#define DNS_SERVER_PORT 53
#define QUERY_BUF_SIZE 2048u
#define ANSWER_BUF_SIZE 2048u
#define DISPLAY_BUF_SIZE 256u
#define QUERY_RETRY_MAX 5
#define NCSI_RETRY_MAX 5

// File descriptors - initialized to invalid value
int dnsSocketFd = -1;
bool isNetworkStackReady = false;
EventLoopTimer *connectionTimer = NULL;
EventRegistration *dnsEventReg = NULL;
EventLoop *dnsEventLoop = NULL;

// If using DNS in an internet-connected network, consider setting the desired status to be
// Networking_InterfaceConnectionStatus_ConnectedToInternet instead.
const Networking_InterfaceConnectionStatus RequiredNetworkStatus =
    Networking_InterfaceConnectionStatus_IpAvailable;
const char *NetworkInterface = "wlan0";

// List of hostname to be tested
const char *ServerList[] = {"eastus-prod-azuresphere.azure-devices.net",
                            "prod.core.sphere.azure.net",
                            "prod.device.core.sphere.azure.net",
                            "prod.deviceauth.sphere.azure.net",
                            "prod.dinsights.core.sphere.azure.net",
                            "prod.releases.sphere.azure.net",
                            "prod.time.sphere.azure.net",
                            "prod.update.sphere.azure.net",
                            "prodmsimg.blob.core.windows.net",
                            "prodmsimg-secondary.blob.core.windows.net",
                            "prodptimg.blob.core.windows.net",
                            "prodptimg-secondary.blob.core.windows.net",
                            "sphere.sb.dl.delivery.mp.microsoft.com",
                            "www.msftconnecttest.com"};
const unsigned int ServerListLen = 17;
ServiceInstanceDetails *InstanceList[17];
int instanceIndex = 0;
int queryRetryCounter = 0;
int NCSIRetryCounter = 0;

int SendDnsQuery(const char *dName, int class, int type, int fd)
{
    char queryBuf[QUERY_BUF_SIZE];
    if (!dName) {
        Log_Debug("ERROR: Can't send DNS query as the domain name is null.\n");
        errno = EINVAL;
        return -1;
    }

    // Construct the DNS query to send
    int ret = res_init();
    if (ret) {
        Log_Debug("ERROR: res_init: %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    int messageSize =
        res_mkquery(ns_o_query, dName, class, type, NULL, 0, NULL, queryBuf, QUERY_BUF_SIZE);
    if (messageSize <= 0) {
        Log_Debug("ERROR: res_mkquery: %s (%d)\n", strerror(errno), errno);
        return -1;
    }

    // Send the constructed DNS query
    struct sockaddr_in si;
    memset(&si, 0, sizeof(si));
    si.sin_family = AF_INET;
    si.sin_port = htons(DNS_SERVER_PORT);
    si.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ret = sendto(fd, queryBuf, (size_t)messageSize, 0, (struct sockaddr *)&si, sizeof(si));
    if (ret == -1) {
        Log_Debug("ERROR: sendto: %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    return 0;
}

int SendServiceDiscoveryQuery(const char *dName, int fd)
{
    return SendDnsQuery(dName, ns_c_in, ns_t_ptr, fd);
}

int SendServiceInstanceDetailsQuery(const char *instanceName, int fd)
{
    return SendDnsQuery(instanceName, ns_c_in, ns_t_any, fd);
}

int SendARecordQuery(const char *domainName, int fd)
{
    return SendDnsQuery(domainName, ns_c_in, ns_t_a, fd);
}

int ProcessMessageBySection(char *buf, ssize_t len, ns_msg msg, ns_sect section,
                            ServiceInstanceDetails **instanceDetails)
{
    char displayBuf[DISPLAY_BUF_SIZE];
    ns_rr rr;
    int messageCount = ns_msg_count(msg, section);

    if (!(*instanceDetails)) {
        // Allocate for ServiceInstanceDetails and initialize its members.
        *instanceDetails = malloc(sizeof(ServiceInstanceDetails));
        if (!(*instanceDetails)) {
            Log_Debug("ERROR: recvfrom: %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        (*instanceDetails)->name = NULL;
        (*instanceDetails)->host = NULL;
        (*instanceDetails)->ipv4Address.s_addr = INADDR_NONE;
        (*instanceDetails)->txtData = NULL;
        (*instanceDetails)->txtDataLength = 0;
        (*instanceDetails)->alias = NULL;
    }

    // Parse each message
    for (int i = 0; i < messageCount; ++i) {
        if (ns_parserr(&msg, section, i, &rr)) {
            Log_Debug("ERROR: ns_parserr: %s (%d)\n", strerror(errno), errno);
            return -1;
        }
        switch (ns_rr_type(rr)) {
        case ns_t_ptr: {
            int compressedNameLength =
                dn_expand(buf, buf + len, ns_rr_rdata(rr), displayBuf, sizeof(displayBuf));
            if (compressedNameLength > 0 && !(*instanceDetails)->name) {
                (*instanceDetails)->name = strdup(displayBuf);
                if (!(*instanceDetails)->name) {
                    Log_Debug("ERROR: strdup for instance name failed: %s (%d)\n", strerror(errno),
                              errno);
                    return -1;
                }
            }
            break;
        }
        case ns_t_srv: {
            // Parse the SRV record and populate the port and host fields in instance details as per
            // DNS SRV record specification: https://tools.ietf.org/rfc/rfc2782.txt
            // SRV record format: Priority|  Weight |   Port  |     Target
            //                   (2 Bytes)|(2 Bytes)|(2 Bytes)|(Remaining Bytes)
            const char *data = ns_rr_rdata(rr);
            (*instanceDetails)->port =
                (uint16_t)ns_get16(data + sizeof(uint16_t) + sizeof(uint16_t));
            int compressedTargetDomainNameLength = dn_expand(
                buf, buf + len, data + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t),
                displayBuf, sizeof(displayBuf));
            if (compressedTargetDomainNameLength > 0 && !(*instanceDetails)->host) {
                (*instanceDetails)->host = strdup(displayBuf);
                if (!(*instanceDetails)->host) {
                    Log_Debug("ERROR: strdup: %s (%d)\n", strerror(errno), errno);
                    return -1;
                }
            }
            break;
        }
        case ns_t_txt: {
            // Populate name field in instance details if it hasn't been set
            if (!(*instanceDetails)->name) {
                (*instanceDetails)->name = strdup(ns_rr_name(rr));
                if (!(*instanceDetails)->name) {
                    Log_Debug("ERROR: strdup(ns_rr_name(rr)): %s (%d)\n", strerror(errno), errno);
                    return -1;
                }
            }

            // Get TXT record, populate txtData and txtDataLength fields in instance details
            if (!(*instanceDetails)->txtData) {
                (*instanceDetails)->txtData =
                    malloc(sizeof(unsigned char) * (size_t)(ns_rr_rdlen(rr)));
                if (!(*instanceDetails)->txtData) {
                    Log_Debug("ERROR: malloc: %s (%d)\n", strerror(errno), errno);
                    return -1;
                }
                memcpy((*instanceDetails)->txtData, ns_rr_rdata(rr), ns_rr_rdlen(rr));
                (*instanceDetails)->txtDataLength = ns_rr_rdlen(rr);
            }
            break;
        }
        case ns_t_a: {
            // Get A record (host address), populate ipv4Address field in instance details
            if (ns_rr_rdlen(rr) == sizeof((*instanceDetails)->ipv4Address)) {
                if ((*instanceDetails)->ipv4Address.s_addr == INADDR_NONE) {
                    memcpy(&(*instanceDetails)->ipv4Address.s_addr, ns_rr_rdata(rr),
                           ns_rr_rdlen(rr));
                }
            } else {
                Log_Debug("ERROR: Invalid DNS A record length: %d\n", ns_rr_rdlen(rr));
            }
            break;
        }
        case ns_t_cname: {
            char answerBuf[ANSWER_BUF_SIZE];
            const char *rd = ns_rr_rdata(rr);
            // Uncompress original name
            int n = ns_name_uncompress(ns_msg_base(msg), ns_msg_end(msg), rd, answerBuf,
                                       sizeof(answerBuf));
            if (n < 0) {
                Log_Debug(
                    "ERROR: Fail to uncompress original name from cname record. bufsize: %d, "
                    "errno: %s (%d)\n",
                    sizeof(answerBuf), strerror(errno), errno);
            }
            (*instanceDetails)->name = strdup(answerBuf);
            // Aggregate alias information
            if (!((*instanceDetails)->alias)) {
                (*instanceDetails)->alias = strdup(ns_rr_name(rr));
            } else {
                char *indent = "\n\t\t";
                char *original = (*instanceDetails)->alias;
                size_t suffixLen = strlen(ns_rr_name(rr));
                size_t originLen = strlen(original);
                size_t indentLen = strlen(indent);
                char *updated = (char *)malloc(suffixLen + originLen + indentLen + 1);
                memcpy(updated, original, originLen);
                memcpy(updated + originLen, indent, indentLen);
                memcpy(updated + originLen + indentLen, ns_rr_name(rr), suffixLen);
                updated[suffixLen + originLen + indentLen] = '\0';
                (*instanceDetails)->alias = updated;
            }
            break;
        }
        default:
            break;
        }
    }
    return 0;
}

int ProcessDnsResponse(int fd, ServiceInstanceDetails **instanceDetails)
{
    Log_Debug("EVENT: Process DNS Response.\n");
    char answerBuf[ANSWER_BUF_SIZE];
    ns_msg msg;
    struct sockaddr_in socketAddress;
    socklen_t addrLength = sizeof(socketAddress);
    ssize_t len =
        recvfrom(fd, answerBuf, ANSWER_BUF_SIZE, 0, (struct sockaddr *)&socketAddress, &addrLength);
    if (len == -1) {
        Log_Debug("ERROR: recvfrom: %s (%d)\n", strerror(errno), errno);
        return -1;
    }

    // Check the response has come from the loopback address
    if (socketAddress.sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
        Log_Debug("ERROR: recvfrom unexpected address: %x\n", socketAddress.sin_addr);
        return -1;
    }

    // Decode received response
    if (ns_initparse(answerBuf, len, &msg) != 0) {
        goto fail;
    }
    if (ProcessMessageBySection(answerBuf, len, msg, ns_s_an, instanceDetails) != 0) {
        goto fail;
    }
    if (ProcessMessageBySection(answerBuf, len, msg, ns_s_ar, instanceDetails) != 0) {
        goto fail;
    }
    return 0;

fail:
    FreeServiceInstanceDetails(*instanceDetails);
    *instanceDetails = NULL;
    return -1;
}

void FreeServiceInstanceDetails(ServiceInstanceDetails *details)
{
    if (details) {
        if (details->name) {
            free(details->name);
        }
        if (details->host) {
            free(details->host);
        }
        if (details->txtData) {
            free(details->txtData);
        }
        if (details->alias) {
            free(details->alias);
        }
        free(details);
    }
}

ExitCode InitializeDNS(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    dnsEventLoop = EventLoop_Create();
    if (dnsEventLoop == NULL) {
        Log_Debug("ERROR: Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    dnsSocketFd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
    if (dnsSocketFd == -1) {
        Log_Debug("ERROR: Failed to create dnsSocketFd: %s (%d)\n", strerror(errno), errno);
        return ExitCode_Init_Socket;
    }

    // Check network interface status at the specified period until it is ready.
    static const struct timespec checkInterval = {.tv_sec = 1, .tv_nsec = 0};
    connectionTimer =
        CreateEventLoopPeriodicTimer(dnsEventLoop, &ConnectionTimerEventHandler, &checkInterval);
    if (connectionTimer == NULL) {
        return ExitCode_Init_ConnectionTimer;
    }

    return ExitCode_Success;
}

int IsConnectionReady(const char *interface, bool *ipAddressAvailable)
{
    Networking_InterfaceConnectionStatus status;
    if (Networking_GetInterfaceConnectionStatus(interface, &status) == 0) {
        Log_Debug("INFO: Network interface %s status: 0x%02x\n", interface, status);
        isNetworkStackReady = true;
    } else {
        if (errno == EAGAIN) {
            Log_Debug("INFO: The networking stack isn't ready yet, will try again later.\n");
            return 0;
        } else {
            Log_Debug("ERROR: Networking_GetInterfaceConnectionStatus: %s (%d)\n", strerror(errno),
                      errno);
            return -1;
        }
    }
    *ipAddressAvailable = (status & RequiredNetworkStatus) != 0;
    return 0;
}

void HandleReceivedDnsDiscoveryResponse(EventLoop *el, int fd, EventLoop_IoEvents events,
                                        void *context)
{
    // Read received DNS response over socket.
    // Process received response, if it contains PRT message, but no SRV or TXT message, send
    // instance details request for each PTR instance. Otherwise, get the instance details.
    ServiceInstanceDetails *details = NULL;
    int result = ProcessDnsResponse(dnsSocketFd, &details);
    if (result != 0) {
        goto fail;
    }
    if (details) {
        if (details->name) {
            Log_Debug("INFO: DNS Service Discovery has found a instance: %s\n", details->name);
            if (!details->host) {
                // Found real domain, send out type a record query
                if (details->alias) {
                    SendARecordQuery(details->name, dnsSocketFd);
                    Log_Debug("EVENT: Send Type A Record Query to: %s\n", details->name);
                    dnsEventReg = EventLoop_RegisterIo(dnsEventLoop, dnsSocketFd, EventLoop_Input,
                                                       &HandleReceivedDnsDiscoveryResponse,
                                                       /* context */ NULL);
                    InstanceList[instanceIndex] = details;
                    return;
                } else {
                    Log_Debug("INFO: Requesting SRV and TXT details for the instance.\n");
                    SendServiceInstanceDetailsQuery(details->name, dnsSocketFd);
                    FreeServiceInstanceDetails(details);
                }
            } else {
                // Found local domain instance
                InstanceList[instanceIndex] = details;
            }
        } else {
            // Update type a record response
            (InstanceList[instanceIndex])->ipv4Address = details->ipv4Address;
            FreeServiceInstanceDetails(details);
        }
    }

fail:
    if ((++instanceIndex) >= ServerListLen) {
        exitCode = ExitCode_Test_Finish;
    }
    // Initialize test for next server address
    exitCode = InitializeDNS();
    queryRetryCounter = 0;
}

void ConnectionTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ConnectionTimer_Consume;
        return;
    }

    // Check whether the network connection is ready.
    bool isConnectionReady = false;
    if (IsConnectionReady(NetworkInterface, &isConnectionReady) != 0) {
        exitCode = ExitCode_ConnectionTimer_ConnectionReady;
    } else if (isConnectionReady || (NCSIRetryCounter++) >= NCSI_RETRY_MAX) {
        // Connection is ready, unregister the connection event handler. Register DNS response
        // handler, then start DNS service discovery.
        bool success = false;
        NCSIRetryCounter = 0;
        if (DisarmEventLoopTimer(connectionTimer) == 0) {
            dnsEventReg = EventLoop_RegisterIo(dnsEventLoop, dnsSocketFd, EventLoop_Input,
                                               &HandleReceivedDnsDiscoveryResponse,
                                               /* context */ NULL);

            success = (dnsEventReg != NULL);
        }

        if (!success) {
            Log_Debug("ERROR: Connection Failure.\n");
            exitCode = ExitCode_ConnectionTimer_Disarm;
            return;
        }
        if (isConnectionReady) {
            Log_Debug("EVENT: Established Connection!\n");
        } else {
            Log_Debug("EVENT: Try DNS lookups despite the networking stack is not ready.\n");
        }
        SendServiceDiscoveryQuery(ServerList[instanceIndex], dnsSocketFd);
    }
}

void CloseFdAndPrintError(int fd, const char *fdName)
{
    if (fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}

void DNSResolverCleanUp(void)
{
    for (int i = 0; i < ServerListLen; ++i) {
        FreeServiceInstanceDetails(InstanceList[i]);
    }

    DisposeEventLoopTimer(connectionTimer);
    EventLoop_UnregisterIo(dnsEventLoop, dnsEventReg);
    EventLoop_Close(dnsEventLoop);

    CloseFdAndPrintError(dnsSocketFd, "DNS Socket");
}

bool RunDNSDiagnostic(void)
{
    exitCode = ExitCode_Success;
    for (int i = 0; i < ServerListLen; ++i) {
        InstanceList[i] = NULL;
    }

    Log_Debug("INFO: Starting DNS Test\n");
    instanceIndex = 0;
    queryRetryCounter = 0;

    exitCode = InitializeDNS();

    // DNS resolver loop
    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(dnsEventLoop, EVENT_LOOP_DURATION, true);
        if (exitCode == ExitCode_Success) {
            Log_Debug("Resolving address: %s\n", ServerList[instanceIndex]);
        }
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
        if ((queryRetryCounter++) >= QUERY_RETRY_MAX) {
            ++instanceIndex;
            queryRetryCounter = 0;
            exitCode = InitializeDNS();
        }
        if (instanceIndex >= ServerListLen) {
            exitCode = ExitCode_Test_Finish;
        }
    }
    for (int i = 0; i < ServerListLen; ++i) {
        if (!InstanceList[i]) {
            return false;
        }
    }
    return true;
}

void PrintDNSSummary(void)
{
    // Print out DNS hostname resolution list
    Log_Debug("\n\nDiagnostic App Summary:\nDNS Hostname Resolution List:\n");
    for (int i = 0; i < ServerListLen; ++i) {
        ServiceInstanceDetails *details = InstanceList[i];
        if (details != NULL) {
            if (details->host) {
                // Local instance
                Log_Debug(
                    "\tIndex: \t%d\n\tName: \t%s\n\tHost: \t%s\n\tIPv4: \t%s\n\tPort: \t%hd\n\tTXT "
                    "Data: \t%.*s\n\n",
                    i, details->name, details->host, inet_ntoa(details->ipv4Address), details->port,
                    details->txtDataLength, details->txtData);
            } else if (details->alias) {
                // CNAME record
                Log_Debug("\tIndex: \t%d\n\tName: \t%s\n\tIPv4: \t%s\n\tAlias: \t%s\n\n", i,
                          details->name, inet_ntoa(details->ipv4Address), details->alias);
            }
        } else if (instanceIndex >= i) {
            // Failed case
            Log_Debug("\tIndex: \t%d\n\tERROR: \tFailed to resolve: %s\n\n", i, ServerList[i]);
        } else {
            // Skipped test
            Log_Debug("\tIndex: \t%d\n\tSkip test with %s\n\n", i, ServerList[i]);
        }
    }
}