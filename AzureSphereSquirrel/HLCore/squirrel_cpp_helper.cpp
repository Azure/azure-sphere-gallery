/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "squirrel_cpp_helper.h"
#include <stdio.h>

namespace SquirrelCppHelper
{
    int registerDelegateInRegistry(HSQUIRRELVM vm, const char* name, DelegateFunction delegateFunctions[], unsigned int numberOfFunctions)
    {
        // Fetch the top of the stack so that we can quickly revert if the call fails
        SQInteger top = sq_gettop(vm);

        // Place the registry table and delegate name onto the stack so that we can add the delegate table for easy shared lookup
        sq_pushregistrytable(vm);
        sq_pushstringex(vm, name, -1, SQTrue);

        // Create and place onto the stack a delegate table, to which we'll attach functionality
        sq_newtableex(vm, 1);

        for(unsigned int i = 0; i < numberOfFunctions; ++i)
        {
            // Attach the function as a static table entry
            sq_pushstringex(vm, delegateFunctions[i].name, -1, SQTrue);
            sq_pushlnativeclosure(vm, delegateFunctions[i].classMethod);
            if(SQ_FAILED(sq_newslot(vm, -3, true)))
            {
                sq_settop(vm, top);
                return -2;
            }
        }

        // Add the class instance to the registry table
        if(SQ_FAILED(sq_newslot(vm, -3, true)))
        {
            sq_settop(vm, top);
            return -3;
        }

        // Reset (clear) this function's impact on the stack
        sq_settop(vm, top);

        return 0;
    }

    int assignDelegateFromRegistry(HSQUIRRELVM vm, const char* name)
    {
        // Fetch the top of the stack so that we can quickly revert if the call fails
        SQInteger top = sq_gettop(vm);

        // Place the registry table and delegate name onto the stack
        sq_pushregistrytable(vm);
        sq_pushstringex(vm, name, -1, SQTrue);

        // Attempt to fetch the delegate from the table
        if(SQ_SUCCEEDED(sq_get(vm, -2)))
        {
            // Assign the delegate
            if(SQ_SUCCEEDED(sq_setdelegate(vm, -3)))
            {
                sq_settop(vm, top);
                return 0;
            }
            else
            {
                sq_settop(vm, top);
                return -2;    
            }
        }

        sq_settop(vm, top);
        return -1;        
    }

    int registerFunctionAsGlobal(HSQUIRRELVM vm, const char* name, SQFUNCTION function)
    {
        sq_pushroottable(vm);
        sq_pushstringex(vm, name, -1, SQTrue);
        sq_pushlnativeclosure(vm, function);
        if(SQ_FAILED(sq_newslot(vm, -3, SQTrue)))
        {
            sq_poptop(vm);
            return -1;
        }
        sq_poptop(vm);
        return 0;
    }

    SQInteger checkParameterTypes(HSQUIRRELVM vm, int numberOfParameters, int numberOfRequiredParameters, int types[])
    {
        SQInteger top = sq_gettop(vm);

        if(top < numberOfRequiredParameters+1)
        {
            return sq_throwerror(vm, "wrong number of parameters");
        }

        numberOfParameters = (top-1 < numberOfParameters) ? top : numberOfParameters+1;

        for(int i = 2; i <= numberOfParameters; ++i)
        {
            if(_RAW_TYPE(sq_gettype(vm, i) & types[i-2]) == 0)
            {
                SQChar *scratchPad = sq_getscratchpad(vm, 60);
                scsprintf(scratchPad, 60, "parameter %d has an invalid type", i-1 );
                return sq_throwerror(vm, scratchPad);
            }
        }

        return SQ_OK;
    }
}