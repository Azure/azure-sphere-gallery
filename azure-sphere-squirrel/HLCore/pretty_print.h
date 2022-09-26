/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef PRETTY_PRINT_H
#define PRETTY_PRINT_H

#include "squirrel_cpp_helper.h"
#include "json.h"

/// Provides PrettyPrint capability to Squirrel.
class PrettyPrint final
{
    // Static Methods
    public:
        static PrettyPrint* registerWithSquirrelAsGlobal(HSQUIRRELVM vm, const char* name);

    // Squirrel Methods
    public:
        SQUIRREL_METHOD(print);
        
    // Methods
    private:
        void growStorage(SQChar *&storage, SQInteger &storageSize, SQInteger requiredSize);
        void fitStorage(SQChar *&storage, SQInteger storageSize, SQInteger finalSize);

    // Constructor/Destructor
    public:
        PrettyPrint();
        ~PrettyPrint();
};

#endif