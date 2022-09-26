/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef HTTP_H
#define HTTP_H

#include "squirrel_cpp_helper.h"
#include <curl/curl.h>
#include <applibs/eventloop.h>
#include "eventloop_timer_utilities.h"

/// Provides Sync and Async HTTP Request capability to Squirrel
class HTTP final
{
    // Attributes
    private:
        CURL *curlTemplate;                 ///< A template CurlEasy handle use to create CurlEasy requests.
        CURLM *curlMulti;                   ///< A CurlMulti handle, handling async requests.
        EventLoop *eventLoop;               ///< A pointer to an async EventLoop for handling async actions.
        EventLoopTimer *curlTimeoutTimer;   ///< A pointer to an EventLoopTimer for handling async timeouts.
        int activeEasyHandles;              ///< The number of currently running async requests.
       
    // Static Methods
    public:
        static HTTP* registerWithSquirrelAsGlobal(HSQUIRRELVM vm, EventLoop *eventLoop, const char* name);

        static int curlTimerCallback(CURLM *multiHandle, long timeout_ms, void *httpUserData);
        static void curlTimerEventHandler(EventLoopTimer *timer);
        static int curlMSocketCallback(CURL *easyHandle, curl_socket_t socketFd, int action, void *httpUserData, void *socketUserData);
        static void curlFdEventHandler(EventLoop *eventLoop, int socketFd, EventLoop_IoEvents events, void *httpUserData);

        static size_t curlReadCallback(void *buffer, size_t size, size_t nitems, void *userdata);
        static size_t curlWriteCallback(void *data, size_t size, size_t nmemb, void *userp);
        static size_t curlWriteHeaderCallback(void *buffer, size_t size, size_t nitems, void *userdata);
        static int curlDebugCallback(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr);

    // Squirrel Methods
    public:
        SQUIRREL_METHOD(request);
        SQUIRREL_METHOD(get);
        SQUIRREL_METHOD(put);
        SQUIRREL_METHOD(post);
        SQUIRREL_METHOD(getDeviceIdFromDAA);
        
    // Methods
    private:
        SQInteger newRequest(HSQUIRRELVM vm);
        SQInteger retrieveAndCopyBody(HSQUIRRELVM vm, SQInteger stackLocation, SQChar *&body, SQInteger &bodySize);
        SQInteger generateHeaderList(HSQUIRRELVM vm, HSQOBJECT headers, curl_slist *&outputHeaderList);
        
        void curlProcessTransfers();

        int _curlTimerCallback(CURLM *multiHandle, long timeout_ms);
        int _curlMSocketCallback(CURL *easyHandle, curl_socket_t socketFd, int action, void *socketUserData);
        void _curlFdEventHandler(EventLoop *eventLoop, int socketFd, EventLoop_IoEvents events);
        void _curlTimerEventHandler(EventLoopTimer *timer);

    // Constructor
    public:
        int initialise(HSQUIRRELVM vm, EventLoop *eventLoop);
        ~HTTP();
};

#endif