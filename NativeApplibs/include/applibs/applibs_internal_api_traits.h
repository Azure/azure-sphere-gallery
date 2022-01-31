#pragma once

/// <summary>
/// Macro to tag a function or variable as deprecated and refer to its replacement.
/// A deprecated function or variable is still available but it is no longer recommended and may be
/// made obsolete in a future release.
/// </summary>
#ifndef _AZSPHERE_DEPRECATED_BY
#ifdef AZURE_SPHERE_PUBLIC_SDK
#define _AZSPHERE_DEPRECATED_BY(_Replacement) \
    __attribute__((deprecated(                \
        "This function or variable is deprecated. Please use '" #_Replacement "' instead.")))
#else
#define _AZSPHERE_DEPRECATED_BY(_Replacement)
#endif
#endif

/// <summary>
/// Macro to tag a function or variable as obsolete and refer to its replacement.
/// An obsolete function or variable cannot be used because it is no longer supported or implemented
/// in the current release.
/// </summary>
#ifndef _AZSPHERE_OBSOLETED_BY
#ifdef AZURE_SPHERE_PUBLIC_SDK
#define _AZSPHERE_OBSOLETED_BY(_Replacement) \
    __attribute__(                           \
        (error("This function or variable is obsolete. Please use '" #_Replacement "' instead.")))
#else
#define _AZSPHERE_OBSOLETED_BY(_Replacement)
#endif
#endif
