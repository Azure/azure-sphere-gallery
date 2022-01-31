#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int PWM_Open(PWM_ControllerId pwm)
{
    fprintf(stderr, "PWM_Open called, pwm: %u\n", pwm);
    return 0;
}

static inline int PWM_Apply(int pwmFd, PWM_ChannelId pwmChannel, const PwmState *newState)
{
    fprintf(stderr, "PWM_Apply called, pwmFd: %d, pwmChannel: %u, newState: %p\n",
        pwmFd, pwmChannel, newState);
    return 0;
}

#ifdef __cplusplus
}
#endif