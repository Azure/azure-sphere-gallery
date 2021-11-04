/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>

#include <applibs/eventloop.h>
#include "exitcodes.h"

/// <summary>
/// Initialize the user interface.
/// </summary>
/// <returns>An <see cref="ExitCode" /> indicating success or failure.</returns>
ExitCode UserInterface_Initialize(void);

/// <summary>
/// Close and clean up the user interface.
/// </summary>
/// <param name=""></param>
void UserInterface_Cleanup(void);

/// <summary>
/// Set the status of the status LED.
/// </summary>
/// <param name="status">Status LED status.</param>
void UserInterface_SetStatus(int index, bool status);
