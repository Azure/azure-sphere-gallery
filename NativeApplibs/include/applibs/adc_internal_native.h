#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int ADC_Open(ADC_ControllerId id)
{
    fprintf(stderr, "ADC_Open called, id: %d\n", id);
    return 0;
}

static inline int ADC_Poll(int fd, ADC_ChannelId channel_id, uint32_t *outSampleValue)
{
    fprintf(stderr, "ADC_Poll called, fd: %d, channel_id: %d, setting outSampleValue to 0\n",
        fd, channel_id);
    *outSampleValue = 0;
    return 0;
}

static inline int ADC_GetSampleBitCount(int fd, ADC_ChannelId channel_id)
{
    fprintf(stderr, "ADC_GetSampleBitCount called, fd: %d, channel_id: %d\n", fd, channel_id);
    return 0;
}

static inline int ADC_SetReferenceVoltage(int fd, ADC_ChannelId channel_id, float referenceVoltage)
{
    fprintf(stderr, "ADC_SetReferenceVoltage called, fd: %d, channel_id: %d, reference_voltage: %f\n",
        fd, channel_id, referenceVoltage);
    return 0;
}

#ifdef __cplusplus
}
#endif