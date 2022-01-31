/// \file applications_internal.h
/// \brief This header contains internal functions of the Applications API and should not be
/// included directly; include applibs/applications.h instead.
#pragma once

#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t z__Application_MemoryAttribute;
enum {
    z__Applications_MemoryAttribute_TotalMemoryUsageInKB = 0,
    z__Applications_MemoryAttribute_UserModeMemoryUsageInKB = 1,
    z__Applications_MemoryAttribute_PeakUserModeMemoryUsageInKB = 2,
};

int z__Applications_GetMemoryInfo(z__Application_MemoryAttribute memoryAttribute, void *buffer,
                                  size_t capacity);

static inline size_t Applications_GetTotalMemoryUsageInKB(void)
{
    size_t ret = 0;
    z__Applications_GetMemoryInfo(z__Applications_MemoryAttribute_TotalMemoryUsageInKB, &ret,
                                  sizeof(size_t));
    return ret;
}

static inline size_t Applications_GetUserModeMemoryUsageInKB(void)
{
    size_t ret = 0;
    z__Applications_GetMemoryInfo(z__Applications_MemoryAttribute_UserModeMemoryUsageInKB, &ret,
                                  sizeof(size_t));
    return ret;
}

static inline size_t Applications_GetPeakUserModeMemoryUsageInKB(void)
{
    size_t ret = 0;
    z__Applications_GetMemoryInfo(z__Applications_MemoryAttribute_PeakUserModeMemoryUsageInKB, &ret,
                                  sizeof(size_t));
    return ret;
}

#ifdef __cplusplus
}
#endif
