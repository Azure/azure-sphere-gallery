/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include "I2S.h"
#include "NVIC.h"
#include "mt3620/i2s.h"
#include "mt3620/dma.h"
#include <stdbool.h>


#define I2S_BUFFER_SIZE 2048

// Each interface has two buffers per stream for double buffering.
static __attribute__((section(".sysram")))
    uint8_t I2S_Buffer[MT3620_I2S_COUNT][MT3620_I2S_STREAM_COUNT][I2S_BUFFER_SIZE] = { 0 };

typedef struct {
    bool      enable;
    unsigned  channels;
    bool    (*callback)(void *buffer, uintptr_t size);
} I2S_Settings;

struct I2S {
    bool         open;
    uint32_t     id;
    I2S_Settings out;
    I2S_Settings in;
};

static I2S context[MT3620_I2S_COUNT] = {0};

// TODO: Clean this up.
#define I2S_PRIORITY 2

static inline unsigned I2S_UnitToID(Platform_Unit unit)
{
    if ((unit < MT3620_UNIT_I2S0) || (unit > MT3620_UNIT_I2S1)) {
        return MT3620_I2S_COUNT;
    }
    return (unit - MT3620_UNIT_I2S0);
}

// There are some sample-rates implied by the datasheet but not officially supported
// Enable this macro to test them
#undef I2S_ALLOW_UNOFFICIAL_SAMPLE_RATE

// Sample rates will be allowed if they're within this threshold
#define I2S_SAMPLE_RATE_THRESH_PERCENT 5

static mt3620_i2s_sr_t I2S_SampleRate(unsigned target)
{
    unsigned low  = (target * (100 - I2S_SAMPLE_RATE_THRESH_PERCENT)) / 100;
    unsigned high = (target * (100 + I2S_SAMPLE_RATE_THRESH_PERCENT)) / 100;

    unsigned i;
    for (i = 0; i < MT3620_I2S_SR_COUNT; i++) {
#ifndef I2S_ALLOW_UNOFFICIAL_SAMPLE_RATE
        if (!MT3620_I2S_SR_IS_OFFICIAL(i)) {
            continue;
        }
#endif

        unsigned rate = MT3620_I2S_SR_CALC(i);
        if ((rate >= low) && (rate <= high)) {
            return i;
        }
    }

    return MT3620_I2S_SR_COUNT;
}

I2S *I2S_Open(Platform_Unit unit, unsigned mclk)
{
    unsigned id = I2S_UnitToID(unit);
    if (id >= MT3620_I2S_COUNT) {
        return NULL;
    }

    if (context[id].open) {
        return NULL;
    }

    // TODO: Check if non-PLL'd 26M is actually core clock.
    mt3620_i2s_clk_sel_e ext_mclk_sel;
    switch (mclk) {
    case        0:
    case 16000000:
        ext_mclk_sel = MT3620_I2S_CLK_SEL_XPLL_16M;
        break;
    case 26000000:
        ext_mclk_sel = MT3620_I2S_CLK_SEL_XPLL_26M;
        break;
    default:
        return NULL;
    }

    // Reset & Clear FIFOs
    MT3620_I2S_FIELD_WRITE(id, soft_reset, glb_soft_rst, true );
    MT3620_I2S_FIELD_WRITE(id, soft_reset, glb_soft_rst, false);

    mt3620_i2s_global_control_t global_control = { .mask = mt3620_i2s[id]->global_control };
    global_control.en             = false;
    global_control.dlfifo_en      = false;
    global_control.ulfifo_en      = false;
    global_control.engen_en       = true;
    global_control.ext_io_ck      = 1;
    global_control.ext            = true;
    global_control.ext_lrsw       = false;
    global_control.mclk_output_en = (mclk != 0);
    global_control.i2s_in_clk_en  = true;
    global_control.i2s_out_clk_en = true;
    global_control.x26m_sel       = true; // Select XPLL 26MHz as we assume it's more accurate.
    global_control.ext_bclk_inv   = true;
    global_control.neg_cap        = true;
    global_control.ext_mclk_sel   = ext_mclk_sel;
    global_control.loopback       = false;
    mt3620_i2s[id]->global_control = global_control.mask;

    context[id].id         = id;
    context[id].open       = true;
    context[id].out.enable = false;
    context[id].in.enable  = false;

    return &context[id];
}

void I2S_Close(I2S *handle)
{
    if (!handle || !handle->open) {
        return;
    }

    // Disable UL & DL.
    MT3620_I2S_FIELD_WRITE(handle->id, dl_control, en, false);
    MT3620_I2S_FIELD_WRITE(handle->id, ul_control, en, false);

    mt3620_i2s_global_control_t global_control
        = { .mask = mt3620_i2s[handle->id]->global_control };
    global_control.en             = false;
    global_control.dlfifo_en      = false;
    global_control.ulfifo_en      = false;
    global_control.engen_en       = false;
    global_control.mclk_output_en = false;
    global_control.i2s_in_clk_en  = false;
    global_control.i2s_out_clk_en = false;
    mt3620_i2s[handle->id]->global_control = global_control.mask;

    // Disable DMA
    mt3620_dma_global->ch_en_clr = (1U << MT3620_I2S_DMA_TX(handle->id));
    mt3620_dma_global->ch_en_clr = (1U << MT3620_I2S_DMA_RX(handle->id));
    mt3620_i2s[handle->id]->dma_if_control = 0;

    handle->open = false;
}


static void I2S_OutputUpdate(I2S *handle)
{
    if (!handle || !handle->open || !handle->out.enable) {
        return;
    }
    unsigned id = handle->id;

    volatile mt3620_dma_t * const dma = &mt3620_dma[MT3620_I2S_DMA_TX(id)];

    unsigned  size   = MT3620_DMA_FIELD_READ(MT3620_I2S_DMA_TX(id), con, size);
    uintptr_t remain = (dma->ffsize - dma->ffcnt) << size;
    uintptr_t swptr  = MT3620_DMA_FIELD_READ(MT3620_I2S_DMA_TX(id), swptr, swptr);

    // Handle buffer wrap-around.
    if ((swptr + remain) >= (dma->ffsize << size)) {
        uintptr_t partial = (dma->ffsize << size) - swptr;
        if (!handle->out.callback(&I2S_Buffer[handle->id][0][swptr], partial)) {
            return;
        }

        // Toggle the wrap bit and set swptr to zero.
        dma->swptr = (dma->swptr ^ 0x00010000) & 0xFFFF0000;
        swptr = 0;
        remain -= partial;
    }

    if ((remain > 0) && handle->out.callback(
        &I2S_Buffer[handle->id][0][swptr], remain)) {
        dma->swptr += remain;
    }

    MT3620_DMA_FIELD_WRITE(MT3620_I2S_DMA_TX(id), ackint, ack, 1);
}

int32_t I2S_Output(
    I2S *handle, I2S_Format format, unsigned channels, unsigned bits, unsigned rate,
    bool (*callback)(void *data, uintptr_t size))
{
    if (!handle) {
        return ERROR_PARAMETER;
    }
    if (!handle->open) {
        return ERROR_HANDLE_CLOSED;
    }
    unsigned id = handle->id;

    bool            enable = (format != I2S_FORMAT_NONE);
    bool            tdm    = false;
    mt3620_i2s_sr_t sr     = MT3620_I2S_SR_48K;
    if (enable) {
        if (channels == 0) {
            return ERROR_PARAMETER;
        }

        switch (format) {
        case I2S_FORMAT_I2S:
            if (channels > 2) {
                return ERROR_PARAMETER;
            }
            break;

        case I2S_FORMAT_TDM:
            if ((channels > 4) || (channels % 2)) {
                return ERROR_UNSUPPORTED;
            }
            tdm = true;
            break;

        default:
            return ERROR_PARAMETER;
        }

        if (bits != 16) {
            return ERROR_UNSUPPORTED;
        }

        sr = I2S_SampleRate(rate);
        if (sr >= MT3620_I2S_SR_COUNT) {
            return ERROR_UNSUPPORTED;
        }
    }

    mt3620_i2s_global_control_t global_control = { .mask = mt3620_i2s[id]->global_control };
    if (enable) {
        global_control.dl_mono           = (channels == 1);
        global_control.dl_mono_dup       = (channels == 1);
        global_control.i2s_out_clk_en    = true;
        global_control.dl_empty_value_en = true;
        global_control.clk_sel_out       = MT3620_I2S_CLK_SEL_EXTERNAL;
        mt3620_i2s[id]->global_control = global_control.mask;
    }

    mt3620_i2s_dl_control_t dl_control = { .mask = mt3620_i2s[id]->dl_control };
    dl_control.en = enable;
    if (enable) {
        dl_control.wlen        = MT3620_I2S_WLEN_16BIT;
        dl_control.src         = MT3620_I2S_SRC_SLAVE;
        dl_control.fmt         = (tdm ? MT3620_I2S_FMT_TDM : MT3620_I2S_FMT_I2S);
        dl_control.wsinv       = tdm;
        dl_control.dlfifo_2deq = (tdm && (channels == 4));
        dl_control.sr          = sr;
        dl_control.bit_per_s   = (tdm && (channels == 4) ? 1 : 0);
        dl_control.ws_rsync    = true; // Functional Spec recommends this be on.
        dl_control.msb_offset  = 0;
        dl_control.ch_per_s    = (tdm && (channels == 4) ? 1 : 0);
    }
    mt3620_i2s[id]->dl_control = dl_control.mask;

    if (enable) {
        // Enable DMA.
        mt3620_dma_global->ch_en_set = (1U << MT3620_I2S_DMA_TX(id));
        MT3620_DMA_FIELD_WRITE(MT3620_I2S_DMA_TX(id), start, str, false);
        mt3620_dma_con_t dma_con_tx = { .mask = mt3620_dma[MT3620_I2S_DMA_TX(id)].con };
        dma_con_tx.dir   = 0;
        dma_con_tx.iten  = true;
        dma_con_tx.dreq  = true;
        dma_con_tx.size  = (channels == 1 ? 1 : 2);
        mt3620_dma[MT3620_I2S_DMA_TX(id)].con     = dma_con_tx.mask;
        mt3620_dma[MT3620_I2S_DMA_TX(id)].fixaddr = (void *)mt3620_i2s_fifo[id];
        mt3620_dma[MT3620_I2S_DMA_TX(id)].pgmaddr = (void *)I2S_Buffer[id][0];
        mt3620_dma[MT3620_I2S_DMA_TX(id)].swptr   = 0;
        mt3620_dma[MT3620_I2S_DMA_TX(id)].ffsize  = (I2S_BUFFER_SIZE >> dma_con_tx.size);
        mt3620_dma[MT3620_I2S_DMA_TX(id)].count   = (mt3620_dma[MT3620_I2S_DMA_TX(id)].ffsize / 4);
    } else {
        MT3620_DMA_FIELD_WRITE(MT3620_I2S_DMA_TX(id), start, str, false);
        mt3620_dma_global->ch_en_clr = (1U << MT3620_I2S_DMA_TX(id));
    }

    // Enable DMA handshakes for master mode.
    mt3620_i2s_dma_if_control_t dma_if_control = { .mask = mt3620_i2s[id]->dma_if_control };
    dma_if_control.dl_dmareq_mi_num = (enable ? 1 : 0);
    dma_if_control.dl_ahb_early_en  = enable;
    dma_if_control.dl_dma_mode_sel  = enable;
    mt3620_i2s[id]->dma_if_control = dma_if_control.mask;

    // Enable I2S.
    global_control.en = (enable || handle->in.enable);
    global_control.dlfifo_en = enable;
    mt3620_i2s[id]->global_control = global_control.mask;

    handle->out.enable   = enable;
    handle->out.channels = channels;
    handle->out.callback = callback;

    if (enable) {
        MT3620_DMA_FIELD_WRITE(MT3620_I2S_DMA_TX(id), start, str, true);
    }

    return ERROR_NONE;
}

static void I2S_InputUpdate(I2S *handle)
{
    if (!handle || !handle->open || !handle->in.enable) {
        return;
    }
    unsigned id = handle->id;

    volatile mt3620_dma_t * const dma = &mt3620_dma[MT3620_I2S_DMA_RX(id)];

    unsigned  size   = MT3620_DMA_FIELD_READ(MT3620_I2S_DMA_RX(id), con, size);
    uintptr_t remain = (dma->ffsize - dma->ffcnt) << size;
    uintptr_t swptr  = MT3620_DMA_FIELD_READ(MT3620_I2S_DMA_RX(id), swptr, swptr);

    // Handle buffer wrap-around.
    if ((swptr + remain) >= (dma->ffsize << size)) {
        uintptr_t partial = (dma->ffsize << size) - swptr;
        if (!handle->in.callback(&I2S_Buffer[handle->id][1][swptr], partial)) {
            return;
        }

        // Toggle the wrap bit and set swptr to zero.
        dma->swptr = (dma->swptr ^ 0x00010000) & 0xFFFF0000;
        swptr = 0;
        remain -= partial;
    }

    if ((remain > 0) && handle->in.callback(
        &I2S_Buffer[handle->id][1][swptr], remain)) {
        dma->swptr += remain;
    }

    MT3620_DMA_FIELD_WRITE(MT3620_I2S_DMA_RX(id), ackint, ack, 1);
}


int32_t I2S_Input(
    I2S *handle, I2S_Format format, unsigned channels, unsigned bits, unsigned rate,
    bool (*callback)(void *data, uintptr_t size))
{
    if (!handle) {
        return ERROR_PARAMETER;
    }
    if (!handle->open) {
        return ERROR_HANDLE_CLOSED;
    }
    unsigned id = handle->id;

    bool            enable = (format != I2S_FORMAT_NONE);
    bool            tdm    = false;
    mt3620_i2s_sr_t sr     = MT3620_I2S_SR_48K;
    if (enable) {
        if (channels == 0) {
            return ERROR_PARAMETER;
        }

        switch (format) {
        case I2S_FORMAT_I2S:
            if (channels > 2) {
                return ERROR_PARAMETER;
            }
            break;

        case I2S_FORMAT_TDM:
            if ((channels > 4) || (channels % 2)) {
                return ERROR_UNSUPPORTED;
            }
            tdm = true;
            break;

        default:
            return ERROR_PARAMETER;
        }

        if (bits != 16) {
            return ERROR_UNSUPPORTED;
        }

        sr = I2S_SampleRate(rate);
        if (sr >= MT3620_I2S_SR_COUNT) {
            return ERROR_UNSUPPORTED;
        }
    }

    mt3620_i2s_global_control_t global_control = { .mask = mt3620_i2s[id]->global_control };
    if (enable) {
        global_control.ul_empty_value_en = false;
        global_control.clk_sel_in        = MT3620_I2S_CLK_SEL_EXTERNAL;
        mt3620_i2s[id]->global_control = global_control.mask;
    }

    mt3620_i2s_ul_control_t ul_control = { .mask = mt3620_i2s[id]->ul_control };
    ul_control.en = enable;
    if (enable) {
        ul_control.wlen        = MT3620_I2S_WLEN_16BIT;
        ul_control.src         = MT3620_I2S_SRC_SLAVE;
        ul_control.fmt         = (tdm ? MT3620_I2S_FMT_TDM : MT3620_I2S_FMT_I2S);
        ul_control.wsinv       = tdm;
        ul_control.sr          = sr;
        ul_control.bit_per_s   = (tdm && (channels == 4) ? 1 : 0);
        ul_control.ws_rsync    = true;  // Functional Spec recommends this be on.
        ul_control.down_rate   = false; // TODO: Figure out when this should be used.
        ul_control.msb_offset  = 0;
        ul_control.update_word = 8;
        ul_control.ch_per_s    = (tdm && (channels == 4) ? 1 : 0);
        ul_control.lr_swap     = false;
    }
    mt3620_i2s[id]->ul_control = ul_control.mask;

    if (enable) {
        // Enable DMA.
        mt3620_dma_global->ch_en_set = (1U << MT3620_I2S_DMA_RX(id));
        MT3620_DMA_FIELD_WRITE(MT3620_I2S_DMA_RX(id), start, str, false);
        mt3620_dma_con_t dma_con_rx = { .mask = mt3620_dma[MT3620_I2S_DMA_RX(id)].con };
        dma_con_rx.dir   = 1;
        dma_con_rx.iten  = true;
        dma_con_rx.dreq  = true;
        dma_con_rx.size  = (channels == 1 ? 1 : 2);
        mt3620_dma[MT3620_I2S_DMA_RX(id)].con = dma_con_rx.mask;
        mt3620_dma[MT3620_I2S_DMA_RX(id)].fixaddr = (void *)mt3620_i2s_fifo[id];
        mt3620_dma[MT3620_I2S_DMA_RX(id)].pgmaddr = (void *)I2S_Buffer[id][1];
        mt3620_dma[MT3620_I2S_DMA_RX(id)].swptr   = 0;
        mt3620_dma[MT3620_I2S_DMA_RX(id)].ffsize  = (I2S_BUFFER_SIZE >> dma_con_rx.size);
        mt3620_dma[MT3620_I2S_DMA_RX(id)].count   = ((mt3620_dma[MT3620_I2S_DMA_RX(id)].ffsize * 3) / 4);
    } else {
        MT3620_DMA_FIELD_WRITE(MT3620_I2S_DMA_RX(id), start, str, false);
        mt3620_dma_global->ch_en_clr = (1U << MT3620_I2S_DMA_RX(id));
    }

    // Enable DMA handshakes for master mode.
    mt3620_i2s_dma_if_control_t dma_if_control = { .mask = mt3620_i2s[id]->dma_if_control };
    dma_if_control.ul_dmareq_mi_num = (enable ? 1 : 0);
    dma_if_control.ul_ahb_early_en  = enable;
    dma_if_control.ul_dma_mode_sel  = enable;
    mt3620_i2s[id]->dma_if_control = dma_if_control.mask;

    // Enable I2S.
    global_control.en = (enable || handle->out.enable);
    global_control.ulfifo_en = enable;
    mt3620_i2s[id]->global_control = global_control.mask;

    handle->in.enable   = enable;
    handle->in.callback = callback;

    if (enable) {
        MT3620_DMA_FIELD_WRITE(MT3620_I2S_DMA_RX(id), start, str, true);
    }

    return ERROR_NONE;
}


unsigned I2S_GetOutputSampleRate(I2S *handle)
{
    if (!handle || !handle->open || !handle->out.enable) {
        return 0;
    }

    return MT3620_I2S_SR_CALC(MT3620_I2S_FIELD_READ(handle->id, dl_control, sr));
}

unsigned I2S_GetInputSampleRate(I2S *handle)
{
    if (!handle || !handle->open || !handle->in.enable) {
        return 0;
    }

    return MT3620_I2S_SR_CALC(MT3620_I2S_FIELD_READ(handle->id, ul_control, sr));
}

void m4dma_irq_b_i2s0_tx(void)
{
    I2S_OutputUpdate(&context[0]);
}

void m4dma_irq_b_i2s0_rx(void)
{
    I2S_InputUpdate(&context[0]);
}

void m4dma_irq_b_i2s1_tx(void)
{
    I2S_OutputUpdate(&context[1]);
}

void m4dma_irq_b_i2s1_rx(void)
{
    I2S_InputUpdate(&context[1]);
}
