/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */
   
#ifndef SQUIRREL_CPP_HELPER_H
#define SQUIRREL_CPP_HELPER_H

#include "squirrel/include/squirrel.h"
#include "squirrel/include/new.h"

namespace SquirrelCppHelper
{
    #define SQUIRREL_METHOD(name)               \
    SQInteger name(HSQUIRRELVM vm);             \
    static SQInteger sq_##name(HSQUIRRELVM vm);

    #define SQUIRREL_METHOD_IMPL(className, name)                                                       \
    SQInteger className::sq_##name(HSQUIRRELVM vm)                                                      \
    {                                                                                                   \
        className *classInstance;                                                                       \
        if(SQ_FAILED(sq_getuserdata(vm, 1, (SQUserPointer*)&classInstance, nullptr)))                   \
        {                                                                                               \
            return sq_throwerror(vm, "Unable to retrieve a reference to the native Class instance");    \
        }                                                                                               \
        return classInstance->name(vm);                                                                 \
    }                                                                                                   \
    SQInteger className::name(HSQUIRRELVM vm)

    #define SQUIRREL_METHOD_NAME(name) sq_##name

    #define SQUIRREL_THROW_BAD_PARAMETER_TYPE(vm) sq_throwerror(vm, "Bad parameter type");

    /// A structure that contains all the nessecary information to construct a
    /// native function within an object's delegate table (templatable).
    struct DelegateFunction
    {
        const char *name; ///< The name of the function within the delegate table (its key) - must be const as a reference will be taken.
        SQFUNCTION classMethod; ///< A pointer to the native Class Method to be called when the delegate function is called.
        DelegateFunction() : name(nullptr), classMethod(nullptr) {};
        DelegateFunction(const char *name, SQFUNCTION classMethod) : name(name), classMethod(classMethod) {}
    };

    /// Wrapper for Class Destructors to allow them to be called as a release hook from Squirrel (templatable).
    /// \param object pointer to the Class instance to be destructed.
    /// \param size the size of the Class instance to be destructed (UNUSED).
    /// \returns '1'.
    template <class T>
    SQInteger callDestructorFromSquirrel(SQUserPointer object, SQInteger size)
    {
        ((T*)object)->~T();
        return 1;
    }

    /// Creates an instance of an object and places it on the stack as sq_userdata, where it will be ref counted (templatable).
    /// \note The object's destructor will be auto-assigned as a release hook.
    /// \param vm the instance of the VM to use.
    /// \returns a pointer to the instance that was placed on the stack otherwise 'nullptr'.
    template <class T>
    T* createInstanceOnStack(HSQUIRRELVM vm)
    {
        T* classInstance = (T*)sq_newuserdata(vm, sizeof(T));
        if(classInstance == nullptr)
        {
            return nullptr;
        }
        new(classInstance) T();

        sq_setreleasehook(vm, -1, callDestructorFromSquirrel<T>);

        return classInstance;
    }

    /// Creates an instance of an object and places it on the stack as sq_userdata, where it will be ref counted (templatable).
    /// This call will not construct the object, leaving it to user code.
    /// \note The object's destructor will be auto-assigned as a release hook.
    /// \param vm the instance of the VM to use.
    /// \returns a pointer to the instance that was placed on the stack otherwise 'nullptr'.
    template <class T>
    T* createInstanceOnStackNoConstructor(HSQUIRRELVM vm)
    {
        T* classInstance = (T*)sq_newuserdata(vm, sizeof(T));
        if(classInstance == nullptr)
        {
            return nullptr;
        }

        sq_setreleasehook(vm, -1, callDestructorFromSquirrel<T>);

        return classInstance;
    }

    /// Registers a native Class in the root table as a global instance, constructs and assigns a delegate table to expose functionality (templatable).
    /// \param vm the instance of the VM to use.
    /// \param name the name of the global instance that will be placed in the root table (its key) - must be const as a reference will be taken.
    /// \param delegateFunctions an array of native functions, their parameter settings and how they should appear in the delegate table.
    /// \param numberOfFunctions the number of functions within the delegateFunctions array to be added to the delegate.
    /// \returns a pointer to the newly created Class instance that was placed in the root table, else nullptr if there was an error.
    template <class T>
    T* registerClassAsGlobal(HSQUIRRELVM vm, const char* name, DelegateFunction delegateFunctions[], unsigned int numberOfFunctions )
    {
        // Fetch the top of the stack so that we can quickly revert if the call fails
        SQInteger top = sq_gettop(vm);

        // Place the root table and name of the global Class instance onto the stack
        sq_pushroottable(vm);
        sq_pushstringex(vm, name, -1, SQTrue);

        // Create storage for, place onto the stack and contruct the Class instance
        SQUserPointer classInstance = sq_newuserdata(vm, sizeof(T));
        if(classInstance == nullptr)
        {
            sq_settop(vm, top);
            return nullptr;
        }
        new(classInstance) T();

        // Retrieve/Create and place onto the stack a delegate table for the Class instance, to which we'll attach functionality
        sq_pushstringex(vm, name, -1, SQTrue);
        if(SQ_FAILED(sq_get(vm, -3)))
        {
            sq_newtableex(vm, 1);
        }
        
        for(unsigned int i = 0; i < numberOfFunctions; ++i)
        {
            // Attach the function as a static table entry
            sq_pushstringex(vm, delegateFunctions[i].name, -1, SQTrue);
            sq_pushlnativeclosure(vm, delegateFunctions[i].classMethod);
            if(SQ_FAILED(sq_newslot(vm, -3, true)))
            {
                sq_settop(vm, top);
                return nullptr;
            }
        }

        if(SQ_FAILED(sq_setdelegate(vm, -2)))
        {
            sq_settop(vm, top);
            return nullptr;
        }

        // Add the class instance to the root table
        if(SQ_FAILED(sq_newslot(vm, -3, true)))
        {
            sq_settop(vm, top);
            return nullptr;
        }

        // Reset (clear) this function's impact on the stack
        sq_settop(vm, top);

        return (T*)classInstance;
    } 

    /// Creates and registers a native delegate in the Squirrel VM Registry table (a shared table only accessible from C/C++), for reuse by instances (templatable).
    /// \note Any existing delegate will be replaced.
    /// \param vm the instance of the VM (and thus Registry table) to store the delegate within.
    /// \param name the name of the delegate (which will be referencable from the Registry table) - must be const as a reference will be taken.
    /// \param delegateFunctions an array of native functions, their parameter settings and how they should appear in the delegate table.
    /// \param numberOfFunctions the number of functions within the delegateFunctions array to be added to the delegate.
    /// \returns '0' on success, '<0' on failure.
    int registerDelegateInRegistry(HSQUIRRELVM vm, const char* name, DelegateFunction delegateFunctions[], unsigned int numberOfFunctions);

    /// Retrieves and assigns to the instance at the top of the stack a native delegate from the Squirrel VM Registry table (a shared table only accessible from C/C++).
    /// \param vm the instance of the VM (and thus Registry table) to retrieve the delegate from.
    /// \param name the name of the delegate (which will be referenced by the Registry table) - must be const as a reference will be taken.
    /// \returns '0' on success, '<0' on failure.
    int assignDelegateFromRegistry(HSQUIRRELVM vm, const char* name);

    /// Registers a native function in the root table as a global instance.
    /// \param vm the instance of the VM to use.
    /// \param name the name of the global function that will be placed in the root table (its key) - must be const as a reference will be taken.
    /// \param function a pointer to the function to be placed in the root table.
    /// \returns '0' on success, '<0' on failure.
    int registerFunctionAsGlobal(HSQUIRRELVM vm, const char* name, SQFUNCTION function);

    /// Checks that the correct number of parameters and correct type of parameters were provided to a Squirrel function.
    /// \param vm the instance of the VM to use.
    /// \param numberOfParameters the number of parameters to expect, including optional, excluding 'this'.
    /// \param numberOfRequiredParameters the number of required parameters, excluding 'this'.
    /// \param types a list of expected parameter types; multiple types may be masked together.
    /// \returns SQ_OK on success or SQ_ERROR (and throws) on failure.
    SQInteger checkParameterTypes(HSQUIRRELVM vm, int numberOfParameters, int numberOfRequiredParameters, int types[]);
}

#endif