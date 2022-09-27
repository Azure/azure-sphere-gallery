/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "http_request.h"
#include <string.h>
#include <ctype.h>
#include <applibs/storage.h>// temp for workaround
#include <applibs/log.h>// temp for workaround
#include <tlsutils/deviceauth_curl.h>
#include "http.h"// temp for workaround

// Static Methods
//---------------
/// Creates and configures new HTTPRequest and places it upon the stack.
/// \param vm the VM in which to create the HTTPRequest object.
/// \param curlMulti a reference to a CurlMulti handle to use in async requests.
/// \param curlTemplate a template curl request, providing base/shared parameters for duplication.
/// \param verb the verb of the HTTP request e.g. "GET".
/// \param url the url of the HTTP request e.g. https://www.microsoft.com/
/// \param headers a Curl-compatible linked-list of HTTP headers to be sent as part of the request (must be created with curl_slist_append, HTTPRequest will take ownership and call curl_slist_free_all).
/// \param body the data body to be sent as part of the request (must be created with sq_malloc, HTTPRequest will take ownership and call sq_free).
/// \param bodySize the size of the data body.
SQInteger HTTPRequest::newHTTPRequest(HSQUIRRELVM vm, CURL *curlMulti, CURL *curlTemplate, const SQChar* verb, const SQChar* url, curl_slist *headers, SQChar *body, SQInteger bodySize)
{
    // Create a new HTTPRequest instance and place it on the stack
    HTTPRequest *request = SquirrelCppHelper::createInstanceOnStackNoConstructor<HTTPRequest>(vm);

    // Assign (create if required) the delegate table to expose functionality in Squirrel
    if(SquirrelCppHelper::assignDelegateFromRegistry(vm, "HTTPRequest") < 0)
    {
        SquirrelCppHelper::DelegateFunction delegateFunctions[4];
        delegateFunctions[0] = SquirrelCppHelper::DelegateFunction("cancel", &HTTPRequest::SQUIRREL_METHOD_NAME(cancel));
        delegateFunctions[1] = SquirrelCppHelper::DelegateFunction("sendSync", &HTTPRequest::SQUIRREL_METHOD_NAME(sendSync));
        delegateFunctions[2] = SquirrelCppHelper::DelegateFunction("sendAsync", &HTTPRequest::SQUIRREL_METHOD_NAME(sendAsync));
        delegateFunctions[3] = SquirrelCppHelper::DelegateFunction("setValidation", &HTTPRequest::SQUIRREL_METHOD_NAME(setValidation));

        SquirrelCppHelper::registerDelegateInRegistry(vm, "HTTPRequest", delegateFunctions, 4);

        SquirrelCppHelper::assignDelegateFromRegistry(vm, "HTTPRequest");
    }

    SQInteger result = request->constructRequest(vm, curlMulti, curlTemplate, verb, url, headers, body, bodySize);

    if(result < 0)
    {
        return result;
    }

    return 1;
}

// Squirrel Methods
//-----------------
/// Unimplemented - will ultimately cancel an ongoing async request.
SQUIRREL_METHOD_IMPL(HTTPRequest, cancel)
{
    return 0;
}

/// Will begin a sync request and return only once the request is complete.
/// \returns the request result as a table of the format { "statusCode": nnn, "headers": {}, "body": "" }.
/// \note If statusCode is < 100, it represents a Curl Error Code.
SQUIRREL_METHOD_IMPL(HTTPRequest, sendSync)
{
    if(request == nullptr || isMulti)
    {
        return sq_throwerror(vm, "HTTPRequest: Request has already been used");
    }

    isMulti = false;
    CURLcode result = curl_easy_perform(request);
    return processResult(result);
}

/// Will begin an async request, triggering the provided doneCallback when complete.
/// \param doneCallback a function that will be executed upon completion of the request, taking one parameter 'result' as a table of the format { "statusCode": nnn, "headers": {}, "body": "" }.
/// \param streamCallback a function for streaming requests (OPTIONAL and UNUSED).
/// \param timeout a timeout in seconds for streaming requests (OPTIONAL and UNUSED).
/// \note If statusCode is < 100, it represents a Curl Error Code.
SQUIRREL_METHOD_IMPL(HTTPRequest, sendAsync)
{
    int types[] = {OT_CLOSURE, OT_CLOSURE, OT_INTEGER | OT_FLOAT};
    if(SQ_FAILED(SquirrelCppHelper::checkParameterTypes(vm, 3, 1, types)))
    {
        return SQ_ERROR;
    }

    if(request == nullptr || isMulti)
    {
        return sq_throwerror(vm, "HTTPRequest: Request has already been used");
    }

    // Fetch the method parameters
    sq_getstackobj(vm, 1, &self);
    sq_addref(vm, &self);

    sq_getstackobj(vm, 2, &doneCallback);
    sq_addref(vm, &doneCallback);

    sq_getstackobj(vm, 3, &streamCallback);
    sq_addref(vm, &streamCallback);

    //SQInteger streamTimeout;
    //sq_getinteger(vm, 4, &streamTimeout);

    isMulti = true;
    CURLMcode result = curl_multi_add_handle(curlMulti, request);
    return 0;
}

/// Unimplemented - will ultimately enforce HTTPS.
SQUIRREL_METHOD_IMPL(HTTPRequest, setValidation)
{
    return 0;
}

// Methods
//--------
/// Processes the results of a completed Curl request, cleaning-up as required
/// and constructing the result table to be placed onto the stack.
/// \param result the Curl request result code (used for sync requests only).
/// \returns places the result table onto the stack and returns '1' for sync requests or calls the doneCallback for async requests.
SQInteger HTTPRequest::processResult(CURLcode result)
{
    // Cleanup the CURL request (setting to nullptr to indicate complete) and free the read (sent) headers and data
    if(isMulti)
    {
        curl_multi_remove_handle(curlMulti, request);
    }    
    curl_easy_cleanup(request);
    request = nullptr;
    curl_slist_free_all(readHeaders);
    if(readData != nullptr) { free(readData); }

    // Create a table to hold the results of the request
    sq_newtableex(vm, 3);

    // Store the response status code
    sq_pushstringex(vm, "statusCode", -1, SQTrue);
    if(result != CURLE_OK)
    {
        sq_pushinteger(vm, result);
    }
    else
    {
        sq_pushinteger(vm, responseStatusCode);
    }
    sq_newslot(vm, -3, false);

    // Store the write (received) data/body in the table and release the data structure
    sq_pushstringex(vm, "body", -1, SQTrue);
    sq_pushstring(vm, (const SQChar*)writeData, writeDataSize);
    sq_newslot(vm, -3, false);
    free(writeData);

    // Store the write (received) headers in the table and release the extra reference
    sq_pushstringex(vm, "headers", -1, SQTrue);
    sq_pushobject(vm, writeHeaders);
    sq_newslot(vm, -3, false);
    sq_release(vm, &writeHeaders);

    if(isMulti)
    {
        // Call the doneCallback
        sq_pushobject(vm, doneCallback);
        sq_pushroottable(vm);
        sq_push(vm, -3);
        if(SQ_FAILED(sq_call(vm,2,false,true)))
        {
            Log_Debug("Execution of doneCallback failed.\n");
            return -1;
        }
        sq_pop(vm, 2);

        sq_release(vm, &self);
        if(doneCallback._type != OT_NULL) { sq_release(vm, &doneCallback); }
        if(streamCallback._type != OT_NULL) { sq_release(vm, &streamCallback); }

        sq_collectgarbage(vm);
        isMulti = false;
    }

    return 1;
}

/// Constructs the HTTPRequest.
/// \param vm the VM in which to create internal objects and issue callbacks.
/// \param curlMulti a reference to a CurlMulti handle to use in async requests.
/// \param curlTemplate a template curl request, providing base/shared parameters for duplication.
/// \param verb the verb of the HTTP request e.g. "GET".
/// \param url the url of the HTTP request e.g. https://www.microsoft.com/
/// \param headers a Curl-compatible linked-list of HTTP headers to be sent as part of the request (must be created with curl_slist_append, HTTPRequest will take ownership and call curl_slist_free_all).
/// \param body the data body to be sent as part of the request (must be created with sq_malloc, HTTPRequest will take ownership and call sq_free).
/// \param bodySize the size of the data body.
/// \returns SQ_OK on Success, otherwise SQ_ERROR.
/// \throws Throws an error message in Squirrel upon error.
SQInteger HTTPRequest::constructRequest(HSQUIRRELVM vm, CURLM *curlMulti, CURL *curlTemplate, const SQChar* verb, const SQChar* url, curl_slist *headers, SQChar *body, SQInteger bodySize)
{
    // Create a new CURL request based on the provided template and its parameters
    request = curl_easy_init();
    
    if(request == nullptr)
    {
        return sq_throwerror(vm, "Unable to create CURL handle");
    }
  
    CURLcode result;
    result = curl_easy_setopt(request, CURLOPT_CUSTOMREQUEST, verb);
    if(result != CURLcode::CURLE_OK) { curl_easy_cleanup(request); return sq_throwerror(vm, curl_easy_strerror(result)); }
    result = curl_easy_setopt(request, CURLOPT_URL, url);
    if(result != CURLcode::CURLE_OK) { curl_easy_cleanup(request); return sq_throwerror(vm, curl_easy_strerror(result)); }
    result = curl_easy_setopt(request, CURLOPT_HTTPHEADER, headers);
    if(result != CURLcode::CURLE_OK) { curl_easy_cleanup(request); return sq_throwerror(vm, curl_easy_strerror(result)); }
    if(bodySize > 0)
    {
        result = curl_easy_setopt(request, CURLOPT_READDATA, (void*)this);
        if(result != CURLcode::CURLE_OK) { curl_easy_cleanup(request); return sq_throwerror(vm, curl_easy_strerror(result)); }
        result = curl_easy_setopt(request, CURLOPT_UPLOAD, 1L);
        if(result != CURLcode::CURLE_OK) { curl_easy_cleanup(request); return sq_throwerror(vm, curl_easy_strerror(result)); }
        result = curl_easy_setopt(request, CURLOPT_INFILESIZE, bodySize);
        if(result != CURLcode::CURLE_OK) { curl_easy_cleanup(request); return sq_throwerror(vm, curl_easy_strerror(result)); }
    }
    result = curl_easy_setopt(request, CURLOPT_WRITEDATA, (void*)this);
    if(result != CURLcode::CURLE_OK) { curl_easy_cleanup(request); return sq_throwerror(vm, curl_easy_strerror(result)); }        
    result = curl_easy_setopt(request, CURLOPT_HEADERDATA, (void*)this);
    if(result != CURLcode::CURLE_OK) { curl_easy_cleanup(request); return sq_throwerror(vm, curl_easy_strerror(result)); }        

    // Temporary options to deal with duphandle not working
    // Enable redirect following
    result = curl_easy_setopt(request, CURLOPT_FOLLOWLOCATION, 1L);
    // Restrict requests to HTTP(S)
    long allowedProtocols = CURLPROTO_HTTP | CURLPROTO_HTTPS;
    result = curl_easy_setopt(request, CURLOPT_PROTOCOLS, allowedProtocols);
    // Restrict redirects to HTTP(S)
    result = curl_easy_setopt(request, CURLOPT_REDIR_PROTOCOLS, allowedProtocols);
    // Load the HTTPS certificate(s)
    char *certificatePath = Storage_GetAbsolutePathInImagePackage("certs/CA.cer");
    result = curl_easy_setopt(request, CURLOPT_CAINFO, certificatePath);
    // Turn of verbose messaging (for performance/space)
    result = curl_easy_setopt(request, CURLOPT_VERBOSE, 0L);
    //result = curl_easy_setopt(request, CURLOPT_DEBUGFUNCTION, HTTP::curlDebugCallback);
    // Use proxy
    //Networking_Curl_SetDefaultProxy(curlTemplate);
    // Attach function to handle downloaded data
    result = curl_easy_setopt(request, CURLOPT_WRITEFUNCTION, HTTP::curlWriteCallback);
    // Attach function to handle downloaded headers
    result = curl_easy_setopt(request, CURLOPT_HEADERFUNCTION, HTTP::curlWriteHeaderCallback);
    // Attach function to handle uploaded data
    result = curl_easy_setopt(request, CURLOPT_READFUNCTION, HTTP::curlReadCallback);
    // Attach a reference to this for use in Multi to determine the owning object of the curl easy request
    result = curl_easy_setopt(request, CURLOPT_PRIVATE, this);
    
    // Setup data storage
    isMulti = false;
    this->vm = vm;
    this->curlMulti = curlMulti;
    responseStatusCode = -1;
    readData = body;
    iReadData = body;
    readDataRemaining = bodySize;
    readHeaders = headers;
    writeData = (SQChar*)malloc(0);
    writeDataSize = 0;
    sq_newtable(vm);
    sq_resetobject(&writeHeaders);
    sq_getstackobj(vm, -1, &writeHeaders);
    sq_addref(vm, &writeHeaders);
    sq_poptop(vm);
    sq_resetobject(&doneCallback);
    sq_resetobject(&streamCallback);

    return SQ_OK;
}

/// Handles Curl read requests, providing the next piece of data to be sent as part of the request.
/// \param buffer the read buffer provided by Curl for placing the request data into.
/// \param maxTransferSize the maximum amount of space within buffer for this transfer.
/// \returns the number of bytes placed/transfered into the buffer.
size_t HTTPRequest::curlReadCallback(void *buffer, size_t maxTransferSize)
{
    size_t transferSize = maxTransferSize < readDataRemaining ? maxTransferSize : readDataRemaining;
    memcpy(buffer, (void*)iReadData, transferSize);
    iReadData += transferSize;
    readDataRemaining -= transferSize;
    return transferSize;
}

/// Handles Curl write requests, expanding and placing request response data into a buffer.
/// \param data the write buffer provided by Curl containing the data to be received.
/// \param dataSize the amount of received data contained within the write buffer for receipt.
/// \returns the number of bytes removed/received from the buffer.
size_t HTTPRequest::curlWriteCallback(void *data, size_t dataSize)
{
    writeData = (SQChar*)realloc((void*)writeData, writeDataSize + dataSize);
    memcpy((void*)(writeData + writeDataSize), data, dataSize);
    writeDataSize += dataSize;
    return dataSize;
}

/// Handles Curl write header request, receiving header data into a Squirrel table and extracting the response statusCode.
/// \param buffer the write buffer provided by Curl containing the header data to be received.
/// \param headerSize the amount of received header data contained within the write buffer for receipt.
/// \returns the number of bytes removed/received from the buffer.
/// \note We assume curl will only provide compliant output.
size_t HTTPRequest::curlWriteHeaderCallback(void *buffer, size_t headerSize)
{
    if(responseStatusCode == -1)
    {
        // Extract the status code from the header
        SQChar* space = (SQChar*)memchr(buffer, ' ', headerSize);
        *(space+4) = '\n';
        responseStatusCode = atoi(space+1);
    }
    else
    {
        SQChar *colon = (SQChar*)memchr(buffer, ':', headerSize);

        if(colon != nullptr)
        {
            // Convert the buffer to lower-case for consistency
            for(size_t i = 0; i < headerSize; ++i)
            {
                ((SQChar*)buffer)[i] = tolower(((SQChar*)buffer)[i]);
            }

            // Place the header field into the headers table
            sq_pushobject(vm, writeHeaders);
            sq_pushstring(vm, (const SQChar*)buffer, colon-(SQChar*)buffer);
            
            // Strip leading whitespace from the header value
            for(++colon; colon < (SQChar*)buffer + headerSize; ++colon)
            {
                if(*colon != ' ')
                {
                    break;
                }
            }
            
            // Place the header value into the headers table, removing the \r\n ending
            sq_pushstring(vm, (const SQChar*)colon, (((SQChar*)buffer)+headerSize)-(colon+2));
            sq_newslot(vm,-3,false);
            sq_poptop(vm);
        }
        else if(headerSize == 2 && strncmp((const char*)buffer, "\r\n", 2) == 0 && responseStatusCode == 100)
        {
            responseStatusCode = -1;
        }
    }

    return headerSize;    
}

// Destructor
//-----------
HTTPRequest::~HTTPRequest()
{
    if(request)
    {
        if(isMulti)
        {
            curl_multi_remove_handle(curlMulti, request);
            if(doneCallback._type != OT_NULL) { sq_release(vm, &doneCallback); }
            if(streamCallback._type != OT_NULL) { sq_release(vm, &streamCallback); }
            isMulti = false;
        }   
        curl_easy_cleanup(request);
        curl_slist_free_all(readHeaders);
        if(readData != nullptr) { free(readData); }
        free(writeData);
        sq_release(vm, &writeHeaders);
    }
}