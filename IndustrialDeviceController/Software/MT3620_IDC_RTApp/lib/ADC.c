/* Copyright (c) Codethink Ltd. All rights reserved.
 *    Licensed under the MIT License. */

#include "ADC.h"
#include "Common.h"
#include "GPT.h"
#include "NVIC.h"
#include "mt3620/adc.h"
#include "mt3620/gpt.h"
#include "mt3620/dma.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//This enables the FIFO clear during initialisation
#define ADC_FIFO_CLEAR

//ADC interrupt priority
#define ADC_PRIORITY 2

//ADC frequency
#define ADC_CLK_FREQUENCY 2000000

struct AdcContext {
    bool init;
    uint32_t *rawData;
    ADC_Data *data;
    uint16_t fifoSize;
    uint8_t channelsCount;
    void (*callback)(int32_t);
};

static AdcContext context[MT3620_ADC_COUNT] = {0};

static inline uint32_t ADC_UnitToID(Platform_Unit unit) {
    if (unit == MT3620_UNIT_ADC0) {
        return 0;
    } else {
        return MT3620_ADC_COUNT + 1;
    }
}

static inline void ADC_DMADisable(){
    mt3620_dma_global->ch_en_set = (0 << MT3620_ADC_DMA_CHANNEL);
    MT3620_DMA_FIELD_WRITE(MT3620_ADC_DMA_CHANNEL, start, str, 0);
}

static uint8_t ADC_CountChannels(uint16_t channelMask)
{
    uint8_t numChannels;
    for (numChannels = 0; channelMask; numChannels++) {
        channelMask &= channelMask - 1;
    }

    return numChannels;
}

AdcContext *ADC_Open(Platform_Unit unit)
{
    uint32_t id = ADC_UnitToID (unit);
    if (id > MT3620_ADC_COUNT) {
        return NULL;
    }

    if (id >= MT3620_ADC_COUNT) {
        return NULL;
    }

    if (context[id].init) {
        return NULL;
    }

    context[id].init = true;
    context[id].callback = NULL;
    context[id].rawData = NULL;
    context[id].data = NULL;
    context[id].fifoSize = 0;
    context[id].channelsCount = 0;

    //Manually reset DMA and ADC
    mt3620_adc->adc_global_ctrl = 0;
    mt3620_adc->adc_global_ctrl = 1;
    mt3620_dma_global->ch_en_clr = (1U << MT3620_ADC_DMA_CHANNEL);

    /* Set NVIC priority and enable the interrupt for ADC and DMA*/
    NVIC_EnableIRQ(MT3620_ADC_INTERRUPT, ADC_PRIORITY);

    return &context[id];
}

void ADC_Close(AdcContext *handle)
{
    if (!handle->init) {
        return;
    }

    /* Turn off interrupt and reset trigger level to default */
    MT3620_ADC_FIELD_WRITE(adc_fifo_ier, rxfen, 0);
    mt3620_adc->adc_fifo_tri_lvl = ADC_FIFO_TRI_LVL_DEF;

    /* Turn off ADC finite state machine and periodic mode */
    mt3620_adc_ctl0_t ctl0 = { .mask = mt3620_adc->adc_ctl0 };
    ctl0.adc_fsm_en = 0;
    ctl0.pmode_en = 0;
    mt3620_adc->adc_ctl0 = ctl0.mask;

    /* Disable the interrupt for ADC and DMA*/
    NVIC_DisableIRQ(MT3620_ADC_INTERRUPT);

    handle->init = false;
    handle->rawData = NULL;
    handle->data = NULL;
    handle->fifoSize = 0;
    handle->channelsCount = 0;
    handle->callback = NULL;
}

static int32_t ADC_Read(
    AdcContext *handle, void (*callback)(int32_t status),
    uint32_t dmaFifoSize, ADC_Data *data,
    uint32_t *rawData, uint16_t channel,
    bool periodic, uint32_t frequency,
    uint16_t referenceVoltage)
{
    handle->callback = callback;
    handle->fifoSize = dmaFifoSize;
    handle->rawData = rawData;
    handle->data = data;

    mt3620_adc_ctl3_t ctl3 = { .mask = mt3620_adc->adc_ctl3 };
    ctl3.comp_time_delay = 1;
    ctl3.comp_preamp_current = 1;
    ctl3.comp_preamp_en = 1;
    ctl3.dither_en = 1;
    ctl3.dither_step_size = 2;
    ctl3.auxadc_in_mux_en = 1;
    ctl3.vcm_gen_en = 1;
    ctl3.auxadc_clk_gen_en = 1;
    ctl3.auxadc_pmu_clk_inv = 0;
    ctl3.auxadc_clk_src = 0;
    if (referenceVoltage > ADC_VREF_1V8_MIN && referenceVoltage < ADC_VREF_1V8_MAX) {
        ctl3.vcm_azure_en = 1;
    }
    else if (referenceVoltage > ADC_VREF_2V5_MIN && referenceVoltage < ADC_VREF_2V5_MAX) {
        ctl3.vcm_azure_en = 0;
    }
    else if (referenceVoltage > ADC_VREF_1V8_MAX && referenceVoltage < ADC_VREF_2V5_MIN) {
        ctl3.vcm_azure_en = ADC_VCM_AZURE_EN_DEF;
    } else {
        return ERROR_ADC_VREF_UNSUPPORTED;
    };
    mt3620_adc->adc_ctl3 = ctl3.mask;

    mt3620_adc_ctl0_t ctl0 = { .mask = mt3620_adc->adc_ctl0 };
    ctl0.adc_fsm_en = 0;
    ctl0.reg_avg_mode = 0;
    ctl0.reg_t_ch = 8;
    ctl0.pmode_en = 0;
    ctl0.reg_t_init = 20;
    ctl0.reg_ch_map = 0;
    mt3620_adc->adc_ctl0 = ctl0.mask;

#ifdef ADC_FIFO_CLEAR
    while (true) {
        mt3620_adc_fifo_debug16_t debug16 = (mt3620_adc_fifo_debug16_t)mt3620_adc->adc_fifo_debug16;
        if (debug16.read_ptr == debug16.write_ptr) {
            break;
        }
        (void)mt3620_adc->adc_fifo_rbr;
    }
#endif

    /* Wait for time specified in datasheet */
    GPT *timer = GPT_Open(MT3620_UNIT_GPT3, MT3620_GPT_3_LOW_SPEED, GPT_MODE_NONE);
    GPT_WaitTimer_Blocking(timer, 50, GPT_UNITS_MICROSEC);
    GPT_Close(timer);

    /* Set trigger level based on number of channels selected */
    uint8_t numChannels = ADC_CountChannels(channel);

    handle->channelsCount = numChannels;

    //Check fifo size is at least as great as the number of channels
    if (handle->fifoSize < numChannels) {
        return ERROR_ADC_FIFO_INVALID;
    }

    //Set DMA registers
    mt3620_dma_global->ch_en_set = (1 << MT3620_ADC_DMA_CHANNEL);

    MT3620_DMA_FIELD_WRITE(MT3620_ADC_DMA_CHANNEL, start, str, false);
    mt3620_dma[MT3620_ADC_DMA_CHANNEL].pgmaddr = handle->rawData;
    mt3620_dma[MT3620_ADC_DMA_CHANNEL].ffsize  = handle->fifoSize;
    if (handle->fifoSize == 1) {
        mt3620_dma[MT3620_ADC_DMA_CHANNEL].count = 1;
    } else {
        mt3620_dma[MT3620_ADC_DMA_CHANNEL].count   = ((3 * mt3620_dma[MT3620_ADC_DMA_CHANNEL].ffsize) / 4);
    }
    mt3620_dma[MT3620_ADC_DMA_CHANNEL].fixaddr = (void*)&mt3620_adc->adc_fifo_rbr;
    mt3620_dma[MT3620_ADC_DMA_CHANNEL].swptr   = 0;

    MT3620_DMA_FIELD_WRITE(MT3620_ADC_DMA_CHANNEL, ackint, ack, 1);

    MT3620_ADC_FIELD_WRITE(adc_fifo_dma_en, rx_dma_en, 1);

    mt3620_dma_con_t con = { .mask = mt3620_dma[MT3620_ADC_DMA_CHANNEL].con };
    con.size = 2;
    con.dir = 1;
    con.dreq = true;
    con.iten = true;
    con.toen = false;
    mt3620_dma[MT3620_ADC_DMA_CHANNEL].con = con.mask;

    MT3620_DMA_FIELD_WRITE(MT3620_ADC_DMA_CHANNEL, start, str, true);

    /* Select ADC channel and enable ADC finite state machine */
    ctl0.mask = mt3620_adc->adc_ctl0;
    if (periodic) {
        if (frequency > ADC_CLK_FREQUENCY) {
            return ERROR_ADC_FREQUENCY_UNSUPPORTED;
        }
        mt3620_adc->reg_period = (ADC_CLK_FREQUENCY / frequency) - 1;
        ctl0.pmode_en = periodic;
    }
    ctl0.reg_ch_map = channel;
    ctl0.adc_fsm_en = 1;
    mt3620_adc->adc_ctl0 = ctl0.mask;

    return ERROR_NONE;
}

int32_t ADC_ReadAsync(
    AdcContext *handle, void (*callback)(int32_t status),
    uint32_t dmaFifoSize, uint32_t *rawData,
    ADC_Data *data, uint16_t channel, uint16_t referenceVoltage)
{
    return ADC_Read(handle, callback, dmaFifoSize, data, rawData, channel,
            false, 0, referenceVoltage);
}

int32_t ADC_ReadPeriodicAsync(
    AdcContext *handle, void (*callback)(int32_t status),
    uint32_t dmaFifoSize, ADC_Data *data, uint32_t *rawData,
    uint16_t channel, uint32_t frequency, uint16_t referenceVoltage)
{
    return ADC_Read(handle, callback, dmaFifoSize, data, rawData, channel,
            true, frequency, referenceVoltage);
}

static volatile bool ADC_ReadSync_Ready = false;
static int32_t ADC_ReadSync_Status;

static void ADC_ReadSync_Callback(int32_t status)
{
    ADC_ReadSync_Ready = true;
    ADC_ReadSync_Status = status;
}

int32_t ADC_ReadSync(
    AdcContext *handle, uint32_t dmaFifoSize,
    ADC_Data *data, uint32_t *rawData,
    uint16_t channel, uint16_t referenceVoltage)
{
    if (ADC_CountChannels(channel) != dmaFifoSize) {
        return ERROR_ADC_FIFO_INVALID;
    }

    ADC_ReadSync_Ready = false;
    int32_t status = ADC_Read(handle, &ADC_ReadSync_Callback, dmaFifoSize, data, rawData,
            channel, false, 0, referenceVoltage);

    if (status != ERROR_NONE) {
        return status;
    }

    while (!ADC_ReadSync_Ready) {
        __asm__("wfi");
    }

    // Disable DMA when we're not using it.
    ADC_DMADisable();

    return ADC_ReadSync_Status;
}

void m4dma_irq_b_adc(void)
{
    AdcContext *handle = &context[0];

    volatile mt3620_dma_t * const dma = &mt3620_dma[MT3620_ADC_DMA_CHANNEL];

    unsigned count = dma->ffcnt;
    unsigned swptr = MT3620_DMA_FIELD_READ(MT3620_ADC_DMA_CHANNEL, swptr, swptr) >> 2;

    unsigned i;
    for (i = 0; i < count; i++) {
        unsigned j = (swptr + i) & (handle->fifoSize - 1);
        handle->data[i].value   = (handle->rawData[j] >> 4) & 0xFFF;
        handle->data[i].channel = handle->rawData[j] & 0xF;
    }

    // Increment swptr by count and toggle wrap bit if we wrapped
    swptr += count;
    bool wrap = (swptr >= handle->fifoSize);
    swptr &= (handle->fifoSize - 1);
    dma->swptr = ((dma->swptr ^ (wrap ? 0x00010000 : 0)) & 0xFFFF0000) | (swptr << 2);

    MT3620_DMA_FIELD_WRITE(MT3620_ADC_DMA_CHANNEL, ackint, ack, 1);

    //Pass the number of data copied back to the function caller
    handle->callback(count);
}
