/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "logs.h"
#include "applibs_versions.h"
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void LogErrno(const char *message, ...)
{
    int currentErrno = errno;
    va_list argptr;
    va_start(argptr, message);
    Log_DebugVarArgs(message, argptr);
    va_end(argptr);
    Log_Debug(" (errno=%d, '%s').\n", currentErrno, strerror(currentErrno));
}

void CloseFdAndLogOnError(int fd, const char *message)
{
    if(fd >= 0)
    {
        int result = close(fd);
        if(result != 0)
        {
            LogErrno("WARNING: Could not close fd (%s)", message);
        }
    }
}