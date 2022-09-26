/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
   
#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include "squirrel_cpp_helper.h"

class EventQueue final
{
    // Structures
    private:
        struct Event;

    // Attributes
    public:
        Event *events;

    // Static Methods
    public:
        static EventQueue* registerWithSquirrelAsGlobal(HSQUIRRELVM vm, const char* name);

    // Squirrel Methods
    public:
        SQUIRREL_METHOD(wakeup);

    // Methods
    public:
        bool process(HSQUIRRELVM vm);

    // Constructor/Destructor
    public:
        EventQueue();
        ~EventQueue();
};

#endif