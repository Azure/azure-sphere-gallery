/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef JSON_H
#define JSON_H

#include "squirrel_cpp_helper.h"
#define JSMN_STATIC
#define JSMN_PARENT_LINKS
#define JSMN_STRICT
#include "jsmn/jsmn.h"

/// Provides JSON encode and decode capability to Squirrel.
class JSON final
{
    // Attributes
    private:
        jsmn_parser jsmn;
        jsmntok_t *tokenBuffer;

    // Static Methods
    public:
        static JSON* registerWithSquirrelAsGlobal(HSQUIRRELVM vm, const char* name);

    private:
        static void growStorage(SQChar *&storage, SQInteger &storageSize, SQInteger requiredSize);
        static void fitStorage(SQChar *&storage, SQInteger storageSize, SQInteger finalSize);

    // Squirrel Methods
    public:
        SQUIRREL_METHOD(decode);
        SQUIRREL_METHOD(encode);
        
    // Methods
    private:
        SQInteger parseToken(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex);
        SQInteger parseObject(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex);
        SQInteger parseArray(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex);
        SQInteger parseString(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex);
        SQInteger parseBool(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex);
        SQInteger parseNull(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex);
        SQInteger parseNumber(HSQUIRRELVM vm, char *jsonString, jsmntok_t tokens[], unsigned int tokenIndex);

        SQInteger encodeObject(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth = 0);
        SQInteger encodeClassTable(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth = 0);
        SQInteger encodeArray(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth = 0);
        SQInteger encodeString(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth = 0);
        SQInteger encodeNumber(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth = 0);
        SQInteger encodeBool(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth = 0);
        SQInteger encodeNull(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth = 0);
        SQInteger encodeInstance(HSQUIRRELVM vm, HSQOBJECT object, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize, SQInteger depth = 0);

        SQInteger escapeAndEncode(HSQUIRRELVM vm, const SQChar *&data, SQInteger dataSize, SQChar *&jsonString, SQInteger &jsonStringStorageSize, SQInteger &jsonStringSize);    

    // Constructor/Destructor
    public:
        JSON();
        ~JSON();
};

#endif