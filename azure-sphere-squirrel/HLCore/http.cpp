/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "http.h"
#include "squirrel_cpp_helper.h"
#include <stdlib.h>
#include <string.h>
#include "squirrel/include/sqstdblob.h"
#include "http_request.h"
#include "curl_logs.h"
#include <applibs/application.h>
#include <applibs/storage.h>
#include <tlsutils/deviceauth.h>
#include <tlsutils/deviceauth_curl.h> // required only for mutual authentication
#include <applibs/networking_curl.h>  // required only if using proxy to connect to the internet
extern "C"
{
    #include <wolfssl/ssl.h>
}

// Static Methods
//---------------
/// Creates and registers an instance of the HTTP Class in the Squirrel roottable as a globally available object
/// and exposes methods so they may be called from within the VM.
/// \param vm the instance of the VM to use.
/// \returns the registered instance.
HTTP* HTTP::registerWithSquirrelAsGlobal(HSQUIRRELVM vm, EventLoop *eventLoop, const char* name)
{
    SquirrelCppHelper::DelegateFunction delegateFunctions[5];
    delegateFunctions[0] = SquirrelCppHelper::DelegateFunction("get", &HTTP::SQUIRREL_METHOD_NAME(get));
    delegateFunctions[1] = SquirrelCppHelper::DelegateFunction("put", &HTTP::SQUIRREL_METHOD_NAME(put));
    delegateFunctions[2] = SquirrelCppHelper::DelegateFunction("post", &HTTP::SQUIRREL_METHOD_NAME(post));
    delegateFunctions[3] = SquirrelCppHelper::DelegateFunction("request", &HTTP::SQUIRREL_METHOD_NAME(request));
    delegateFunctions[4] = SquirrelCppHelper::DelegateFunction("getDeviceIdFromDAA", &HTTP::SQUIRREL_METHOD_NAME(getDeviceIdFromDAA));

    HTTP* http = SquirrelCppHelper::registerClassAsGlobal<HTTP>(vm, name, delegateFunctions, 5);
    http->initialise(vm, eventLoop);
    return http;
}

/// Provides a CURLMOPT_TIMERFUNCTION compatible callback, forwarding timer callbacks to the apropriate HTTP instance.
int HTTP::curlTimerCallback(CURLM *multiHandle, long timeout_ms, void *httpUserData)
{
    return ((HTTP*)httpUserData)->_curlTimerCallback(multiHandle, timeout_ms);
}

/// Provides an EventLoopTimer compatible callback, forwarding timer events to the apropriate HTTP instance.
void HTTP::curlTimerEventHandler(EventLoopTimer *timer)
{
    ((HTTP*)timer->context)->_curlTimerEventHandler(timer);
}

/// Provides a CURLMOPT_SOCKETFUNCTION compatible callback, forwarding socket events to the apropriate HTTP instance.
int HTTP::curlMSocketCallback(CURL *easyHandle, curl_socket_t socketFd, int action, void *httpUserData, void *socketUserData)
{
    return ((HTTP*)httpUserData)->_curlMSocketCallback(easyHandle, socketFd, action, socketUserData);
}

/// Provides an EventLoop I/O compatible callback, forwarding file I/O events to the apropriate HTTP instance.
void HTTP::curlFdEventHandler(EventLoop *eventLoop, int socketFd, EventLoop_IoEvents events, void *httpUserData)
{
    ((HTTP*)httpUserData)->_curlFdEventHandler(eventLoop, socketFd, events);
}

/// Provides a CURLOPT_READFUNCTION compatible callback, forwarding read requests to the apropriate HTTPRequest.
size_t HTTP::curlReadCallback(void *buffer, size_t size, size_t nitems, void *userdata)
{
    return ((HTTPRequest*)userdata)->curlReadCallback(buffer, size * nitems);
}

/// Provides a CURLOPT_WRITEFUNCTION compatible callback, forwarding read requests to the apropriate HTTPRequest.
size_t HTTP::curlWriteCallback(void *data, size_t size, size_t nmemb, void *userp)
{
    return ((HTTPRequest*)userp)->curlWriteCallback(data, nmemb);
}

/// Provides a CURLOPT_HEADERFUNCTION compatible callback, forwarding read requests to the apropriate HTTPRequest.
size_t HTTP::curlWriteHeaderCallback(void *buffer, size_t size, size_t nitems, void *userdata)
{
    return ((HTTPRequest*)userdata)->curlWriteHeaderCallback(buffer, nitems);
}

/*
/// Helpful function for debug only.
int HTTP::curlDebugCallback(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr)
{
    const char *text;
  (void)handle; // prevent compiler warning
  (void)userptr;
 
  switch (type) {
  case CURLINFO_TEXT:
    Log_Debug("== Info: %s", data);
  default: // in case a new one is introduced to shock us
    return 0;
 
  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }

  Log_Debug(text);
}*/

// Squirrel Methods
//-----------------
/// Constructs a GET HTTPRequest and places it on the stack.
/// \param vm the instance of the VM to use.
/// \returns SQ_SUCCEEDED on success, otherwise SQ_FAILED.
/// \throws throws within the Squirrel VM if it's not possible to retrieve user pointers to the instance or method.
SQUIRREL_METHOD_IMPL(HTTP, get)
{
    int types[] = {OT_STRING, OT_TABLE};
    if(SQ_FAILED(SquirrelCppHelper::checkParameterTypes(vm, 2, 2, types)))
    {
        return SQ_ERROR;
    }

    // Fetch the method parameters
    const SQChar *url;
    sq_getstring(vm, 2, &url);

    HSQOBJECT headers;
    sq_getstackobj(vm, 3, &headers);

    // Convert the headers table to a curl string linked list by iterating over the table
    curl_slist *headerList;
    SQInteger result = generateHeaderList(vm, headers, headerList);
    if(SQ_FAILED(result))
    {
        return result;
    }

    // Create, push to stack and setup the new custom HTTPRequest
    return HTTPRequest::newHTTPRequest(vm, curlMulti, curlTemplate, "GET", url, headerList);
}

/// Constructs a PUT HTTPRequest and places it on the stack.
/// \param vm the instance of the VM to use.
/// \returns SQ_SUCCEEDED on success, otherwise SQ_FAILED.
/// \throws throws within the Squirrel VM if it's not possible to retrieve user pointers to the instance or method.
SQUIRREL_METHOD_IMPL(HTTP, put)
{
    int types[] = {OT_STRING, OT_TABLE, OT_STRING | OT_INSTANCE};
    if(SQ_FAILED(SquirrelCppHelper::checkParameterTypes(vm, 3, 3, types)))
    {
        return SQ_ERROR;
    }

    // Fetch the method parameters
    const SQChar *url;
    sq_getstring(vm, 2, &url);

    HSQOBJECT headers;
    sq_getstackobj(vm, 3, &headers);

    SQChar *body;
    SQInteger bodySize;
    SQInteger result = retrieveAndCopyBody(vm, 4, body, bodySize);
    if(SQ_FAILED(result))
    {
        return result;
    }

    // Convert the headers table to a curl string linked list by iterating over the table
    curl_slist *headerList;
    result = generateHeaderList(vm, headers, headerList);
    if(SQ_FAILED(result))
    {
        sq_free(body, bodySize);
        return result;
    }

    // Create, push to stack and setup the new custom HTTPRequest
    return HTTPRequest::newHTTPRequest(vm, curlMulti, curlTemplate, "PUT", url, headerList, body, bodySize);
}

/// Constructs a PUT HTTPRequest and places it on the stack.
/// \param vm the instance of the VM to use.
/// \returns SQ_SUCCEEDED on success, otherwise SQ_FAILED.
/// \throws throws within the Squirrel VM if it's not possible to retrieve user pointers to the instance or method.
SQUIRREL_METHOD_IMPL(HTTP, post)
{
    int types[] = {OT_STRING, OT_TABLE, OT_STRING | OT_INSTANCE};
    if(SQ_FAILED(SquirrelCppHelper::checkParameterTypes(vm, 3, 3, types)))
    {
        return SQ_ERROR;
    }

    // Fetch the method parameters
    const SQChar *url;
    sq_getstring(vm, 2, &url);

    HSQOBJECT headers;
    sq_getstackobj(vm, 3, &headers);

    SQChar *body;
    SQInteger bodySize;
    SQInteger result = retrieveAndCopyBody(vm, 4, body, bodySize);
    if(SQ_FAILED(result))
    {
        return result;
    }

    // Convert the headers table to a curl string linked list by iterating over the table
    curl_slist *headerList;
    result = generateHeaderList(vm, headers, headerList);
    if(SQ_FAILED(result))
    {
        sq_free(body, bodySize);
        return result;
    }

    // Create, push to stack and setup the new custom HTTPRequest
    return HTTPRequest::newHTTPRequest(vm, curlMulti, curlTemplate, "POST", url, headerList, body, bodySize);
}

/// Constructs a generic HTTPRequest and places it on the stack.
/// \param vm the instance of the VM to use.
/// \returns SQ_SUCCEEDED on success, otherwise SQ_FAILED.
/// \throws throws within the Squirrel VM if it's not possible to retrieve user pointers to the instance or method.
SQUIRREL_METHOD_IMPL(HTTP, request)
{
    int types[] = {OT_STRING, OT_STRING, OT_TABLE, OT_STRING | OT_INSTANCE};
    if(SQ_FAILED(SquirrelCppHelper::checkParameterTypes(vm, 4, 4, types)))
    {
        return SQ_ERROR;
    }

    // Fetch the method parameters
    const SQChar *verb;
    sq_getstring(vm, 2, &verb);

    const SQChar *url;
    sq_getstring(vm, 3, &url);

    HSQOBJECT headers;
    sq_getstackobj(vm, 4, &headers);

    SQChar *body;
    SQInteger bodySize;
    SQInteger result = retrieveAndCopyBody(vm, 5, body, bodySize);
    if(SQ_FAILED(result))
    {
        return result;
    }

    // Convert the headers table to a curl string linked list by iterating over the table
    curl_slist *headerList;
    result = generateHeaderList(vm, headers, headerList);
    if(SQ_FAILED(result))
    {
        sq_free(body, bodySize);
        return result;
    }

    // Create, push to stack and setup the new custom HTTPRequest
    return HTTPRequest::newHTTPRequest(vm, curlMulti, curlTemplate, verb, url, headerList, body, bodySize);
}

/// Retrieves the device ID from the DAA certificate.
/// \param vm the instance of the VM to use.
/// \returns SQ_SUCCEEDED on success, otherwise SQ_FAILED.
/// \throws throws within the Squirrel VM if the DAA certificate has not yet been obtained or WolfSSL fails.
SQUIRREL_METHOD_IMPL(HTTP, getDeviceIdFromDAA)
{
    int types[] = {OT_NULL};
    if(SQ_FAILED(SquirrelCppHelper::checkParameterTypes(vm, 0, 0, types)))
    {
        return SQ_ERROR;
    }
    
    // Ensure that we have received the DAA certificate
    bool daaRetrieved = false;
    if(Application_IsDeviceAuthReady(&daaRetrieved) < 0 || !daaRetrieved)
    {
        return sq_throwerror(vm, "The DAA Certification needed to retrieve Device ID has not yet been obtained");
    }

    // Initialise WolfSSL so that we can retrieve the DAA certificate
    if(wolfSSL_Init() != WOLFSSL_SUCCESS)
    {
        return sq_throwerror(vm, "Unable to initialise WolfSSL");
    }

    // Retrieve the DAA certificate
    WOLFSSL_X509 *daaCertificate = wolfSSL_X509_load_certificate_file(DeviceAuth_GetCertificatePath(), WOLFSSL_FILETYPE_PEM);

    if(daaCertificate == NULL)
    {
        wolfSSL_Cleanup();
        return sq_throwerror(vm, "WolfSSL was unable to load the DAA certifcate file");
    }

    // Extract the DAA certifcate subject name (this contains the device ID)
    WOLFSSL_X509_NAME* subjectName = wolfSSL_X509_get_subject_name(daaCertificate);
    if(subjectName == NULL)
    {
        wolfSSL_X509_free(daaCertificate);
        wolfSSL_Cleanup();
        return sq_throwerror(vm, "WolfSSL was unable to extract the subject name from the DAA certifcate");
    }

    // Extract the device ID from the subject and place on the stack as a return value
    char deviceId[134] = { 0 };
    if(wolfSSL_X509_NAME_oneline(subjectName, (char*)&deviceId, sizeof(deviceId)) < 0)
    {
        wolfSSL_X509_free(daaCertificate);
        wolfSSL_Cleanup();
        return sq_throwerror(vm, "WolfSSL was unable to extract the device id from the subject in the DAA certifcate");
    }

    // Cleanup WolfSSL
    wolfSSL_X509_free(daaCertificate);
    wolfSSL_Cleanup();

    sq_pushstring(vm, deviceId+4, sizeof(deviceId)-6);

    // Return the Squirrel object from the top of the stack
    return 1;
}

// Methods
//--------
/// Retrieves a HTTP request body (string|blob) from the specified stack location and provides a copy of the underlying data.
/// \note Ownership of outputBody is passed to the function caller and must be freed once no longer needed.
/// \param vm the instance of the VM to use.
/// \param stackLocation the location on the stack where the body string|blob may be found.
/// \param outputBody will be populated with a heap allocated copy of the underlying data from the body string|blob (must be freed once no longer needed).
/// \param outputBodySize will be populated with the size of the data pointed to by outputBody.
/// \returns SQ_SUCCEEDED on success, otherwise SQ_FAILED.
/// \throws throws within the Squirrel VM and so return value should be passed out of the calling function back to the VM.
SQInteger HTTP::retrieveAndCopyBody(HSQUIRRELVM vm, SQInteger stackLocation, SQChar *&outputBody, SQInteger &outputBodySize)
{
    const SQChar *body;
    if(sq_gettype(vm, stackLocation) == OT_STRING)
    {
        sq_getstringandsize(vm, stackLocation, (const SQChar**)&body, &outputBodySize);
        outputBody = (SQChar*)sq_malloc(outputBodySize);
        memcpy((void*)outputBody, (void*)body, outputBodySize);
    }
    else
    {
        if(SQ_FAILED(sqstd_getblob(vm, stackLocation, (SQUserPointer*)&outputBody)))
        {
            return SQUIRREL_THROW_BAD_PARAMETER_TYPE(vm);
        }
        outputBodySize = sqstd_getblobsize(vm, stackLocation);
        outputBody = (SQChar*)sq_malloc(outputBodySize);
        memcpy((void*)outputBody, (void*)body, outputBodySize);
    }

    return 0;
}

/// Generates a curl compliant HTTP header string linked list from the specified header table object.
/// \note Ownership of outputHeaderList is passed to the function caller and must be freed once no longer needed.
/// \param vm the instance of the VM to use.
/// \param headers the table object to extract HTTP headers from.
/// \param outputHeaderList will be populated with a heap allocated copy of the headers converted into a curl_slist linked list (must be freed once no longer needed).
/// \returns SQ_SUCCEEDED on success, otherwise SQ_FAILED.
/// \throws throws within the Squirrel VM and so return value should be passed out of the calling function back to the VM.
SQInteger HTTP::generateHeaderList(HSQUIRRELVM vm, HSQOBJECT headers, curl_slist *&outputHeaderList)
{
    // Convert the headers table to a curl string linked list by iterating over the table
    outputHeaderList = nullptr;
    sq_pushobject(vm, headers);
    sq_pushnull(vm);
    SQChar headerString[1024];
    while(SQ_SUCCEEDED(sq_next(vm, -2)))
    {
        const SQChar *key;
        SQInteger keyLen;
        sq_getstringandsize(vm, -2, &key, &keyLen);
        const SQChar *val;
        SQInteger valLen;
        sq_getstringandsize(vm, -1, &val, &valLen);

        if( keyLen + valLen + 2 > 1024 )
        {
            curl_slist_free_all(outputHeaderList);
            return sq_throwerror(vm, "At least one header exceeds the maximum allowable size of 1KB.");
        }

        strncpy( headerString, key, keyLen );
        strcat( headerString, ": " );
        strncat( headerString, val, valLen );
        
        outputHeaderList = curl_slist_append(outputHeaderList, headerString);

        sq_pop(vm, 2);
    }

    return 0; 
}

/// Triggers CURL to process the next stage of transfers on the associated CurlMulti handle.
void HTTP::curlProcessTransfers()
{
    CURLMcode code = curl_multi_socket_action(curlMulti, CURL_SOCKET_TIMEOUT, 0, &activeEasyHandles);
    if(code != CURLM_OK)
    {
        LogCurlMultiError("curl_multi_socket_action", code);
    }
}

/// Handles Curl timer callbacks, resecheduling the timeout timer with the latest timeout provided.
/// \param multiHandle a pointer to a Curl Multi handle.
/// \param timeout_ms the timeout (in milliseconds) to configure the timer for.
/// \returns '0' on Success, otherwise '<0'. 
int HTTP::_curlTimerCallback(CURLM *multiHandle, long timeout_ms)
{
    // A value of -1 means the timer does not need to be started.
    if(timeout_ms != -1)
    {
        // Invoke cURL immediately if requested to do so.
        if(timeout_ms == 0)
        {
            curlProcessTransfers();
        }
        else
        {
            // Start a single shot timer with the period as provided by cURL.
            // The timer handler will invoke cURL to process the web transfers.
            const struct timespec timeout = {.tv_sec = timeout_ms / 1000,
                                             .tv_nsec = (timeout_ms % 1000) * 1000000};
            SetEventLoopTimerOneShot(curlTimeoutTimer, &timeout);
        }
    }

    return 0;
}

/// Handles the timeout timer triggered event by consuming the event and prompting
/// curl to process the next stage of transfers.
/// \param timer a pointer to the timer object that triggered the event.
void HTTP::_curlTimerEventHandler(EventLoopTimer *timer)
{
    if(ConsumeEventLoopTimerEvent(timer) != 0)
    {
        Log_Debug("ERROR: cannot consume the timer event.\n");
        return;
    }

    curlProcessTransfers();
}

/// Handles Curl socket lifetime events by adding, modifying and removing them for event monitoring.
/// \param easyHandle a pointer to the Curl Easy handle of the socket.
/// \param socketFd the file descriptor of the socket (with which to listen or not for events).
/// \param action the action to take (add/modify/remove from event monitoring).
/// \param socketUserData a reference to the I/O event currently being monitored (otherwise NULL).
/// \returns '0' on Success, otherwise '<0'. 
int HTTP::_curlMSocketCallback(CURL *easyHandle, curl_socket_t socketFd, int action, void *socketUserData)
{
    EventRegistration *socketEvent = (EventRegistration *)socketUserData;

    // Determine if we should unregister sockets for monitoring by the event loop
    if(action == CURL_POLL_REMOVE)
    {
        int result = EventLoop_UnregisterIo(eventLoop, socketEvent);

        // Note: Allow EBADF errors as sometimes the kernel has done this cleanup for us already
        if (result == -1 && errno != EBADF)
        {
            Log_Debug("ERROR: Cannot unregister IO event: %d\n", errno);
        }

        return CURLM_OK;
    }

    // If we're not currently monitoring the socketFd, do so
    if(socketEvent == NULL)
    {
        socketEvent = EventLoop_RegisterIo(eventLoop, socketFd, 0x0, HTTP::curlFdEventHandler, this);

        if(socketEvent == NULL)
        {
            LogErrno("ERROR: Could not create socket event", errno);
            return -1;
        }

        curl_multi_assign(curlMulti, socketFd, socketEvent);
    }

    // Determine if we should be monitoring for in, out or both events and update the events mask
    EventLoop_IoEvents eventsMask = 0;

    if(action == CURL_POLL_IN || action == CURL_POLL_INOUT)
    {
        eventsMask |= EventLoop_Input;
    }
    if(action == CURL_POLL_OUT || action == CURL_POLL_INOUT)
    {
        eventsMask |= EventLoop_Output;
    }

    int result = EventLoop_ModifyIoEvents(eventLoop, socketEvent, eventsMask);
    if(result == -1)
    {
        LogErrno("ERROR: Could not add or modify socket event mask", socketFd);
        return -1;
    }

    return CURLM_OK;
}

/// Handles file I/O events, requesting Curl to read/write data etc.
/// \param eventLoop a pointer to the event loop that picked-up the event (UNUSED).
/// \param socketFd the file descriptor of the socket on which the event occured.
/// \param events a bitmask of which events occured on the socket (UNUSED).
void HTTP::_curlFdEventHandler(EventLoop *eventLoop, int socketFd, EventLoop_IoEvents events)
{
    CURLMcode code;
    int newActiveEasyHandles = 0;
    if((code = curl_multi_socket_action(curlMulti, socketFd, 0, &newActiveEasyHandles)) != CURLM_OK)
    {
        LogCurlMultiError("curl_multi_socket_action", code);
        return;
    }

    // Each time the 'running_handles' counter changes, curl_multi_info_read() will return info
    // about the specific transfers that completed.
    if(newActiveEasyHandles != activeEasyHandles)
    {
        int numberOfMessagesInQueue;
        CURLMsg *curlMessage;
        
        while((curlMessage = curl_multi_info_read(curlMulti, &numberOfMessagesInQueue)) != NULL)
        {
            if(curlMessage->msg == CURLMSG_DONE)
            {
                HTTPRequest *httpRequest;
                curl_easy_getinfo(curlMessage->easy_handle, CURLINFO_PRIVATE, (void**)&httpRequest);
                httpRequest->processResult(curlMessage->data.result);
            }
        }
    }
    activeEasyHandles = newActiveEasyHandles;
}

/// Initialises the instance, this should be a constructor but without try/throw that wasn't possible.
/// @param vm the VM instance upon which to act (UNUSED).
/// @param eventLoop a pointer to the EventLoop to register async timers/events with.
/// \return '0' on Success, otherwise '<0'.
int HTTP::initialise(HSQUIRRELVM vm, EventLoop *eventLoop)
{
    this->eventLoop = eventLoop;

    // Initialise CURL
    curl_global_init(CURL_GLOBAL_ALL);
    
    // Create a CURL easy handle to hold system-wide configuration, for use as a template for other handles
    curlTemplate = curl_easy_init();

    CURLcode result;
    // Enable redirect following
    result = curl_easy_setopt(curlTemplate, CURLOPT_FOLLOWLOCATION, 1L);
    // Restrict requests to HTTP(S)
    long allowedProtocols = CURLPROTO_HTTP | CURLPROTO_HTTPS;
    result = curl_easy_setopt(curlTemplate, CURLOPT_PROTOCOLS, allowedProtocols);
    // Restrict redirects to HTTP(S)
    result = curl_easy_setopt(curlTemplate, CURLOPT_REDIR_PROTOCOLS, allowedProtocols);
    // Load the HTTPS certificate(s)
    char *certificatePath = Storage_GetAbsolutePathInImagePackage("certs/CA.cer");
    result = curl_easy_setopt(curlTemplate, CURLOPT_CAINFO, certificatePath);
    // Enable mTLS
    result = curl_easy_setopt(curlTemplate, CURLOPT_SSL_CTX_FUNCTION, DeviceAuth_CurlSslFunc);
    // Turn of verbose messaging (for performance/space)
    result = curl_easy_setopt(curlTemplate, CURLOPT_VERBOSE, 0);
    // Use proxy
    //Networking_Curl_SetDefaultProxy(curlTemplate);
    // Send headers to the server only and not the proxy (needed for correct HTTPS proxy support)
    result = curl_easy_setopt(curlTemplate, CURLOPT_HEADEROPT, CURLHEADER_SEPARATE);
    // Attach function to handle downloaded data
    result = curl_easy_setopt(curlTemplate, CURLOPT_WRITEFUNCTION, HTTP::curlWriteCallback);
    // Attach function to handle downloaded headers
    result = curl_easy_setopt(curlTemplate, CURLOPT_HEADERFUNCTION, HTTP::curlWriteHeaderCallback);
    // Attach function to handle uploaded data
    result = curl_easy_setopt(curlTemplate, CURLOPT_READFUNCTION, HTTP::curlReadCallback);

    // Create a CURL Multi handle for high-performance aysnc requests
    curlMulti = curl_multi_init();

    CURLMcode multiResult;
    multiResult = curl_multi_setopt(curlMulti, CURLMOPT_SOCKETFUNCTION, HTTP::curlMSocketCallback);
    multiResult = curl_multi_setopt(curlMulti, CURLMOPT_SOCKETDATA, this);
    multiResult = curl_multi_setopt(curlMulti, CURLMOPT_TIMERFUNCTION, HTTP::curlTimerCallback);
    multiResult = curl_multi_setopt(curlMulti, CURLMOPT_TIMERDATA, this);
    activeEasyHandles = 0;

    // Create an EventLoop timer to be set by CURL Multi for handling timeouts
    curlTimeoutTimer = CreateEventLoopDisarmedTimer(eventLoop, HTTP::curlTimerEventHandler, this);
    if(curlTimeoutTimer == NULL)
    {
        return -1;
    }

    return 0;
}

HTTP::~HTTP()
{
    // Individual easy handles should have been cleaned-up by the destruction of HTTPRequest objects/VM.

    // Cleanup the template CurlEasy handle
    curl_easy_cleanup(curlTemplate);
    
    // Cleanup the CurlMulti handle
    curl_multi_cleanup(curlMulti);
    
    // Cleanup Curl globally
    curl_global_cleanup();
}