/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "squirrel/include/squirrel.h"
#include "squirrel/include/sqstdaux.h"
#include "squirrel/include/sqstdblob.h"
#include "squirrel/include/sqstdio.h"
#include "squirrel/include/sqstdmath.h"
#include "squirrel/include/sqstdstring.h"
#include "squirrel/include/sqstdsystem.h"
#include "squirrel_cpp_helper.h"
#include <applibs/log.h>
#include <applibs/storage.h>
#include <applibs/eventloop.h>
#include <applibs/networking.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "event_queue.h"
#include "json.h"
#include "pretty_print.h"
#include "http.h"
#include "gpio.h"

#define SQUIRREL_INITIAL_STACK_SIZE 256 ///< The initial stack size to assign to the Squirrel VM

extern "C" void __cxa_pure_virtual()
{
  while (1);
}

/// Directs the Squirrel 'print' function to Log_Debug.
/// \param vm the virtual machine the print call originated from.
/// \param format the format of the variable parameters list.
void print(HSQUIRRELVM vm, const SQChar* format, ...)
{
    va_list arglist;
    va_start(arglist, format);
    Log_DebugVarArgs(format, arglist);
}

/// Directs Squirrel compiler errors to Log_Debug.
/// \param vm the virtual machine the compiler error call originated from.
/// \param errorMessage the string representation of the error.
/// \param fileName the file the error originated from.
/// \param lineNumber the line number where the error occured.
/// \param columnNumber the column number where the error occured.
void printCompilerError(HSQUIRRELVM vm, const SQChar* errorMessage, const SQChar* fileName, SQInteger lineNumber, SQInteger columnNumber)
{
    Log_Debug("Compile Error in %s line: %u column: %u - Reason: %s\n", fileName, lineNumber, columnNumber, errorMessage);
}

/// Handles Squirrel runtime errors by retrieving the error string from the top of the stack (if present),
/// and providing a dump of the callsack via sqstd_printcallstack.
/// \param vm the virtual machine the runtime error originated from.
/// \returns '0'.
SQInteger errorHandler(HSQUIRRELVM vm)
{
    SQPRINTFUNCTION errorFunction = sq_geterrorfunc(vm);

    if(errorFunction)
    {
        const SQChar *errorString = 0;
        if(sq_gettop(vm) >= 1)
        {
            if(SQ_SUCCEEDED(sq_getstring(vm, 2, &errorString)))
            {
                errorFunction(vm, _SC("\nAN ERROR HAS OCCURRED [%s]\n"), errorString);
            }
            else
            {
                errorFunction(vm, _SC("\nAN ERROR HAS OCCURRED [unknown]\n"));
            }
            sqstd_printcallstack(vm);
        }
    }

    return 0;
}

/// Returns the position of the stack top for diagnostic purposes.
/// \param vm the Squirrel VM instance to use.
/// \returns The height of the stack (onto the Squirrel VM), '1' in C++.
SQInteger getStackTop(HSQUIRRELVM vm)
{
    sq_pushinteger(vm, sq_gettop(vm));
    return 1;
}

/// Application main entrypoint.
/// \returns '0' on success, '<0' on error.
int main(int argc, char **argv)
{
    // Print Squirrel VM version and copyright information
    Log_Debug("\nHLCore: %s %s (%d bits)\n\n", SQUIRREL_VERSION, SQUIRREL_COPYRIGHT, ((int)(sizeof(SQInteger)*8)));

    // Create an Event Loop to process system events
    EventLoop *eventLoop = EventLoop_Create();

    // Extract the name of the Squirrel file to compile from command-line arguments
    if(argc < 2)
    {
        Log_Debug("No source filename passed as a command-line parameter");
        return -1;
    }

    char* sourceNutFileName = argv[1];

    // Load the main.nut script into a buffer
    int mainNut = Storage_OpenFileInImagePackage(sourceNutFileName);
    if(mainNut < 0)
    {
        Log_Debug("Unable to open %s. Errno: %i\n", sourceNutFileName, errno);
        return -1;
    }

    off_t mainNutLen = lseek(mainNut, 0, SEEK_END);
    SQChar *mainNutBuffer = (SQChar*)malloc((size_t)mainNutLen);
    if(mainNutBuffer == NULL)
    {
        Log_Debug("Unable to create enough storage for %s.\n", sourceNutFileName);
        return -1;
    }

    lseek(mainNut, 0, SEEK_SET);
    read(mainNut, mainNutBuffer, (size_t)mainNutLen);
    close(mainNut);

    // Open a new VM
    HSQUIRRELVM vm;
    vm = sq_open(SQUIRREL_INITIAL_STACK_SIZE);

    // Attach print/error handling functions
    sq_setprintfunc(vm, &print, &print);
    sq_setcompilererrorhandler(vm, &printCompilerError);
    sq_newclosure(vm, errorHandler, 0);
    sq_seterrorhandler(vm);

    // Inject libraries
    sq_pushroottable( vm );

    sqstd_register_bloblib(vm);
    sqstd_register_iolib(vm);
    sqstd_register_systemlib(vm);
    sqstd_register_mathlib(vm);
    sqstd_register_stringlib(vm);

    EventQueue *eventQueue = EventQueue::registerWithSquirrelAsGlobal(vm, "hlCore");
    JSON *json = JSON::registerWithSquirrelAsGlobal(vm, "json");
    PrettyPrint *prettyPrint = PrettyPrint::registerWithSquirrelAsGlobal(vm, "prettyPrint");
    HTTP *http = HTTP::registerWithSquirrelAsGlobal(vm, eventLoop, "http");
    GPIO::registerWithSquirrelAsGlobal(vm, "hardware");

    SquirrelCppHelper::registerFunctionAsGlobal(vm, "getStackTop", getStackTop);

    sq_settop(vm, 0);

    // Compile main.nut and place the closure on the stack
    if(SQ_FAILED(sq_compilebuffer(vm, mainNutBuffer, (SQInteger)mainNutLen, sourceNutFileName, true)))
    {
        Log_Debug("Unable to compile %s.\n", sourceNutFileName);
        free(mainNutBuffer);
        return -1;
    }

    free(mainNutBuffer);
/*
    bool isNetworkingReady = false;

    while(!isNetworkingReady)
    {
        Networking_IsNetworkingReady(&isNetworkingReady);
    }
*/
    // Run the main.nut closure
    sq_pushroottable(vm);
    if(SQ_FAILED(sq_call(vm,1,false,true)))
    {
        Log_Debug("Execution of %s failed.\n", sourceNutFileName);
        return -2;
    }
    sq_collectgarbage(vm);

    // Process wake-up events in an infinite loop
    while(true)
    {
        EventLoop_Run(eventLoop, 0, true);

        if(eventQueue->process(vm))
        {
            sq_collectgarbage(vm);
        }
    }

    // The loop has been broken, close the VM and the application
    sq_close(vm);
    EventLoop_Close(eventLoop);

    Log_Debug("\n... HLCore: Squirrel Ended.\n");

    return 0;
}