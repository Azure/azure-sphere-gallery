/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef AZURE_SPHERE_CPUFREQ_H_
#define AZURE_SPHERE_CPUFREQ_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdbool.h>

bool     CPUFreq_Set(unsigned freq);
unsigned CPUFreq_Get(void);

#ifdef __cplusplus
 }
#endif

#endif // #ifndef AZURE_SPHERE_CPUFREQ_H_
