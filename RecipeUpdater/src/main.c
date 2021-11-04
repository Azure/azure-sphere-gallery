#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <curl/curl.h>

#include "applibs_versions.h"

#include <applibs/eventloop.h>
#include <applibs/networking.h>
#include <applibs/log.h>
#include <applibs/storage.h>

#include "common/eventloop_timer_utilities.h"
#include "common/user_interface.h"
#include "common/exitcodes.h"
#include "common/cloud.h"
#include "common/options.h"
#include "common/connection_iot_hub.h"
#include "common/connection.h"

#define CURL_TIMEOUT_S 60

static volatile sig_atomic_t exitCode = ExitCode_Success;


// Initialization/Cleanup
static ExitCode InitPeripheralsAndHandlers(void);
static void ClosePeripheralsAndHandlers(void);

// Interface callbacks
static void ExitCodeCallbackHandler(ExitCode ec);

// Cloud
static const char *CloudResultToString(Cloud_Result result);
static void ConnectionChangedCallbackHandler(bool connected);
static void CloudDesiredRecipeChangedCallback(DeviceUpdateRequest request);
static void DisplayAlertCallbackHandler(const char *alertMessage);

static ExitCode CheckNetworkStatus(void);
static ExitCode ConfigureNetworkInterfaceWithStaticIp(const char *interfaceName);
static ExitCode StartSntpServer(const char *interfaceName);
static ExitCode ConfigureAndStartDhcpSever(const char *interfaceName);
static ExitCode CheckNetworkStackStatusAndLaunchServers(void);


// Timer / polling
static EventLoop *eventLoop = NULL;
static EventLoopTimer *telemetryTimer = NULL;

static bool isConnected = false;
static bool isNetworkStackReady = false;

static const char *serialNumber = "OVEN-01234";


static DeviceUpdateRequest failed_update = { 0 };

typedef struct {
    CURL *cloudHandle;
    CURL *localHandle;
    struct curl_httppost* formData;

    /**
     * @brief The path of the root SSL certificate (or NULL)
     */
    char *certificatePath;

    /**
     * @brief Data buffer to transfer.
     */
    char *data;

    /**
     * @brief Size of the data buffer
     */
    size_t size;

    /**
     * @brief Total bytes expected
     */
    size_t totalSize;

    /**
     * @brief Total bytes sent
     */
    size_t totalSent;
} MemoryBlock;

// Ethernet settings.
static struct in_addr localServerIpAddress;
static struct in_addr subnetMask;
static struct in_addr gatewayIpAddress;
static const char NetworkInterface[] = "eth0";

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}


/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Azure IoT Application starting.\n");

    bool isNetworkingReady = false;
    if ((Networking_IsNetworkingReady(&isNetworkingReady) == -1) || !isNetworkingReady) {
        Log_Debug("WARNING: Network is not ready. Device cannot connect until network is ready.\n");
    }

    exitCode = Options_ParseArgs(argc, argv);

    if (exitCode != ExitCode_Success) {
        return exitCode;
    }

    exitCode = InitPeripheralsAndHandlers();


    if (exitCode == ExitCode_Success) {
        if (!isNetworkStackReady) {
            exitCode = CheckNetworkStackStatusAndLaunchServers();
        }
    } else {
        Log_Debug("ERROR: failed to init peripherals and handlers\n");
    }
    
    // Main loop
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    ClosePeripheralsAndHandlers();

    Log_Debug("Application exiting.\n");

    return exitCode;
}

static void ExitCodeCallbackHandler(ExitCode ec)
{
    exitCode = ec;
}

/// <summary>
///     Check network status and display information about all available network interfaces.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode CheckNetworkStatus(void)
{
    // Ensure the necessary network interface is enabled.
    int result = Networking_SetInterfaceState(NetworkInterface, true);
    if (result != 0) {
        if (errno == EAGAIN) {
            Log_Debug("INFO: The networking stack isn't ready yet, will try again later.\n");
            return ExitCode_Success;
        } else {
            Log_Debug(
                "ERROR: Networking_SetInterfaceState for interface '%s' failed: errno=%d (%s)\n",
                NetworkInterface, errno, strerror(errno));
            return ExitCode_CheckStatus_SetInterfaceState;
        }
    }
    isNetworkStackReady = true;

    // Display total number of network interfaces.
    ssize_t count = Networking_GetInterfaceCount();
    if (count == -1) {
        Log_Debug("ERROR: Networking_GetInterfaceCount: errno=%d (%s)\n", errno, strerror(errno));
        return ExitCode_CheckStatus_GetInterfaceCount;
    }
    Log_Debug("INFO: Networking_GetInterfaceCount: count=%zd\n", count);

    // Read current status of all interfaces.
    size_t bytesRequired = ((size_t)count) * sizeof(Networking_NetworkInterface);
    Networking_NetworkInterface *interfaces = malloc(bytesRequired);
    if (!interfaces) {
        abort();
    }

    ssize_t actualCount = Networking_GetInterfaces(interfaces, (size_t)count);
    if (actualCount == -1) {
        Log_Debug("ERROR: Networking_GetInterfaces: errno=%d (%s)\n", errno, strerror(errno));
    }
    Log_Debug("INFO: Networking_GetInterfaces: actualCount=%zd\n", actualCount);

    // Print detailed description of each interface.
    for (ssize_t i = 0; i < actualCount; ++i) {
        Log_Debug("INFO: interface #%zd\n", i);

        // Print the interface's name.
        Log_Debug("INFO:   interfaceName=\"%s\"\n", interfaces[i].interfaceName);

        // Print whether the interface is enabled.
        Log_Debug("INFO:   isEnabled=\"%d\"\n", interfaces[i].isEnabled);

        // Print the interface's configuration type.
        Networking_IpType ipType = interfaces[i].ipConfigurationType;
        const char *typeText;
        switch (ipType) {
        case Networking_IpType_DhcpNone:
            typeText = "DhcpNone";
            break;
        case Networking_IpType_DhcpClient:
            typeText = "DhcpClient";
            break;
        default:
            typeText = "unknown-configuration-type";
            break;
        }
        Log_Debug("INFO:   ipConfigurationType=%d (%s)\n", ipType, typeText);

        // Print the interface's medium.
        Networking_InterfaceMedium_Type mediumType = interfaces[i].interfaceMediumType;
        const char *mediumText;
        switch (mediumType) {
        case Networking_InterfaceMedium_Unspecified:
            mediumText = "unspecified";
            break;
        case Networking_InterfaceMedium_Wifi:
            mediumText = "Wi-Fi";
            break;
        case Networking_InterfaceMedium_Ethernet:
            mediumText = "Ethernet";
            break;
        default:
            mediumText = "unknown-medium";
            break;
        }
        Log_Debug("INFO:   interfaceMediumType=%d (%s)\n", mediumType, mediumText);

        // Print the interface connection status
        Networking_InterfaceConnectionStatus status;
        int result = Networking_GetInterfaceConnectionStatus(interfaces[i].interfaceName, &status);
        if (result != 0) {
            Log_Debug("ERROR: Networking_GetInterfaceConnectionStatus: errno=%d (%s)\n", errno,
                      strerror(errno));
            return ExitCode_CheckStatus_GetInterfaceConnectionStatus;
        }
        Log_Debug("INFO:   interfaceStatus=0x%02x\n", status);
    }

    free(interfaces);

    return ExitCode_Success;
}

/// <summary>
///     Configure the specified network interface with a static IP address.
/// </summary>
/// <param name="interfaceName">
///     The name of the network interface to be configured.
/// </param>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode ConfigureNetworkInterfaceWithStaticIp(const char *interfaceName)
{
    Networking_IpConfig ipConfig;
    Networking_IpConfig_Init(&ipConfig);
    inet_aton("172.16.0.1", &localServerIpAddress);
    inet_aton("255.255.255.0", &subnetMask);
    inet_aton("0.0.0.0", &gatewayIpAddress);
    Networking_IpConfig_EnableStaticIp(&ipConfig, localServerIpAddress, subnetMask,
                                       gatewayIpAddress);

    int result = Networking_IpConfig_Apply(interfaceName, &ipConfig);
    Networking_IpConfig_Destroy(&ipConfig);
    if (result != 0) {
        Log_Debug("ERROR: Networking_IpConfig_Apply: %d (%s)\n", errno, strerror(errno));
        return ExitCode_ConfigureStaticIp_IpConfigApply;
    }
    Log_Debug("INFO: Set static IP address on network interface: %s.\n", interfaceName);

    return ExitCode_Success;
}

/// <summary>
///     Start SNTP server on the specified network interface.
/// </summary>
/// <param name="interfaceName">
///     The name of the network interface on which to start the SNTP server.
/// </param>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode StartSntpServer(const char *interfaceName)
{
    Networking_SntpServerConfig sntpServerConfig;
    Networking_SntpServerConfig_Init(&sntpServerConfig);
    int result = Networking_SntpServer_Start(interfaceName, &sntpServerConfig);
    Networking_SntpServerConfig_Destroy(&sntpServerConfig);
    if (result != 0) {
        Log_Debug("ERROR: Networking_SntpServer_Start: %d (%s)\n", errno, strerror(errno));
        return ExitCode_StartSntpServer_StartSntp;
    }
    Log_Debug("INFO: SNTP server has started on network interface: %s.\n", interfaceName);
    return ExitCode_Success;
}

/// <summary>
///     Configure and start DHCP server on the specified network interface.
/// </summary>
/// <param name="interfaceName">
///     The name of the network interface on which to start the DHCP server.
/// </param>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode ConfigureAndStartDhcpSever(const char *interfaceName)
{
    Networking_DhcpServerConfig dhcpServerConfig;
    Networking_DhcpServerConfig_Init(&dhcpServerConfig);

    struct in_addr dhcpStartIpAddress;
    inet_aton("172.16.0.2", &dhcpStartIpAddress);

    Networking_DhcpServerConfig_SetLease(&dhcpServerConfig, dhcpStartIpAddress, 1, subnetMask,
                                         gatewayIpAddress, 24);
    Networking_DhcpServerConfig_SetNtpServerAddresses(&dhcpServerConfig, &localServerIpAddress, 1);

    int result = Networking_DhcpServer_Start(interfaceName, &dhcpServerConfig);
    Networking_DhcpServerConfig_Destroy(&dhcpServerConfig);
    if (result != 0) {
        Log_Debug("ERROR: Networking_DhcpServer_Start: %d (%s)\n", errno, strerror(errno));
        return ExitCode_StartDhcpServer_StartDhcp;
    }

    Log_Debug("INFO: DHCP server has started on network interface: %s.\n", interfaceName);
    UserInterface_SetStatus(1, true);

    return ExitCode_Success;
}

/// <summary>
///     Configure network interface, start SNTP server and TCP server.
/// </summary>
/// <returns>
///     ExitCode_Success on success; otherwise another ExitCode value which indicates
///     the specific failure.
/// </returns>
static ExitCode CheckNetworkStackStatusAndLaunchServers(void)
{
    // Check the network stack readiness and display available interfaces when it's ready.
    ExitCode localExitCode = CheckNetworkStatus();
    if (localExitCode != ExitCode_Success) {
        return localExitCode;
    }

    // The network stack is ready, so unregister the timer event handler and launch servers.
    if (isNetworkStackReady) {
        // DisarmEventLoopTimer(checkStatusTimer);

        // Use static IP addressing to configure network interface.
        localExitCode = ConfigureNetworkInterfaceWithStaticIp(NetworkInterface);
        if (localExitCode == ExitCode_Success) {
            localExitCode = StartSntpServer(NetworkInterface);
        }

        if (localExitCode == ExitCode_Success) {
            localExitCode = ConfigureAndStartDhcpSever(NetworkInterface);
        }

        if (localExitCode != ExitCode_Success) {
            return localExitCode;
        }
    }

    return ExitCode_Success;
}


static const char *CloudResultToString(Cloud_Result result)
{
    switch (result) {
    case Cloud_Result_OK:
        return "OK";
    case Cloud_Result_NoNetwork:
        return "No network connection available";
    case Cloud_Result_OtherFailure:
        return "Other failure";
    }

    return "Unknown Cloud_Result";
}

/// <summary>
/// Callback function when the eth0 curl handle requests data
/// </summary>
/// <returns>Returns the number of bytes sent</returns>
static size_t LocalReadRequestedDataCallback(void *chunk, size_t chunkSize, size_t chunksCount, void *memoryBlock) {
    MemoryBlock *block = (MemoryBlock *)memoryBlock;
    size_t maxSize = chunkSize * chunksCount;
    
    if (block->totalSent == block->totalSize) {
        return 0;
    }

    if (block->data == NULL) {
        return CURL_READFUNC_PAUSE;
    }
    
    if (block->size > maxSize) {
        memcpy(chunk, block->data, maxSize);
        block->data += maxSize;
        block->size -= maxSize;
        
        return maxSize;
    } else {
        size_t size = block->size;
        memcpy(chunk, block->data, block->size);
        free(block->data);        

        block->totalSent += block->size;
        block->data = NULL;
        block->size = 0;

        curl_easy_pause(block->cloudHandle, CURLPAUSE_CONT);

        return size;
    }
}


/// <summary>
/// Callback function when the cloud curl handle sends us data
/// </summary>
/// <returns>Returns the number of bytes read</returns>
static size_t CloudWriteDownloadedDataCallback(void *chunk, size_t chunkSize, size_t chunksCount, void *memoryBlock)
{
    MemoryBlock *block = (MemoryBlock *)memoryBlock;
    size_t realsize = chunkSize * chunksCount;

    if (block->data != NULL) {
        return CURL_WRITEFUNC_PAUSE;
    }
    
    char *ptr = realloc(block->data, block->size + realsize + 1);

    if(ptr == NULL)
        return 0;  /* out of memory! */

    block->data = ptr;
    memcpy(&(block->data[block->size]), chunk, realsize);
    block->size += realsize;
    block->data[block->size] = 0;
    
    curl_easy_pause(block->localHandle, CURLPAUSE_CONT);
    
    return realsize;
}

/// <summary>
/// Initializes a cloud curl handle
/// </summary>
/// <returns>Returns a CURL pointer</returns>
static CURL * InitCloudCurlHandle(MemoryBlock *block, DeviceUpdateRequest request) {
    CURLcode res = 0;
    char *certificatePath = NULL;
    CURL *handle = curl_easy_init();
    Connection_IotHub_Config *connectionContext = (Connection_IotHub_Config *)Options_GetConnectionContext();

    if ((res = curl_easy_setopt(handle, CURLOPT_SSL_SESSIONID_CACHE, 0)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_SSL_SESSIONID_CACHE\n");
        return NULL;
    }
    if ((res = curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)block)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_WRITEDATA\n");
        return NULL;
    }
    
    if ((res = curl_easy_setopt(handle, CURLOPT_URL, request.url)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_URL\n");
        return NULL;
    }

    if (connectionContext->certPath != NULL && connectionContext->certPath[0] != '\0') {
        certificatePath = Storage_GetAbsolutePathInImagePackage(connectionContext->certPath);

        if (certificatePath == NULL) {
            Log_Debug("The certificate path could not be resolved: errno=%d (%s)\n", errno,
                    strerror(errno));
            return NULL;
        }
        
        block->certificatePath = certificatePath;

        if ((res = curl_easy_setopt(handle, CURLOPT_CAINFO, certificatePath)) != CURLE_OK) {
            Log_Debug("curl_easy_setopt CURLOPT_CAINFO\n");
            return NULL;
        }
    }

    // Let cURL follow any HTTP 3xx redirects.
    // Important: Any redirection to different domain names requires that domain name to be added to
    // app_manifest.json.
    if ((res = curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_FOLLOWLOCATION\n");
        return NULL;
    }

    // Set up callback for cURL to use when downloading data.
    if ((res = curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, CloudWriteDownloadedDataCallback)) !=
        CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_WRITEFUNCTION\n", res);
        return NULL;
    }

    // Set the custom parameter of the callback to the memory block.
    if ((res = curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)block)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_WRITEDATA\n", res);
        return NULL;
    }

    // Specify a user agent.
    if ((res = curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl-agent/1.0")) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_USERAGENT\n");
        return NULL;
    }
    
    if ((res = curl_easy_setopt(handle, CURLOPT_TIMEOUT, CURL_TIMEOUT_S)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_TIMEOUT\n");
        return NULL;
    }
    
    if ((res = curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_FAILONERROR\n");
        return NULL;
    }

    return handle;
}

/// <summary>
/// Initializes a local curl handle to the first DHCP address (on eth0)
/// </summary>
/// <returns>Returns a CURL pointer</returns>
static CURL * InitLocalCurlHandle(MemoryBlock *block, DeviceUpdateRequest request) {
    CURLcode res = 0;
    CURL *handle = curl_easy_init();
    struct curl_httppost* post = NULL;
    struct curl_httppost* last = NULL;
    struct curl_slist *header = NULL;
    Connection_IotHub_Config *connectionContext = (Connection_IotHub_Config *)Options_GetConnectionContext();
    
    Log_Debug("INFO: file '%s', size=%d\n", request.filename, (long)request.size);

    header = curl_slist_append(header, "Content-Type: multipart/form-data");
    // header = curl_slist_append(header, "Transfer-Encoding: chunked");

    // curl_formadd(&post, &last, CURLFORM_COPYNAME, "uuid",
    //     CURLFORM_COPYCONTENTS, uuid,
    //     CURLFORM_END);

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "file",
        CURLFORM_CONTENTTYPE, "application/octet-stream",
        CURLFORM_STREAM, (void *)block, 
        CURLFORM_CONTENTSLENGTH, request.size,
        CURLFORM_FILENAME, request.filename,
        CURLFORM_END);

    if ((res = curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl-agent/1.0")) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_USERAGENT\n");
        return NULL;
    }

    if ((res = curl_easy_setopt(handle, CURLOPT_HTTPHEADER, header)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_HTTPHEADER\n");
        return NULL;
    }

    if ((res = curl_easy_setopt(handle, CURLOPT_HTTP_TRANSFER_DECODING, 0)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_HTTP_TRANSFER_DECODING\n");
        return NULL;
    }

    if ((res = curl_easy_setopt(handle, CURLOPT_READFUNCTION, LocalReadRequestedDataCallback)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_READFUNCTION\n");
        return NULL;
    }

    if ((res = curl_easy_setopt(handle, CURLOPT_URL, connectionContext->endpoint)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_URL\n");
        return NULL;
    }
    
    if ((res = curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_FOLLOWLOCATION\n");
        return NULL;
    }

    if ((res = curl_easy_setopt(handle, CURLOPT_HTTPPOST, post)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_HTTPPOST\n");
        return NULL;
    }

    if ((res = curl_easy_setopt(handle, CURLOPT_TIMEOUT, CURL_TIMEOUT_S)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_TIMEOUT\n");
        return NULL;
    }

    if ((res = curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1)) != CURLE_OK) {
        Log_Debug("curl_easy_setopt CURLOPT_FAILONERROR\n");
        return NULL;
    }

    block->formData = post;
    
    return handle;
}


/// <summary>
/// Callback function executed when the desired recipe is changed
/// </summary>
/// <returns>Returns a CURL pointer</returns>
static void CloudDesiredRecipeChangedCallback(DeviceUpdateRequest request) {
    CURLcode res = 0;
    int still_running = 1;
    MemoryBlock block = { .size = 0, .data = NULL, .totalSize = request.size, .totalSent = 0 };
    CURLM *multi_handle;
    struct CURLMsg *msg;
    int msgs_left; /* how many messages are left */
    bool update_failed = 0;

    if (request.size <= 0) {
        Log_Debug("INFO: canceled current campaign\n");

        UserInterface_SetStatus(3, false);
        UserInterface_SetStatus(2, false);

        memset(&failed_update, 0, sizeof(DeviceUpdateRequest));
        return;
    }

    UserInterface_SetStatus(3, false);
    UserInterface_SetStatus(2, true);

    if ((res = curl_global_init(CURL_GLOBAL_ALL)) != CURLE_OK) {
        Log_Debug("ERROR: curl_global_init failed\n");
        goto exitLabel;
    }
    
    multi_handle = curl_multi_init();

    block.cloudHandle = InitCloudCurlHandle(&block, request);
    block.localHandle = InitLocalCurlHandle(&block, request);
    
    if (block.cloudHandle == NULL || block.localHandle == NULL) {
        Log_Debug("ERROR: failed to create CURL handles\n");
        goto cleanupLabel;
    }
    
    curl_multi_add_handle(multi_handle, block.localHandle);
    curl_multi_add_handle(multi_handle, block.cloudHandle); 

    while(still_running) {
        CURLMcode mc = curl_multi_perform(multi_handle, &still_running);
    
        if (mc == CURLM_OK) {
            /* wait for activity, timeout or "nothing" */
            mc = curl_multi_wait(multi_handle, NULL, 0, 1000, NULL);
        }
        else {
            Log_Debug("ERROR: curlm failed\n");
        }
    }
    
    // look for errors in each handle
    while((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
        if(msg->msg == CURLMSG_DONE) {
            res = msg->data.result;
            if (res != CURLE_OK) {
                update_failed = true;

                UserInterface_SetStatus(3, true);
                Log_Debug("ERROR: curl failed with code=%d\n", res);
            }
        }
    }

cleanupLabel:
    free(block.certificatePath);
    curl_multi_cleanup(multi_handle);
    curl_global_cleanup();
    curl_formfree(block.formData);

exitLabel:
    Log_Debug("INFO: curl: exit\n");
    UserInterface_SetStatus(2, false);

    if (update_failed) {
        if (strcmp(failed_update.uuid, request.uuid) == 0) {
            failed_update.retries++;
        } else {
            strcpy(failed_update.uuid, request.uuid);
            strcpy(failed_update.filename, request.filename);
            strcpy(failed_update.url, request.url);

            failed_update.size = request.size;
        }
    } else {
        memset(&failed_update, 0, sizeof(DeviceUpdateRequest));
    }
    
    Cloud_Result result = Cloud_SendRecipeCampaignChangedEvent(request, update_failed);

    if (result != Cloud_Result_OK) {
        Log_Debug(
            "WARNING: Could not send recipe campaign changed event to cloud: "
            "%s",
            CloudResultToString(result));
    }
    
}

static void DisplayAlertCallbackHandler(const char *alertMessage)
{
    Log_Debug("ALERT: %s\n", alertMessage);
}

static void ConnectionChangedCallbackHandler(bool connected)
{
    isConnected = connected;

    if (isConnected) {
        Cloud_Result result = Cloud_SendDeviceDetails(serialNumber);
        if (result != Cloud_Result_OK) {
            Log_Debug("WARNING: Could not send device details to cloud: %s\n",
                      CloudResultToString(result));
        }
    }
}

static void TimerCallbackHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_TelemetryTimer_Consume;
        return;
    }
    // Check whether the network stack is ready.
    if (!isNetworkStackReady) {
        ExitCode localExitCode = CheckNetworkStackStatusAndLaunchServers();
        if (localExitCode != ExitCode_Success) {
            exitCode = localExitCode;
            return;
        }
    }

    if (failed_update.size) {
        Log_Debug("INFO: retrying a failed update, uuid=%s\n", failed_update.uuid);
        DeviceUpdateRequest req;
        memcpy(&req, &failed_update, sizeof(DeviceUpdateRequest));

        CloudDesiredRecipeChangedCallback(req);
    }
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;

    memset(&failed_update, 0, sizeof(DeviceUpdateRequest));

    sigaction(SIGTERM, &action, NULL);

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    struct timespec telemetryPeriod = {.tv_sec = (2*CURL_TIMEOUT_S), .tv_nsec = 0};
    telemetryTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &TimerCallbackHandler, &telemetryPeriod);
    if (telemetryTimer == NULL) {
        return ExitCode_Init_TelemetryTimer;
    }

    ExitCode interfaceExitCode = UserInterface_Initialize();

    if (interfaceExitCode != ExitCode_Success) {
        return interfaceExitCode;
    }

    void *connectionContext = Options_GetConnectionContext();

    return Cloud_Initialize(eventLoop, connectionContext, ExitCodeCallbackHandler,
                            CloudDesiredRecipeChangedCallback,
                            DisplayAlertCallbackHandler, ConnectionChangedCallbackHandler);
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    DisposeEventLoopTimer(telemetryTimer);
    Cloud_Cleanup();
    UserInterface_Cleanup();
    Connection_Cleanup();
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors\n");
}
