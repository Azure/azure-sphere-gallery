/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef AZURE_SPHERE_COMMON_H_
#define AZURE_SPHERE_COMMON_H_

/// <summary>Returned when a function or operation succeeds.</summary>
#define ERROR_NONE           (   0)

/// <summary>Returned when an unspecified error occurs.</summary>
#define ERROR                (  -1)

/// <summary>Returned when an operation is attempted while a resource is locked.</summary>
#define ERROR_BUSY           (  -2)

/// <summary>Returned when an operation fails due to timeout.</summary>
#define ERROR_TIMEOUT        (  -3)

/// <summary>Returned when use of an unsupported feature is attempted.</summary>
#define ERROR_UNSUPPORTED    (  -4)

/// <summary>Returned when an operation is invoked with an invalid parameter.</summary>
#define ERROR_PARAMETER      (  -5)

/// <summary>Returned when an operation requires DMA but the data is not located in
/// DMA accessible memory.</summary>
#define ERROR_DMA_SOURCE     (  -6)

/// <summary>Returned when an operation is requested on a handle that's closed.</summary>
#define ERROR_HANDLE_CLOSED  (  -7)

/// <summary>Returned when an operation fails due to invalid or unexpected hardware state.</summary>
#define ERROR_HARDWARE_STATE (  -8)

/// <summary>This is not an error itself but is used by other modules as an offset for module
/// or driver specific errors.</summary>
#define ERROR_SPECIFIC    (-255)

#endif // #ifndef AZURE_SPHERE_COMMON_H_
