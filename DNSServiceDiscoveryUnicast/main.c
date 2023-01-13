/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application shows how to perform a DNS service discovery. It makes queries using
// multicast to local network and processes responses from the available DNS responders.
//
// It uses the API for the following Azure Sphere application libraries:
// - log (displays messages in the Device Output window during debugging)
// - networking (get network interface connection status)
// - eventloop (system invokes handlers for timer events)

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>

#include <applibs/log.h>
#include <applibs/networking.h>

#include <curl/curl.h>

#include "dns-sd.h"
#include "eventloop_timer_utilities.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_ConnectionTimer_Consume = 2,
    ExitCode_ConnectionTimer_ConnectionReady = 3,
    ExitCode_ConnectionTimer_Disarm = 4,

    ExitCode_Init_EventLoop = 5,
    ExitCode_Init_Socket = 6,

    ExitCode_Init_ConnectionTimer = 7,
    ExitCode_Init_DnsResponseHandler = 8,

    ExitCode_Main_EventLoopFail = 9
} ExitCode;

// File descriptors - initialized to invalid value
static bool isNetworkStackReady = false;

static EventLoop *eventLoop = NULL;
static EventLoopTimer *connectionTimer = NULL;
static EventRegistration *dnsEventReg = NULL;

static const Networking_InterfaceConnectionStatus RequiredNetworkStatus =
    Networking_InterfaceConnectionStatus_IpAvailable;
static const char NetworkInterface[] = "eth0";
static const char DnsSDServiceType[] = "_http._tcp.home";
static const char DnsSDServerIp[] = "10.0.0.1"; // Replace this with your DNS server for service discovery
static const char OtherDnsServerIp[] = "10.0.0.2"; // Replace this with a second DNS server for normal resolution


// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

static void DoFetch(const char* url);
static void DoQuery(void);
static void TerminationHandler(int signalNumber);
static void ConnectionTimerEventHandler(EventLoopTimer *timer);
static ExitCode InitializeAndStartDnsServiceDiscovery(void);
static void Cleanup(void);

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

static void DoFetch(const char* url)
{
    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      Log_Debug("curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    curl_easy_cleanup(curl);
}

#define ANSWER_BUF_SIZE 2048u
static void DoQuery(void)
{
    static char answerBuf[ANSWER_BUF_SIZE];
    int answerSize = SendServiceDiscoveryQuery(DnsSDServiceType, answerBuf, ANSWER_BUF_SIZE);
    if (answerSize < 0) {
        goto fail;
    }
    
    ServiceInstanceDetails *details = NULL;
    int result = ProcessDnsResponse(&details, answerBuf, answerSize);
    if (result != 0) {
        goto fail;
    }

    if (details && details->name) {
        Log_Debug("INFO: DNS Service Discovery has found an instance: %s.\n", details->name);
        if (!details->host) {
            Log_Debug("INFO: Requesting SRV and TXT details for the instance.\n");
            answerSize = SendServiceInstanceDetailsQuery(details->name, answerBuf, ANSWER_BUF_SIZE);
            result = ProcessDnsResponse(&details, answerBuf, answerSize);
            if (result != 0) {
                goto fail;
            }

            // NOTE: The TXT data is simply treated as a string and isn't parsed here. You should
            // replace this with your own production logic.
            Log_Debug("\tName: %s\n\tHost: %s\n\tIPv4 Address: %s\n\tPort: %hd\n\tTXT Data: %.*s\n",
                      details->name, details->host, inet_ntoa(details->ipv4Address), details->port,
                      details->txtDataLength, details->txtData);

            DoFetch(details->host);
        }
    }

fail:
    FreeServiceInstanceDetails(details);
}

/// <summary>
///     Check whether the required network connection status has been met.
/// </summary>
/// <param name="interface">The network interface to perform the check on.</param>
/// <param name="ipAddressAvailable">The result of the connection status check.</param>
/// <returns>0 on success, or -1 on failure.</returns>
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
            Log_Debug("ERROR: Networking_GetInterfaceConnectionStatus: %d (%s)\n", errno,
                      strerror(errno));
            return -1;
        }
    }
    *ipAddressAvailable = (status & RequiredNetworkStatus) != 0;
    return 0;
}

/// <summary>
///     The timer event handler to check whether network connection is ready and send DNS service
///     discovery queries.
/// </summary>
static void ConnectionTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ConnectionTimer_Consume;
        return;
    }

    // Check whether the network connection is ready.
    bool isConnectionReady = false;
    if (IsConnectionReady(NetworkInterface, &isConnectionReady) != 0) {
        exitCode = ExitCode_ConnectionTimer_ConnectionReady;
    } else if (isConnectionReady) {
        // Connection is ready, send a DNS service discovery query.
        DoQuery();
    }
}

/// <summary>
///     Set up SIGTERM termination handler and event handlers.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode InitializeAndStartDnsServiceDiscovery(void)
{
    // Configure DNS servers - you should specify at least the service discovery server,
    // and also a secondary if using the provided container (which does not permit recursive
    // lookups)
    static const size_t numOfDnsServerAddressSpecified = 2;
    static const char *dnsServerIpAddress[] = { DnsSDServerIp, OtherDnsServerIp };
    Networking_IpConfig ipConfig;

    // Convert the addresses from the numbers-and-dots notation into integers.
    struct in_addr dnsServers[numOfDnsServerAddressSpecified];
    for (int i = 0; i < numOfDnsServerAddressSpecified; i++) {
        if (inet_pton(AF_INET, dnsServerIpAddress[i], &dnsServers[i]) != 1) {
            Log_Debug("ERROR: Invalid DNS server address or address family specified.\n");
            return -1;
        }
    }

    Networking_IpConfig_Init(&ipConfig);

    int result =
        Networking_IpConfig_EnableCustomDns(&ipConfig, dnsServers, numOfDnsServerAddressSpecified);

    if (result != 0) {
        Log_Debug("ERROR: Networking_IpConfig_EnableCustomDns: %d (%s)\n", errno, strerror(errno));
        Networking_IpConfig_Destroy(&ipConfig);
        return -1;
    }

    result = Networking_IpConfig_Apply(NetworkInterface, &ipConfig);
    Networking_IpConfig_Destroy(&ipConfig);

    if (result != 0) {
        Log_Debug("ERROR: Networking_IpConfig_Apply: %d (%s)\n", errno, strerror(errno));
        return -1;
    }
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    // Check network interface status at the specified period until it is ready.
    // This also defines the frequency at which the sample sends out DNS-SD queries.
    static const struct timespec checkInterval = {.tv_sec = 10, .tv_nsec = 0};
    connectionTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &ConnectionTimerEventHandler, &checkInterval);
    if (connectionTimer == NULL) {
        return ExitCode_Init_ConnectionTimer;
    }

    return ExitCode_Success;
}

/// <summary>
///     Clean up the resources previously allocated.
/// </summary>
static void Cleanup(void)
{
    DisposeEventLoopTimer(connectionTimer);
    EventLoop_UnregisterIo(eventLoop, dnsEventReg);
    EventLoop_Close(eventLoop);

}

int main(void)
{
    Log_Debug("INFO: DNS Service Discovery sample starting.\n");

    exitCode = InitializeAndStartDnsServiceDiscovery();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    Cleanup();
    Log_Debug("INFO: Application exiting.\n");
    return exitCode;
}