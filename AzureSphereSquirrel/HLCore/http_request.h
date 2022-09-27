/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "squirrel_cpp_helper.h"
#include <curl/curl.h>
#include <applibs/networking_curl.h>  // required only if using proxy to connect to the internet

/// This class holds the nessecary information to track a CURL HTTP(S) Request.
class HTTPRequest final
{
    // Attributes
    private:
        HSQUIRRELVM vm;                 ///< The VM upon which async requests should trigger their callbacks.
        CURLM *curlMulti;               ///< A reference to the CurlMulti handle that is processing this request, if async (otherwise NULL).
        CURL *request;                  ///< A reference to the CurlEasy handle that represents this request.
        SQChar *writeData;              ///< A pointer to the storage where the written (received) response body data will be placed.
        uint writeDataSize;             ///< The current size of the written (received) response body data.
        SQChar *readData;               ///< A pointer to the storage where read (transmit) request body data is stored, ready to be sent.
        SQChar *iReadData;              ///< A pointer to the next location in storage where the next read (transmit) request body data is to be taken from.
        size_t readDataRemaining;       ///< The current amount/size of read data remaining to be sent.
        HSQOBJECT writeHeaders;         ///< A Squirrel table containing the currently written (received) response headers.
        curl_slist *readHeaders;        ///< A linked-list of request headers to be sent.
        SQInteger responseStatusCode;   ///< The extracted response status code.
        SQBool isMulti;                 ///< A flag indicating if the request is multi (async) or not (sync).
        HSQOBJECT doneCallback;         ///< A ref-counted reference to the Squirrel doneCallback function, to be triggered upon async request completion.
        HSQOBJECT streamCallback;       ///< A ref-counted reference to the Squirrel streamCallback function, to be triggered upon async stream data receipt (UNUSED).
        HSQOBJECT self;                 ///< A ref-counted reference to the HTTPRequest object as it is stored in Squirrel (to ensure no premature deletion).

    // Static Methods
    public:
        static SQInteger newHTTPRequest(HSQUIRRELVM vm, CURLM *curlMulti, CURL *curlTemplate, const SQChar* verb, const SQChar* url, curl_slist *headers, SQChar *body = nullptr, SQInteger bodySize = 0); 

    // Squirrel Methods
    public:
        SQUIRREL_METHOD(cancel);
        SQUIRREL_METHOD(sendSync);
        SQUIRREL_METHOD(sendAsync);
        SQUIRREL_METHOD(setValidation);

    // Methods
    public:
        SQInteger processResult(CURLcode result);

        size_t curlReadCallback(void *buffer, size_t maxTransferSize);
        size_t curlWriteCallback(void *data, size_t dataSize);
        size_t curlWriteHeaderCallback(void *buffer, size_t headerSize);
        
    private:
        SQInteger constructRequest(HSQUIRRELVM vm, CURLM *curlMulti, CURL *curlTemplate, const SQChar* verb, const SQChar* url, curl_slist *headers, SQChar *body, SQInteger bodySize);

    // Constructor/Destructor
    public:
        ~HTTPRequest();
};

#endif