/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef GPIO_H
#define GPIO_H

#include "squirrel_cpp_helper.h"

/// Provides multi-instance GPIO control capability to Squirrel.
class GPIO final
{
    // Attributes
    private:
        int pinNumber;  ///< The MT3620 pin number represented by the instance.
        int fd;         ///< The MT3620 file descriptor of the configured and opened pin.

    // Static Methods
    public:
        static void registerWithSquirrelAsGlobal(HSQUIRRELVM vm, const char* name);
        static SQInteger newGPIO(HSQUIRRELVM vm, int pinNumber); 

    // Squirrel Methods
    public:
        SQUIRREL_METHOD(configure);
        SQUIRREL_METHOD(disable);
        SQUIRREL_METHOD(read);
        SQUIRREL_METHOD(write);

    // Constructor/Destructor
    public:
        ~GPIO();
};

#endif