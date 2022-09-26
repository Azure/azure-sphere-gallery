/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "pretty_print.h"
#include "json.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <applibs/log.h>

#define PRETTY_PRINT_INITIAL_SIZE   50      ///< Set the initial storage size of the Pretty string (a point of optimisation).
#define DEFAULT_INDENT_STRING       "    " ///< The default string to use to represent an indentation.

// Static Methods
//---------------
/// Registers the PrettyPrint class with Squirrel as a global (stored in the root table) singleton.
/// \param vm the instance of the VM to use.
/// \param name the name by which the class will be accessible from within Squirrel.
/// \returns a pointer to the singleton instance instantiated and placed within the root table.
PrettyPrint* PrettyPrint::registerWithSquirrelAsGlobal(HSQUIRRELVM vm, const char* name)
{
    SquirrelCppHelper::DelegateFunction delegateFunctions[1];
    delegateFunctions[0] = SquirrelCppHelper::DelegateFunction("print", &PrettyPrint::SQUIRREL_METHOD_NAME(print));

    return SquirrelCppHelper::registerClassAsGlobal<PrettyPrint>(vm, name, delegateFunctions, 1);
}

// Squirrel Methods
//-----------------
/// PrettyPrints a Squirrel object by JSON enocding and the prettifying it.
/// \param value the Squirrel object to be PrettyPrinted.
/// \throws an error message if the object could not be JSON encoded.
SQUIRREL_METHOD_IMPL(PrettyPrint, print)
{
    // Validate the number and type of parameters  
    int types[] = {0x0FFFFFFF};
    if(SQ_FAILED(SquirrelCppHelper::checkParameterTypes(vm, 1, 1, types)))
    {
        return SQ_ERROR;
    }

    // Fetch the JSON library instance from the root table and attempt to JSON encode the object
    sq_pushroottable(vm);
    sq_pushstringex(vm, "json", -1, SQTrue);
    sq_get(vm, -2); // This could fail but JSON should be there
    sq_pushstringex(vm, "encode", -1, SQTrue);
    sq_get(vm, -2); // This could fail but encode should be there
    sq_push(vm, -2);
    sq_push(vm, 2);

    if(SQ_FAILED(sq_call(vm,2,true,true)))
    {
        return SQ_ERROR;
    }

    // Retrieve the JSON string from the top of the stack
    const SQChar *jsonString;
    SQInteger jsonStringSize;
    sq_getstringandsize(vm, -1, &jsonString, &jsonStringSize); // Ignore error, we know there's a string here

    // Prepare efficiently growable storage for the Pretty string
    SQChar *prettyString = (SQChar*)sq_malloc(PRETTY_PRINT_INITIAL_SIZE);
    SQInteger prettyStringStorageSize = PRETTY_PRINT_INITIAL_SIZE;
    SQInteger prettyStringSize = 0;

    // Prettify the JSON string
    SQChar character;
    SQChar previousCharacter = 0;
    SQInteger indentLevel = 0;
    bool inQuotes = false;

    for(int i = 0; i < jsonStringSize; ++i)
    {
        character = jsonString[i];

        if(character == '"' && previousCharacter != '\\')
        {
            // We have either entered or exited a quoted string
            inQuotes = !inQuotes;
        }
        else if((character == '}' || character == ']') && !inQuotes)
        {
            // We have reached the end of an object, decrease the level of indent
            --indentLevel;

            // Move to the next line and add indentation (growing storage as required)
            growStorage(prettyString, prettyStringStorageSize, prettyStringSize+1+(indentLevel*4));
            prettyString[prettyStringSize++] = '\n';
            memset(&prettyString[prettyStringSize], ' ', indentLevel*4);
            prettyStringSize += indentLevel*4;
        }
        else if(character == ' ' && !inQuotes)
        {
            // Skip any spaces outside of quotes (we're not interested)
            ++i;
            continue;
        }

        // Grow storage to account for the character and add it to the Pretty string
        growStorage(prettyString, prettyStringStorageSize, prettyStringSize+1);
        prettyString[prettyStringSize++] = character;

        if((character == ',' || character == '{' || character == '[') && !inQuotes)
        {
            if(character == '{' || character == '[')
            {
                // We're at the start of an object, increase the level of indent
                ++indentLevel;
            }

            // Move to the next line and add indentation (growing storage as required)
            growStorage(prettyString, prettyStringStorageSize, prettyStringSize+1+(indentLevel*4));
            prettyString[prettyStringSize++] = '\n';
            memset(&prettyString[prettyStringSize], ' ', indentLevel*4);
            prettyStringSize += indentLevel*4;
        }
        else if(character == ':' && !inQuotes)
        {
            // Add a space between table key :value pairs (growing storage as required)
            growStorage(prettyString, prettyStringStorageSize, prettyStringSize+1);
            prettyString[prettyStringSize++] = ' ';
        }

        previousCharacter = character;
    }

    // '\0' terminate the string (growing storage as required)
    growStorage(prettyString, prettyStringStorageSize, prettyStringSize+1);
    prettyString[prettyStringSize++] = '\0';

    // Print the string and free
    SQPRINTFUNCTION print = sq_getprintfunc(vm);
    print(vm, prettyString);
    sq_free(prettyString, prettyStringStorageSize);

    return 0;
}

// Methods
//--------
/// Grows the size of an sq_malloc allocated SQChar array to match at least the requiredSize.
/// Will overgrow proportionally to reduce the number of sq_reallocs.
/// \param storage a reference to the storage array to be grown.
/// \param storageSize a reference to the current size (in bytes) of the storage to be grown.
/// \param requiredSize the minimum size required to grow the storage to.
void PrettyPrint::growStorage(SQChar *&storage, SQInteger &storageSize, SQInteger requiredSize)
{
    if(storageSize < requiredSize)
    {
        requiredSize = requiredSize + (requiredSize >> 1);
        storage = (SQChar *)sq_realloc(storage, storageSize, requiredSize);
        storageSize = requiredSize;
    }
}

/// Shrinks the size of an sq_malloc allocated SQChar array to match the finalSize.
/// \param storage a reference to the storage array to be shrunk to fit.
/// \param storageSize the current size (in bytes) of the storage to be shrunk to fit.
/// \param requiredSize the size required to fit the storage to.
void PrettyPrint::fitStorage(SQChar *&storage, SQInteger storageSize, SQInteger finalSize)
{
    storage = (SQChar *)sq_realloc(storage, storageSize, finalSize);
}

// Constructor/Destructor
//-----------------------
PrettyPrint::PrettyPrint()
{
}

PrettyPrint::~PrettyPrint()
{
}