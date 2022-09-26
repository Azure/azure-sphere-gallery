/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "json.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <applibs/log.h>
#include "squirrel/include/sqstdblob.h"

#define JSON_MAX_ENCODE_DEPTH       32  ///< Set the maximum encode depth to prevent against abuse/cyclical references.
#define JSON_MAX_TOKENS             512 ///< Set the maximum number of JSON tokens that may be decoded (avoids a malloc).
#define JSON_INITIAL_DECODE_SIZE    50  ///< Set the initial storage size of the decoded JSON string (a point of optimisation).

// Static Methods
//---------------
/// Registers the JSON class with Squirrel as a global (stored in the root table) singleton.
/// \param vm the instance of the VM to use.
/// \param name the name by which the class will be accessible from within Squirrel.
/// \returns a pointer to the singleton instance instantiated and placed within the root table.
JSON* JSON::registerWithSquirrelAsGlobal(HSQUIRRELVM vm, const char* name)
{
    SquirrelCppHelper::DelegateFunction delegateFunctions[2];
    delegateFunctions[0] = SquirrelCppHelper::DelegateFunction("decode", &JSON::SQUIRREL_METHOD_NAME(decode));
    delegateFunctions[1] = SquirrelCppHelper::DelegateFunction("encode", &JSON::SQUIRREL_METHOD_NAME(encode));

    return SquirrelCppHelper::registerClassAsGlobal<JSON>(vm, name, delegateFunctions, 2);
}

/// Grows the size of an sq_malloc allocated SQChar array to match at least the requiredSize.
/// Will overgrow proportionally to reduce the number of sq_reallocs.
/// \param storage a reference to the storage array to be grown.
/// \param storageSize a reference to the current size (in bytes) of the storage to be grown.
/// \param requiredSize the minimum size required to grow the storage to.
void JSON::growStorage(SQChar *&storage, SQInteger &storageSize, SQInteger requiredSize)
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
void JSON::fitStorage(SQChar *&storage, SQInteger storageSize, SQInteger finalSize)
{
    storage = (SQChar *)sq_realloc(storage, storageSize, finalSize);
}

// Squirrel Methods
//-----------------
/// Decodes a strict JSON string into a Squirrel object.
/// \param json the strict JSON to be decoded.
/// \returns the decoded Squirrel object.
/// \throws an error message if the JSON string could not be decoded.
SQUIRREL_METHOD_IMPL(JSON, decode)
{
    // Validate the number and type of parameters  
    int types[] = {OT_STRING};
    if(SQ_FAILED(SquirrelCppHelper::checkParameterTypes(vm, 1, 1, types)))
    {
        return SQ_ERROR;
    }

    // Retrieve the JSON string to be decoded
    SQChar* jsonString;
    SQInteger jsonStringLen;
    sq_getstringandsize(vm, 2, (const SQChar**)&jsonString, &jsonStringLen);

    // Parse the JSON string
    jsmn_init(&jsmn);
    int result = jsmn_parse(&jsmn, jsonString, jsonStringLen, tokenBuffer, JSON_MAX_TOKENS);
    if(result < 0)
    {
        return sq_throwerror(vm, "Unable to parse JSON");
    }

    // Construct a Squirrel object on the stack and insert the JSON entries
    if(SQ_FAILED(parseToken(vm, jsonString, tokenBuffer, 0)))
    {
        return sq_throwerror(vm, "Unable to convert JSON");
    }

    // Return the Squirrel object from the top of the stack
    return 1;
}

/// Encodes a Squirrel object into a strict JSON string.
/// \param value the Squirrel object to be encoded.
/// \returns the encoded JSON string.
/// \throws an error message if the JSON string could not be encoded. 
SQUIRREL_METHOD_IMPL(JSON, encode)
{
    // Validate the number and type of parameters 
    int types[] = {OT_TABLE|OT_ARRAY};
    if(SQ_FAILED(SquirrelCppHelper::checkParameterTypes(vm, 1, 1, types)))
    {
        return SQ_ERROR;
    }

    // Retrieve the Squirrel object to be encoded
    HSQOBJECT object;
    sq_resetobject(&object);
    sq_getstackobj(vm, 2, &object);

    // Prepare efficiently growable storage for the JSON string
    SQChar *jsonString = (SQChar*)sq_malloc(JSON_INITIAL_DECODE_SIZE);
    SQInteger jsonStringStorageSize = JSON_INITIAL_DECODE_SIZE;
    SQInteger jsonStringSize = 0;

    if(SQ_FAILED(encodeObject(vm, object, jsonString, jsonStringStorageSize, jsonStringSize)))
    {
        sq_free(jsonString, jsonStringStorageSize);
        return SQ_ERROR;
    }

    // Grow storage to account for '\0'
    growStorage(jsonString, jsonStringStorageSize, jsonStringSize+1);

    // NULL terminate the string
    jsonString[jsonStringSize] = '\0';

    // Copy and push the the JSON string onto the stack as a Squirrel String
    sq_pushstring(vm, jsonString, jsonStringSize);

    // Free the JSON string
    sq_free(jsonString, jsonStringStorageSize);

    // Return the JSON string from the top of the stack
    return 1;
}

// Methods
//--------
/// Parses recursively a JSON token produced by JSMN decode into a Squirrel object.
/// \param vm the instance of the VM to use.
/// \param jsonString the JSON string being parsed (used for data extraction).
/// \param tokens a list of parsed tokens.
/// \param tokenIndex the index of the currently parsing token in the tokens list.
/// \returns SQ_OK or SQ_ERROR if the JSON string could not be decoded.
/// \throws an error message if the JSON string could not be decoded.
SQInteger JSON::parseToken(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex)
{
    jsmntok_t *token = &tokens[tokenIndex];

    switch(token->type)
    {
        case jsmntype_t::JSMN_OBJECT: return parseObject(vm, jsonString, tokens, tokenIndex);
        case jsmntype_t::JSMN_ARRAY: return parseArray(vm, jsonString, tokens, tokenIndex);
        case jsmntype_t::JSMN_STRING: return parseString(vm, jsonString, tokens, tokenIndex);
        case jsmntype_t::JSMN_PRIMITIVE:
            switch(jsonString[token->start])
            {
                case 't': case 'f': return parseBool(vm, jsonString, tokens, tokenIndex);
                case 'n': return parseNull(vm, jsonString, tokens, tokenIndex);
                case '-': case '0': case '1': case '2':
                case '3': case '4': case '5': case '6':
                case '7': case '8': case '9': return parseNumber(vm, jsonString, tokens, tokenIndex);
                default: return sq_throwerror(vm, "Unknown primitive token");
            }
        break;
        case jsmntype_t::JSMN_UNDEFINED:
        default: return sq_throwerror(vm, "Unknown token type");
    }
}

/// Parses a JSON object token produced by JSMN decode into a Squirrel table.
/// \param vm the instance of the VM to use.
/// \param jsonString the JSON string being parsed (used for data extraction).
/// \param tokens a list of parsed tokens.
/// \param tokenIndex the index of the currently parsing token in the tokens list.
/// \returns SQ_OK or SQ_ERROR if the JSON object token could not be decoded.
/// \throws an error message if the JSON object token could not be decoded.
SQInteger JSON::parseObject(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex)
{
    jsmntok_t *token = &tokens[tokenIndex];

    int numberOfKeyPairs = token->size;
    int childTokenIndexStart = tokenIndex + 1;
    int childTokenIndexEnd = childTokenIndexStart + numberOfKeyPairs;

    sq_newtableex(vm, numberOfKeyPairs);
    
    for( int i = childTokenIndexStart; i < childTokenIndexEnd; i += 2)
    {
        if(SQ_FAILED(parseToken(vm, jsonString, tokens, i))) { return sq_throwerror(vm, "Unable to parse object token key"); }
        if(SQ_FAILED(parseToken(vm, jsonString, tokens, i+1))) { return sq_throwerror(vm, "Unable to parse object token value"); }
        sq_createslot(vm, -3);
    }

    return SQ_OK;
}

/// Parses a JSON array token produced by JSMN decode into a Squirrel array.
/// \param vm the instance of the VM to use.
/// \param jsonString the JSON string being parsed (used for data extraction).
/// \param tokens a list of parsed tokens.
/// \param tokenIndex the index of the currently parsing token in the tokens list.
/// \returns SQ_OK or SQ_ERROR if the JSON array token could not be decoded.
/// \throws an error message if the JSON array token could not be decoded.
SQInteger JSON::parseArray(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex)
{
    jsmntok_t *token = &tokens[tokenIndex];

    int numberOfElements = token->size;
    int childTokenIndexStart = tokenIndex + 1;
    int childTokenIndexEnd = childTokenIndexStart + numberOfElements;

    sq_newarray(vm, numberOfElements);
    
    for( int i = childTokenIndexStart, u = 0; i < childTokenIndexEnd; ++i, ++u)
    {
        if(SQ_FAILED(parseToken(vm, jsonString, tokens, i))) { return sq_throwerror(vm, "Unable to parse array token element"); }
        sq_arrayinsert(vm, -2, u);
    }

    return SQ_OK;
}

/// Parses a JSON string token produced by JSMN decode into a Squirrel string, unescaping characters as required.
/// \param vm the instance of the VM to use.
/// \param jsonString the JSON string being parsed (used for data extraction).
/// \param tokens a list of parsed tokens.
/// \param tokenIndex the index of the currently parsing token in the tokens list.
/// \returns SQ_OK or SQ_ERROR if the JSON string token could not be decoded.
/// \throws an error message if the JSON string token could not be decoded.
/// \todo Add support for unicode and unicode \u parsing
SQInteger JSON::parseString(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex)
{
    jsmntok_t *token = &tokens[tokenIndex];
    char *string = &jsonString[token->start];
    int stringLen = token->end - token->start;
    int finalStringLen = stringLen;

    for(int i = 0; i < stringLen-1; ++i)
    {
        if(string[i] == '\\')
        {
            switch(string[i+1])
            {
                case '"': case '\\': case '/':
                    string[i] = 255;
                    --finalStringLen;
                    ++i;
                    break;
                case 'b':
                    string[i] = 255;
                    string[i+1] = '\b';
                    --finalStringLen;
                    ++i;
                    break;
                case 'f':
                    string[i] = 255;
                    string[i+1] = '\f';
                    --finalStringLen;
                    ++i;
                    break;
                case 'n': 
                    string[i] = 255;
                    string[i+1] = '\n';
                    --finalStringLen;
                    ++i;
                    break;
                case 'r':
                    string[i] = 255;
                    string[i+1] = '\r';
                    --finalStringLen;
                    ++i;
                    break;
                case 't':
                    string[i] = 255;
                    string[i+1] = '\t';
                    --finalStringLen;
                    ++i;
                    break;
                case 'u':
                    unsigned int byteCode;

                    if(stringLen - i == 6)
                    {
                        byteCode = strtol(&string[i+2], nullptr, 16);
                    }
                    else if(stringLen - i > 6)
                    {
                        char tempChar = string[i+6];
                        string[i+6] = '\0';
                        byteCode = strtol(&string[i+2], nullptr, 16);
                        string[i+6] = tempChar;
                    }
                    else
                    {
                        return sq_throwerror(vm, "Unable to parse string token \\u");
                    }

                    if(byteCode >= 256)
                    {
                        return sq_throwerror(vm, "Unable to parse string token \\u, sizes above 1Byte not yet supported");
                    }

                    string[i] = 255;
                    string[i+1] = byteCode;
                    string[i+2] = 255;
                    string[i+3] = 255;
                    string[i+4] = 255;
                    string[i+5] = 255;
                    finalStringLen -= 5;
                    break;
                default: return sq_throwerror(vm, "Unable to parse string token, unknown escape sequence");;
            }
        }
    }

    for(int i = 0, u = 0; i < stringLen; ++i)
    {
        if(string[i] == 255)
        {
            continue;
        }

        string[u++] = string[i];
    }

    sq_pushstring(vm, string, finalStringLen);
    return SQ_OK;
}

/// Parses a JSON bool token produced by JSMN decode into a Squirrel bool.
/// \param vm the instance of the VM to use.
/// \param jsonString the JSON string being parsed (used for data extraction).
/// \param tokens a list of parsed tokens.
/// \param tokenIndex the index of the currently parsing token in the tokens list.
/// \returns SQ_OK or SQ_ERROR if the JSON bool token could not be decoded.
/// \throws an error message if the JSON bool token could not be decoded.
SQInteger JSON::parseBool(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex)
{
    jsmntok_t *token = &tokens[tokenIndex];
    sq_pushbool(vm, jsonString[token->start] == 't' ? true : false);
    return SQ_OK;
}

/// Parses a JSON null token produced by JSMN decode into a Squirrel null.
/// \param vm the instance of the VM to use.
/// \param jsonString the JSON string being parsed (used for data extraction).
/// \param tokens a list of parsed tokens.
/// \param tokenIndex the index of the currently parsing token in the tokens list.
/// \returns SQ_OK or SQ_ERROR if the JSON null token could not be decoded.
/// \throws an error message if the JSON null token could not be decoded.
SQInteger JSON::parseNull(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex)
{
    sq_pushnull(vm);
    return SQ_OK;
}

/// Parses a JSON number token produced by JSMN decode into a Squirrel integer or float.
/// \param vm the instance of the VM to use.
/// \param jsonString the JSON string being parsed (used for data extraction).
/// \param tokens a list of parsed tokens.
/// \param tokenIndex the index of the currently parsing token in the tokens list.
/// \returns SQ_OK or SQ_ERROR if the JSON number token could not be decoded.
/// \throws an error message if the JSON number token could not be decoded.
SQInteger JSON::parseNumber(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex)
{
    jsmntok_t *token = &tokens[tokenIndex];
    const char *string = &jsonString[token->start];
    int stringLen = token->end - token->start;

    for(int i = 0; i < stringLen; ++i)
    {
        if(!isdigit(string[i]))
        {
            sq_pushfloat(vm, strtof(string, nullptr));
            return SQ_OK;
        }
    }

    sq_pushinteger(vm, strtol(string, nullptr, 10));
    
    return SQ_OK;
}

/// Recursively encodes a Squirrel object into a strict JSON string.
/// \param vm the instance of the VM to use.
/// \param object the Squirrel object to be encoded.
/// \param jsonString the storage for the encoded JSON string (must be first initialied by sq_malloc and ultimately freed with sq_free by the user).
/// \param jsonStringStorageSize the size of the encoded JSON string storage.
/// \param jsonStringSize the size of the writen/decoded JSON string so far.
/// \param depth the current recursion depth (leave blank on first call).
/// \returns SQ_OK or SQ_ERROR if the Squirrel object could not be encoded.
/// \throws an error message if the Squirrel object could not be encoded.
SQInteger JSON::encodeObject(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth)
{
    // Determine if we've recursed too deep,
    // we're likely in a cicular reference and risk a stack overflow.
    if(depth > JSON_MAX_ENCODE_DEPTH)
    {
        return sq_throwerror(vm, "Maximum encode depth reached");
    }

    // Determine the type of the object to encode
    switch(sq_type(object))
    {
        case OT_TABLE:
        case OT_CLASS:
            if(SQ_FAILED(encodeClassTable(vm, object, jsonString, jsonStringStorageSize, jsonStringSize, depth)))
            {
                return SQ_ERROR;
            }   
            break;
        case OT_ARRAY:
            if(SQ_FAILED(encodeArray(vm, object, jsonString, jsonStringStorageSize, jsonStringSize, depth)))
            {
                return SQ_ERROR;
            }   
            break;
        case OT_STRING:
            if(SQ_FAILED(encodeString(vm, object, jsonString, jsonStringStorageSize, jsonStringSize, depth)))
            {
                return SQ_ERROR;
            }   
            break;
        case OT_INTEGER:
        case OT_FLOAT:
            if(SQ_FAILED(encodeNumber(vm, object, jsonString, jsonStringStorageSize, jsonStringSize, depth)))
            {
                return SQ_ERROR;
            }   
            break;
        case OT_BOOL:
            if(SQ_FAILED(encodeBool(vm, object, jsonString, jsonStringStorageSize, jsonStringSize, depth)))
            {
                return SQ_ERROR;
            }   
            break;           
        case OT_NULL:
            if(SQ_FAILED(encodeNull(vm, object, jsonString, jsonStringStorageSize, jsonStringSize, depth)))
            {
                return SQ_ERROR;
            }   
            break;
        case OT_INSTANCE:
            if(SQ_FAILED(encodeInstance(vm, object, jsonString, jsonStringStorageSize, jsonStringSize, depth)))
            {
                return SQ_ERROR;
            }   
            break;
        default: return sq_throwerror(vm, "Unserializable object encountered");            
    }

    return SQ_OK;
}

/// Recursively encodes a Squirrel class|table into a strict JSON string.
/// \param vm the instance of the VM to use.
/// \param object the Squirrel object to be encoded.
/// \param jsonString the storage for the encoded JSON string (must be first initialied by sq_malloc and ultimately freed with sq_free by the user).
/// \param jsonStringStorageSize the size of the encoded JSON string storage.
/// \param jsonStringSize the size of the writen/decoded JSON string so far.
/// \param depth the current recursion depth (leave blank on first call).
/// \returns SQ_OK or SQ_ERROR if the Squirrel class|table could not be encoded.
/// \throws an error message if the Squirrel class|table could not be encoded.
SQInteger JSON::encodeClassTable(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth)
{
    // Grow storage to account for '{'
    growStorage(jsonString, jsonStringStorageSize, jsonStringSize+1);

    jsonString[jsonStringSize++] = '{';

    // Push the table|class and a blank iterator to the stack
    sq_pushobject(vm, object);
    sq_pushnull(vm);

    // Take a snapshot of the jsonStringSize to see if there were properties in the table|class
    SQInteger outputSizeSnapshot = jsonStringSize;

    // Iterate over each entry within the table|class (will push key and value onto the stack)
    while(SQ_SUCCEEDED(sq_next(vm, -2)))
    {
        // Retrieve the key
        const SQChar* key;
        SQInteger keySize;
        sq_getstringandsize(vm, -2, &key, &keySize); // Ignore the return value, we know it's a string

        // Retrieve the value
        HSQOBJECT value;
        sq_resetobject(&value);
        sq_getstackobj(vm, -1, &value);

        // Pop both the key and value from the stack before we recurse to reduce stack depth impact
        sq_pop(vm, 2);

        // Ignore the entry if it refers to a function (we're only serializing properties)
        if(!sq_isfunction(value))
        {
            // Grow storage to account for '"key":'
            growStorage(jsonString, jsonStringStorageSize, jsonStringSize+3+keySize);

            // Encode the key
            jsonString[jsonStringSize++] = '"';
            memcpy(&jsonString[jsonStringSize], key, keySize);
            jsonStringSize += keySize;
            jsonString[jsonStringSize++] = '"';
            jsonString[jsonStringSize++] = ':';

            // Recurse to encode the value
            if(SQ_FAILED(encodeObject(vm, value, jsonString, jsonStringStorageSize, jsonStringSize, depth+1)))
            {
                return SQ_ERROR;
            }

            // Grow storage to account for ','
            growStorage(jsonString, jsonStringStorageSize, jsonStringSize+1);

            jsonString[jsonStringSize++] = ',';
        }
    }

    // Pop the iterator and object from the stack
    sq_pop(vm, 2);

    // If there were entries, place the closing '}' one space back to erase the trailing comma
    if(outputSizeSnapshot != jsonStringSize)
    {
        jsonString[jsonStringSize-1] = '}';
    }
    else
    {
        // Grow storage to account for '}'
        growStorage(jsonString, jsonStringStorageSize, jsonStringSize+1);

        jsonString[jsonStringSize++] = '}';
    }

    return SQ_OK;
}

/// Recursively encodes a Squirrel array into a strict JSON string.
/// \param vm the instance of the VM to use.
/// \param object the Squirrel object to be encoded.
/// \param jsonString the storage for the encoded JSON string (must be first initialied by sq_malloc and ultimately freed with sq_free by the user).
/// \param jsonStringStorageSize the size of the encoded JSON string storage.
/// \param jsonStringSize the size of the writen/decoded JSON string so far.
/// \param depth the current recursion depth (leave blank on first call).
/// \returns SQ_OK or SQ_ERROR if the Squirrel array could not be encoded.
/// \throws an error message if the Squirrel array could not be encoded.
SQInteger JSON::encodeArray(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth)
{
    // Grow storage to account for '['
    growStorage(jsonString, jsonStringStorageSize, jsonStringSize+1);

    jsonString[jsonStringSize++] = '[';

    // Push the array and a blank iterator to the stack
    sq_pushobject(vm, object);
    sq_pushnull(vm);

    // Take a snapshot of the jsonStringSize to see if there were elements in the array
    SQInteger outputSizeSnapshot = jsonStringSize;

    // Iterate over each entry within the array (will push index and value onto the stack)
    while(SQ_SUCCEEDED(sq_next(vm, -2)))
    {
        // Retrieve the value (we're ignoring the index)
        HSQOBJECT value;
        sq_resetobject(&value);
        sq_getstackobj(vm, -1, &value);

        // Pop both the index and value from the stack before we recurse to reduce stack depth impact
        sq_pop(vm, 2);

        // Recurse to encode the value
        if(SQ_FAILED(encodeObject(vm, value, jsonString, jsonStringStorageSize, jsonStringSize, depth+1)))
        {
            return SQ_ERROR;
        }

        // Grow storage to account for ','
        growStorage(jsonString, jsonStringStorageSize, jsonStringSize+1);

        jsonString[jsonStringSize++] = ',';
    }

    // Pop the iterator and object from the stack
    sq_pop(vm, 2);

    // If there were entries, place the closing ']' one space back to erase the trailing comma
    if(outputSizeSnapshot != jsonStringSize)
    {
        jsonString[jsonStringSize-1] = ']';
    }
    else
    {
        // Grow storage to account for ']'
        growStorage(jsonString, jsonStringStorageSize, jsonStringSize+1);

        jsonString[jsonStringSize++] = ']';
    }

    return SQ_OK;
}

/// Recursively encodes a Squirrel string into a strict JSON string.
/// \param vm the instance of the VM to use.
/// \param object the Squirrel object to be encoded.
/// \param jsonString the storage for the encoded JSON string (must be first initialied by sq_malloc and ultimately freed with sq_free by the user).
/// \param jsonStringStorageSize the size of the encoded JSON string storage.
/// \param jsonStringSize the size of the writen/decoded JSON string so far.
/// \param depth the current recursion depth (leave blank on first call).
/// \returns SQ_OK or SQ_ERROR if the Squirrel string could not be encoded.
/// \throws an error message if the Squirrel string could not be encoded.
SQInteger JSON::encodeString(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth)
{
    // Push the string to the stack
    sq_pushobject(vm, object);

    // Retrieve the string and string size from the stack
    const SQChar* string;
    SQInteger stringSize;
    sq_getstringandsize(vm, -1, &string, &stringSize); // Ignore the return value, we know there's a string here

    // Pop the object from the stack
    sq_poptop(vm);

    // Escape and encode the string
    if(SQ_FAILED(escapeAndEncode(vm, string, stringSize, jsonString, jsonStringStorageSize, jsonStringSize)))
    {
        return SQ_ERROR;
    }

    return SQ_OK;
}

/// Recursively encodes a Squirrel integer|float into a strict JSON string.
/// \param vm the instance of the VM to use.
/// \param object the Squirrel object to be encoded.
/// \param jsonString the storage for the encoded JSON string (must be first initialied by sq_malloc and ultimately freed with sq_free by the user).
/// \param jsonStringStorageSize the size of the encoded JSON string storage.
/// \param jsonStringSize the size of the writen/decoded JSON string so far.
/// \param depth the current recursion depth (leave blank on first call).
/// \returns SQ_OK or SQ_ERROR if the Squirrel integer|float could not be encoded.
/// \throws an error message if the Squirrel integer|float could not be encoded.
SQInteger JSON::encodeNumber(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth)
{
    // Push the integer|float to the stack and attempt to convert it to a string (which is pushed also)
    sq_pushobject(vm, object);
    sq_tostring(vm,-1); // Ignore the return value, it won't fail for a known integer|float

    // Retrieve the number converted string and string size from the stack
    const SQChar* string;
    SQInteger stringSize;
    sq_getstringandsize(vm, -1, &string, &stringSize); // Ignore the return value, we know there's a string here

    // Pop the converted number string and object from the stack
    sq_pop(vm, 2);

    // Grow storage to account for 'number'
    growStorage(jsonString, jsonStringStorageSize, jsonStringSize+stringSize);

    // Encode the converted number
    memcpy(&jsonString[jsonStringSize], string, stringSize);
    jsonStringSize += stringSize;

    return SQ_OK;
}

/// Recursively encodes a Squirrel bool into a strict JSON string.
/// \param vm the instance of the VM to use.
/// \param object the Squirrel object to be encoded.
/// \param jsonString the storage for the encoded JSON string (must be first initialied by sq_malloc and ultimately freed with sq_free by the user).
/// \param jsonStringStorageSize the size of the encoded JSON string storage.
/// \param jsonStringSize the size of the writen/decoded JSON string so far.
/// \param depth the current recursion depth (leave blank on first call).
/// \returns SQ_OK or SQ_ERROR if the Squirrel bool could not be encoded.
/// \throws an error message if the Squirrel bool could not be encoded.
SQInteger JSON::encodeBool(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth)
{
    // Determine if the bool is true or false
    if(object._unVal.nInteger)
    {
        // Grow storage to account for 'true'
        growStorage(jsonString, jsonStringStorageSize, jsonStringSize+4);

        // Encode the true bool
        jsonString[jsonStringSize]   = 't';
        jsonString[jsonStringSize+1] = 'r';
        jsonString[jsonStringSize+2] = 'u';
        jsonString[jsonStringSize+3] = 'e';
        jsonStringSize += 4;
    }
    else
    {
        // Grow storage to account for 'false'
        growStorage(jsonString, jsonStringStorageSize, jsonStringSize+5);

        // Encode the false bool
        jsonString[jsonStringSize]   = 'f';
        jsonString[jsonStringSize+1] = 'a';
        jsonString[jsonStringSize+2] = 'l';
        jsonString[jsonStringSize+3] = 's';
        jsonString[jsonStringSize+4] = 'e';
        jsonStringSize += 5;
    }

    return SQ_OK;
}

/// Recursively encodes a Squirrel null into a strict JSON string.
/// \param vm the instance of the VM to use.
/// \param object the Squirrel object to be encoded.
/// \param jsonString the storage for the encoded JSON string (must be first initialied by sq_malloc and ultimately freed with sq_free by the user).
/// \param jsonStringStorageSize the size of the encoded JSON string storage.
/// \param jsonStringSize the size of the writen/decoded JSON string so far.
/// \param depth the current recursion depth (leave blank on first call).
/// \returns SQ_OK or SQ_ERROR if the Squirrel null could not be encoded.
/// \throws an error message if the Squirrel null could not be encoded.
SQInteger JSON::encodeNull(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth)
{
    // Grow storage to account for 'null'
    growStorage(jsonString, jsonStringStorageSize, jsonStringSize+4);
    
    // Encode the null
    jsonString[jsonStringSize]   = 'n';
    jsonString[jsonStringSize+1] = 'u';
    jsonString[jsonStringSize+2] = 'l';
    jsonString[jsonStringSize+3] = 'l';
    jsonStringSize += 4;

    return SQ_OK;
}

/// Recursively encodes a Squirrel instance (including blobs) into a strict JSON string.
/// \param vm the instance of the VM to use.
/// \param object the Squirrel object to be encoded.
/// \param jsonString the storage for the encoded JSON string (must be first initialied by sq_malloc and ultimately freed with sq_free by the user).
/// \param jsonStringStorageSize the size of the encoded JSON string storage.
/// \param jsonStringSize the size of the writen/decoded JSON string so far.
/// \param depth the current recursion depth (leave blank on first call).
/// \returns SQ_OK or SQ_ERROR if the Squirrel instance could not be encoded.
/// \throws an error message if the Squirrel instance could not be encoded.
SQInteger JSON::encodeInstance(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth)
{
    // Push the instance to the stack and retrieve its typetag (that identifies if it's a special type)
    SQUserPointer typeTag;
    sq_pushobject(vm, object);
    sq_gettypetag(vm, -1, &typeTag);

    // Determine if the instance if of type 'blob', otherwise treat it generically
    if((SQUnsignedInteger)typeTag == 0x80000002)
    {
        // The instance was a blob, retrieve a reference to its inner data structure and size
        const SQChar* blobData;
        sqstd_getblob(vm, -1, (SQUserPointer*)&blobData);
        SQInteger blobDataSize = sqstd_getblobsize(vm, -1);

        // Pop the blob object from the stack
        sq_poptop(vm);

        // Escape and encode the blob
        if(SQ_FAILED(escapeAndEncode(vm, blobData, blobDataSize, jsonString, jsonStringStorageSize, jsonStringSize)))
        {
            return SQ_ERROR;
        }
    }
    else
    {
        // Determine if the instance has a '_serializeRaw' function (will pop the key and push result to the stack)
        sq_pushstringex(vm, "_serializeRaw", -1, SQTrue);
        if(SQ_SUCCEEDED(sq_get(vm, -2)) && sq_gettype(vm, -1) == OT_CLOSURE)
        {
            // Push the instance object to the stack as the 'this' parameter and call '_serializeRaw'
            sq_pushobject(vm, object);
            if(SQ_FAILED(sq_call(vm,1,true,true)))
            {
                return sq_throwerror(vm, "Unable to execute instance's _serializeRaw() function");
            }

            // Attempt to convert the returned value to a string, pushing the result to the stack
            if(SQ_FAILED(sq_tostring(vm, -1)))
            {
                return sq_throwerror(vm, "Instance's _serializeRaw did not produce a tostring(able) output");
            }

            // Retrieve the converted string and string size from the stack
            const SQChar* string;
            SQInteger stringSize;
            sq_getstringandsize(vm, -1, &string, &stringSize); // Ignore the return value, we know there's a string here

            // Grow storage to account for '"string"'
            growStorage(jsonString, jsonStringStorageSize, jsonStringSize+2+stringSize);

            jsonString[jsonStringSize++] = '"';
            memcpy(&jsonString[jsonStringSize], string, stringSize);
            jsonStringSize += stringSize;
            jsonString[jsonStringSize++] = '"';

            // Pop the instance object, function closure and converted string from the stack
            // (we're doing this here as the lifetime of the converted string is questionable)
            sq_pop(vm, 3);

            return SQ_OK;
        }

        // Determine if the instance has a '_serialize' function (will pop the key and push result to the stack)
        sq_pushstringex(vm, "_serialize", -1, SQTrue);
        if(SQ_SUCCEEDED(sq_get(vm, -2)) && sq_gettype(vm, -1) == OT_CLOSURE)
        {
            // Push the instance object to the stack as the 'this' parameter and call '_serialize'
            sq_pushobject(vm, object);
            if(SQ_FAILED(sq_call(vm,1,true,true)))
            {
                return sq_throwerror(vm, "Unable to execute instance's _serialize() function");
            }

            // Retrieve the returned object from the stack and recursively encode it
            HSQOBJECT returnedObject;
            sq_resetobject(&returnedObject);
            sq_getstackobj(vm, -1, &returnedObject);

            if(SQ_FAILED(encodeObject(vm, returnedObject, jsonString, jsonStringStorageSize, jsonStringSize, depth+1)))
            {
                return SQ_ERROR;
            }

            // Pop the instance object, function closure and returned object from the stack
            // (we're doing this here as the lifetime of the returned object is questionable)
            sq_pop(vm, 3);

            return SQ_OK;
        }

        // Determine if the instance is nexti iteratable by trying and seeing if an error was produced
        sq_reseterror(vm);

        // Grow storage to account for '{'
        growStorage(jsonString, jsonStringStorageSize, jsonStringSize+1);

        jsonString[jsonStringSize++] = '{';

        // Take a snapshot of the jsonStringSize to revert should nexti fail
        SQInteger outputSizeSnapshot = jsonStringSize;

        // Push a blank iterator to the stack
        sq_pushnull(vm);

        // Iterate over each entry within the instance via nexti (will push key and value onto the stack)
        while(SQ_SUCCEEDED(sq_next(vm, -2)))
        {
            // Retrieve the value
            HSQOBJECT value;
            sq_resetobject(&value);
            sq_getstackobj(vm, -1, &value);

            // Ignore the entry if it refers to a function (we're only serializing properties)
            if(!sq_isfunction(value))
            {
                // Attempt to convert the key to a string (nexti doesn't guarantee strings)
                if(SQ_FAILED(sq_tostring(vm, -2)))
                {
                    return sq_throwerror(vm, "Instance nexti key could not be converted into a string");
                }

                // Retrieve the converted string and string size from the stack
                const SQChar* key;
                SQInteger keySize;
                sq_getstringandsize(vm, -1, &key, &keySize); // Ignore the return value, we know there's a string here

                // Grow storage to account for '"key":'
                growStorage(jsonString, jsonStringStorageSize, jsonStringSize+3+keySize);

                // Encode the key
                jsonString[jsonStringSize++] = '"';
                memcpy(&jsonString[jsonStringSize], key, keySize);
                jsonStringSize += keySize;
                jsonString[jsonStringSize++] = '"';
                jsonString[jsonStringSize++] = ':';

                // Pop both the key, value and converted string from the stack before we recurse to reduce stack depth impact
                sq_pop(vm, 3);

                // Recurse to encode the value
                if(SQ_FAILED(encodeObject(vm, value, jsonString, jsonStringStorageSize, jsonStringSize, depth+1)))
                {
                    return SQ_ERROR;
                }

                // Grow storage to account for ','
                growStorage(jsonString, jsonStringStorageSize, jsonStringSize+1);

                jsonString[jsonStringSize++] = ',';
            }
            else
            {
                // Pop both the key and value from the stack
                sq_pop(vm, 2);
            }
        }

        // Pop the iterator from the stack (we're not done with the instance object yet)
        sq_poptop(vm);

        // If there were entries, place the closing '}' one space back to erase the trailing comma
        if(outputSizeSnapshot != jsonStringSize)
        {
            jsonString[jsonStringSize-1] = '}';
        }
        else
        {
            // Grow storage to account for '}'
            growStorage(jsonString, jsonStringStorageSize, jsonStringSize+1);

            jsonString[jsonStringSize++] = '}';
        }

        // Retrieve the last error onto and then from the stack, popping the retrieved value 
        sq_getlasterror(vm);
        HSQOBJECT subObject;
        sq_resetobject(&subObject);
        sq_getstackobj(vm, -1, &subObject);
        sq_poptop(vm);

        // If the last error was not null, nexti failed, instead, extract the instance's class
        // and iterate over this instead as a fallback
        if(!sq_isnull(subObject))
        {
            // Rewind the jsonStringSize position
            jsonStringSize = outputSizeSnapshot - 1;

            // Fetch the instance's class onto and from the stack, immediately popping the value and instance object
            sq_getclass(vm, -1);
            sq_resetobject(&subObject);
            sq_getstackobj(vm, -1, &subObject);
            sq_pop(vm, 2);

            // Recurse to encode the value (same depth as it's a fallback)
            if(SQ_FAILED(encodeObject(vm, subObject, jsonString, jsonStringStorageSize, jsonStringSize, depth)))
            {
                return SQ_ERROR;
            }
        }
        else
        {
            // Pop the instance object
            sq_poptop(vm);
        }
    }

    return SQ_OK;
}

/// Interprets byte data as a string and escapes it in a JSON compliant manner.
/// \param vm the instance of the VM to use.
/// \param data the data to be escaped as a string.
/// \param dataSize the size of the data to be escaped.
/// \param jsonString the storage for the encoded JSON string (must be first initialied by sq_malloc and ultimately freed with sq_free by the user).
/// \param jsonStringStorageSize the size of the encoded JSON string storage.
/// \param jsonStringSize the size of the writen/decoded JSON string so far.
/// \returns SQ_OK or SQ_ERROR if the data could not be escaped.
/// \throws an error message if the data could not be escaped.
SQInteger JSON::escapeAndEncode(HSQUIRRELVM vm, const SQChar *&data, SQInteger dataSize, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize)
{
    // Pre-grow storage to account for '"dataAsString"' to reduce number of mallocs
    growStorage(jsonString, jsonStringStorageSize, jsonStringSize+2+dataSize);

    jsonString[jsonStringSize++] = '"';

    for(int i = 0; i < dataSize; ++i)
    {
        unsigned char character = data[i];

        // Determine if the character can be represented as 7-bit ASCII or is unicode
        if((character & 0x80) == 0x00)
        {
            // Character was 7-bit ASCII, search for escape sequences
            switch(character)
            {
                case '"': case '\\': case '/': case '\b': case '\f':
                case '\n': case '\r': case '\t': case '\0':
                    growStorage(jsonString, jsonStringStorageSize, jsonStringSize+2);
                    jsonString[jsonStringSize++] = '\\';
                    break;
                default:
                    growStorage(jsonString, jsonStringStorageSize, jsonStringSize+1);
                    jsonString[jsonStringSize++] = character;
                    continue;
            }

            switch(character)
            {
                case '"': jsonString[jsonStringSize++] = '"'; break;
                case '\\': jsonString[jsonStringSize++] = '\\'; break;
                case '/': jsonString[jsonStringSize++] = '/'; break;
                case '\b': jsonString[jsonStringSize++] = 'b'; break;
                case '\f': jsonString[jsonStringSize++] = 'f'; break;
                case '\n': jsonString[jsonStringSize++] = 'n'; break;
                case '\r': jsonString[jsonStringSize++] = 'r'; break;
                case '\t': jsonString[jsonStringSize++] = 't'; break;
                case '\0':
                    growStorage(jsonString, jsonStringStorageSize, jsonStringSize+5);
                    jsonString[jsonStringSize]   = 'u';
                    jsonString[jsonStringSize+1] = '0';
                    jsonString[jsonStringSize+2] = '0';
                    jsonString[jsonStringSize+3] = '0';
                    jsonString[jsonStringSize+4] = '0';
                    jsonStringSize += 5;
                    break;
                default: break;
            }                
        }
        else
        {
            // Character was unicode, determine which format and encode
            if((character & 0xE0) == 0xC0)
            {
                // 2-byte unicode
                if(i+1 >= dataSize)
                {
                    return sq_throwerror(vm, "Unable to escape data as unicode");
                }
                
                growStorage(jsonString, jsonStringStorageSize, jsonStringSize+2);
                jsonString[jsonStringSize] = character;
                jsonString[jsonStringSize+1] = data[++i];
                jsonStringSize += 2;
            }
            else if((character & 0xF0) == 0xE0)
            {
                // 3-byte unicode
                if(i+2 >= dataSize)
                {
                    return sq_throwerror(vm, "Unable to escape data as unicode");
                }
                
                growStorage(jsonString, jsonStringStorageSize, jsonStringSize+3);
                jsonString[jsonStringSize] = character;
                jsonString[jsonStringSize+1] = data[++i];
                jsonString[jsonStringSize+2] = data[++i];
                jsonStringSize += 3;
            }
            else if((character & 0xF8) == 0xF0)
            {
                // 4-byte unicode
                if(i+3 >= dataSize)
                {
                    return sq_throwerror(vm, "Unable to escape data as unicode");
                }
                
                growStorage(jsonString, jsonStringStorageSize, jsonStringSize+4);
                jsonString[jsonStringSize] = character;
                jsonString[jsonStringSize+1] = data[++i];
                jsonString[jsonStringSize+2] = data[++i];
                jsonString[jsonStringSize+3] = data[++i];
                jsonStringSize += 4;
            }
        }
    }

    growStorage(jsonString, jsonStringStorageSize, jsonStringSize+1);
    jsonString[jsonStringSize++] = '"';

    return SQ_OK;
}

// Constructor/Destructor
//-----------------------
JSON::JSON()
{
    tokenBuffer = (jsmntok_t*)sq_malloc(JSON_MAX_TOKENS*sizeof(jsmntok_t));
}

JSON::~JSON()
{
    sq_free(tokenBuffer, JSON_MAX_TOKENS*sizeof(jsmntok_t));
}