/* Copyright (c) Codethink Ltd. All rights reserved.
 *    Licensed under the MIT License. */

#ifndef AZURE_SPHERE_ADC_H_
#define AZURE_SPHERE_ADC_H_

#include <stdbool.h>
#include <stdint.h>

#include "Common.h"
#include "Platform.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef struct AdcContext AdcContext;

/// <summary>Returned when the user supplied FIFO is invalid for the number of channels.</summary>
#define ERROR_ADC_FIFO_INVALID                 (ERROR_SPECIFIC - 1)

/// <summary>Returned when a frequency for the ADC period is unsupported.</summary>
#define ERROR_ADC_FREQUENCY_UNSUPPORTED        (ERROR_SPECIFIC - 2)

/// <summary>Returned when a voltage reference for the V_ref setting is unsupported.</summary>
#define ERROR_ADC_VREF_UNSUPPORTED             (ERROR_SPECIFIC - 3)

/// <summary>A data structure for separating ADC data from the channel number.</summary>
typedef struct {
    /// <summary>
    /// <para>12 bits of ADC register; with uint12_t range corresponding to 0->V_ref.</para>
    /// </summary>
    uint32_t value;
    /// <summary>
    /// <para>Channel of data (4 accessible channels in total).</para>
    /// </summary>
    uint32_t channel;
} ADC_Data;

/// <summary>
/// <para>The application has to call this function to register a callback and a buffer.
/// The callback will be called in the interrupt and give the user a status return. The
/// data buffer needs to be sized to the number of channels being used.</para>
/// </summary>
/// <param name="unit">Which ADC block to initialise and acquire a handle for. This is defined
/// in the Platform.h file.</param>
/// <returns>A handle to an ADC block or NULL on failure.</returns>
AdcContext *ADC_Open(Platform_Unit unit);


/// <summary>
/// <para>The application has to call this function to release a handle once finished.
/// The application is then free to open it again.</para>
/// </summary>
/// <param name="handle">The ADC handle which is to be released.</param>
void ADC_Close(AdcContext *handle);

/// <summary>
/// <para>Configures the appropriate ADC block to fill the data buffer
/// and trigger interrupt when ADC data is ready.</para>
/// </summary>
/// <param name="handle">The ADC block to enable.</param>
/// <param name="callback">A pointer to a function that will be called during an interrupt.
/// The status parameter will be set equal to the number of values copied in to the data buffer.
/// </param>
/// <param name="dmaFifoSize">How many entries the DMA buffer can hold.</param>
/// <param name="data">A pointer to a data structure used to store formatted ADC data.</param>
/// <param name="rawData">A pointer to a data structure to contain the unformatted data.</param>
/// <param name="channel"> Which ADC channels to use, this is a bit mask, so 0111 would
/// enable ADC channels 0, 1 and 2.</param>
/// <param name="referenceVoltage">The reference voltage being used, either between 1.62V and 1.92V
/// or between 2.25V and 2.75V. Defaults to setting for 1.8V between 1.92V and 2.25V. This parameter
/// is scaled in millivolts, so for 1.8V pass 1800 and for 2.5V pass 2500.</param>
int32_t ADC_ReadAsync(
    AdcContext *handle, void (*callback)(int32_t status),
    uint32_t dmaFifoSize, uint32_t *rawData,
    ADC_Data *data, uint16_t channel,
    uint16_t referenceVoltage);

/// <summary>
/// <para>Configures the appropriate ADC block to periodically fill the data buffer
/// and trigger interrupt when ADC data is ready. </para>
/// </summary>
/// <param name="handle">The ADC block to enable.</param>
/// <param name="callback">A pointer to a function that will be called during an interrupt.
/// The status parameter will be set equal to the number of values copied in to the data buffer.
/// </param>
/// <param name="dmaFifoSize">How many entries the DMA buffer can hold.</param>
/// <param name="data">A pointer to a data structure used to store ADC data.</param>
/// <param name="rawData">A pointer to a data structure to contain the unformatted data.</param>
/// <param name="channel"> Which ADC channels to use, this is a bit mask, so 0111 would
/// enable ADC channels 0, 1 and 2.</param>
/// <param name="frequency">Sets the the frequency at which the ADC block is run.</param>
/// <param name="referenceVoltage">The reference voltage being used, either between 1.62V and 1.92V
/// or between 2.25V and 2.75V. Defaults to setting for 1.8V between 1.92V and 2.25V. This parameter
/// is scaled in millivolts, so for 1.8V pass 1800 and for 2.5V pass 2500.</param>
int32_t ADC_ReadPeriodicAsync(
    AdcContext *handle, void (*callback)(int32_t status),
    uint32_t dmaFifoSize, ADC_Data *data, uint32_t *rawData,
    uint16_t channel, uint32_t frequency,
    uint16_t referenceVoltage);

/// <summary>
/// <para>Configures the appropriate ADC block and returns the requested ADC data synchronously.
/// </para>
/// </summary>
/// <param name="handle">The ADC block to enable.</param>
/// <param name="dmaFifoSize">How many entries the DMA buffer can hold.</param>
/// <param name="data">A pointer to a data structure used to store ADC data.</param>
/// <param name="rawData">A pointer to a data structure to contain the unformatted data.</param>
/// <param name="channel"> Which ADC channels to use, this is a bit mask, so 0111 would
/// enable ADC channels 0, 1 and 2.</param>
/// <param name="referenceVoltage">The reference voltage being used, either between 1.62V and 1.92V
/// or between 2.25V and 2.75V. Defaults to setting for 1.8V between 1.92V and 2.25V. This parameter
/// is scaled in millivolts, so for 1.8V pass 1800 and for 2.5V pass 2500.</param>
int32_t ADC_ReadSync(
    AdcContext *handle, uint32_t dmaFifoSize,
    ADC_Data *data, uint32_t *rawData,
    uint16_t channel, uint16_t referenceVoltage);

#ifdef __cplusplus
 }
#endif

#endif
