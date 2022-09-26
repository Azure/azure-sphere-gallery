/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "gpio.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <applibs/gpio.h>
#include <applibs/log.h>// temp for workaround

#define DIGITAL_OUT     0
#define DIGITAL_OUT_OD  1
#define DIGITAL_OUT_OS  2
#define DIGITAL_IN      3

// Static Methods
//---------------
/// Registers multiple instances of the class with Squirrel as a global (stored under 'name' in the root table).
/// \param vm the instance of the VM to use.
/// \param name the name under which the instances will be accessible from within Squirrel.
void GPIO::registerWithSquirrelAsGlobal(HSQUIRRELVM vm, const char* name)
{
    // Create a table which will hold all 80 GPIOs on the VM ready to push to the root table
    sq_pushroottable(vm);
    sq_pushstring(vm, name, -1);
    sq_newtableex(vm, 80);

    // Create an instance of GPIO for each pin and store under the key 'pinN'
    for(int i = 0; i <= 80; ++i)
    {
        char pinName[6];
        snprintf(pinName, 6, "pin%i", i); // We could maybe make this more efficient by statically creating these strings
        sq_pushstring(vm, pinName, -1);
        newGPIO(vm, i);
        sq_newslot(vm, -3, true);
    }

    // Place the GPIO table into the root table and then pop the root table
    sq_newslot(vm, -3, true);
    sq_poptop(vm);
}

/// Creates and configures a new instance of GPIO and pushes the object to the stack.
/// \param vm the instance of the VM to use.
/// \param pinNumber the MT3620 pin number to associate with the instance.
/// \returns '1' and places the generated GPIO object onto the stack. 
SQInteger GPIO::newGPIO(HSQUIRRELVM vm, int pinNumber)
{
    // Create a new GPIO instance and place it on the stack
    GPIO *gpio = SquirrelCppHelper::createInstanceOnStackNoConstructor<GPIO>(vm);

    gpio->pinNumber = pinNumber;
    gpio->fd = -1;

    // Assign (create if required) the delegate table to expose functionality in Squirrel
    if(SquirrelCppHelper::assignDelegateFromRegistry(vm, "Pin") < 0)
    {
        SquirrelCppHelper::DelegateFunction delegateFunctions[4];
        delegateFunctions[0] = SquirrelCppHelper::DelegateFunction("configure", &GPIO::SQUIRREL_METHOD_NAME(configure));
        delegateFunctions[1] = SquirrelCppHelper::DelegateFunction("disable", &GPIO::SQUIRREL_METHOD_NAME(disable));
        delegateFunctions[2] = SquirrelCppHelper::DelegateFunction("read", &GPIO::SQUIRREL_METHOD_NAME(read));
        delegateFunctions[3] = SquirrelCppHelper::DelegateFunction("write", &GPIO::SQUIRREL_METHOD_NAME(write));

        SquirrelCppHelper::registerDelegateInRegistry(vm, "Pin", delegateFunctions, 4);

        SquirrelCppHelper::assignDelegateFromRegistry(vm, "Pin");

        // Push some consts
        sq_pushconsttable(vm);
        sq_pushstringex(vm, "DIGITAL_OUT", -1, SQTrue);
        sq_pushinteger(vm, DIGITAL_OUT);
        sq_newslot(vm, -3, true);
        sq_pushstringex(vm, "DIGITAL_OUT_OD", -1, SQTrue);
        sq_pushinteger(vm, DIGITAL_OUT_OD);
        sq_newslot(vm, -3, true);
        sq_pushstringex(vm, "DIGITAL_OUT_OS", -1, SQTrue);
        sq_pushinteger(vm, DIGITAL_OUT_OS);
        sq_newslot(vm, -3, true);
        sq_pushstringex(vm, "DIGITAL_IN", -1, SQTrue);
        sq_pushinteger(vm, DIGITAL_IN);
        sq_newslot(vm, -3, true);
        sq_poptop(vm);
    }

    return 1;
}

// Squirrel Methods
//-----------------
/// Configures (opens) a GPIO pin as either input or output and various electrical modes.
/// \param mode the mode of the pin (DIGITAL_OUT, DIGITAL_OUT_OD, DIGITAL_OUT_OS, DIGITAL_IN).
/// \param initialState if configured as output, initial state is expected to set the initial logical state of the pin.
/// \throws an error message if the GPIO pin could not be configured.
SQUIRREL_METHOD_IMPL(GPIO, configure)
{
    int types[] = {OT_INTEGER};
    if(SQ_FAILED(SquirrelCppHelper::checkParameterTypes(vm, 1, 1, types)))
    {
        return SQ_ERROR;
    }

    SQInteger mode;
    sq_getinteger(vm, 2, &mode);

    switch(mode)
    {
        case DIGITAL_OUT:
        case DIGITAL_OUT_OD:
        case DIGITAL_OUT_OS:
        {
            SQObject transientObject;
            sq_getstackobj(vm, 3, &transientObject);
            if(!sq_isnumeric(transientObject))
            {
                return sq_throwerror(vm, "initialState was not a number");
            }
            SQInteger initialState = sq_objtointeger(&transientObject);

            fd = GPIO_OpenAsOutput(pinNumber, mode, initialState == 0 ? 0 : 1);

            if(fd == -1)
            {
                switch(errno)
                {
                    case EACCES: return sq_throwerror(vm, "Use is not permitted, please list the pin in the Gpio field of the application manifest");
                    case EBUSY: return sq_throwerror(vm, "The pin is already open");
                    default: return sq_throwerror(vm, "Internal error");
                }
            }
            break;
        }
        case DIGITAL_IN:
        {
            fd = GPIO_OpenAsInput(pinNumber);

            if(fd == -1)
            {
                switch(errno)
                {
                    case EACCES: return sq_throwerror(vm, "Use is not permitted, please list the pin in the Gpio field of the application manifest");
                    case EBUSY: return sq_throwerror(vm, "The pin is already open");
                    default: return sq_throwerror(vm, "Internal error");
                }
            }
            break;
        }
        default: return sq_throwerror(vm, "Pin: Invalid mode supplied");
    }

    return 0;
}

/// Disables (closes) a GPIO pin, returning it to an input state for reduced power consumption.
/// \throws an error message if the GPIO pin could not be configured during the closing process.
SQUIRREL_METHOD_IMPL(GPIO, disable)
{
    // Return if the pin isn't currently in-use
    if(fd == -1) { return 0; }

    // Close the pin, reopen it as an input (in order to switch modes) then close it again
    close(fd);

    fd = GPIO_OpenAsInput(pinNumber);

    if(fd == -1)
    {
        switch(errno)
        {
            case EACCES: return sq_throwerror(vm, "use is not permitted, please list the pin in the Gpio field of the application manifest");
            case EBUSY: return sq_throwerror(vm, "the pin is already open");
            default: return sq_throwerror(vm, "Internal error");
        }
    }

    close(fd);
    fd = -1;

    return 0;
}

/// Reads an input GPIO pin.
/// \returns the pin state as either '0' or '1'.
/// \throws an error message if the GPIO could not be read. 
SQUIRREL_METHOD_IMPL(GPIO, read)
{
    if(fd == -1) { return sq_throwerror(vm, "Has not been configured"); }

    GPIO_Value_Type value;
    if(GPIO_GetValue(fd, &value) == -1)
    {
        switch(errno)
        {
            case EFAULT: return sq_throwerror(vm, "Unable to read value");
            default: return sq_throwerror(vm, "Internal error");
        }
    }

    sq_pushinteger(vm, (SQInteger)value);

    return 1;
}

/// Write to an output GPIO pin.
/// \param value the pin value/state to write, may be a float or integer, must convert to 0 or 1.
/// \throws an error message if the GPIO could not be written. 
SQUIRREL_METHOD_IMPL(GPIO, write)
{
    if(fd == -1) { return sq_throwerror(vm, "Has not been configured"); }

    SQObject transientObject;
    sq_getstackobj(vm, 2, &transientObject);
    if(!sq_isnumeric(transientObject))
    {
        return sq_throwerror(vm, "Value was not a number");
    }
    SQInteger value = sq_objtointeger(&transientObject);

    if(GPIO_SetValue(fd, (GPIO_Value_Type)(value == 0 ? 0 : 1)) == -1)
    {
        switch(errno)
        {
            case EFAULT: return sq_throwerror(vm, "Unable to read value");
            default: return sq_throwerror(vm, "Internal error");
        }
    }

    return 0;
}

// Destructor
//-----------
GPIO::~GPIO()
{
    // Best-effort close the pin (if open) and configure it as an input for lower power usage
    if(fd == -1) { return; }
    close(fd);
    fd = GPIO_OpenAsInput(pinNumber);
    if(fd != -1) { close(fd); }
}