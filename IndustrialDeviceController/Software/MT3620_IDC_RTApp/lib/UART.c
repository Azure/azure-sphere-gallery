/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include "UART.h"
#include "mt3620/uart.h"
#include "mt3620/dma.h"
#include "NVIC.h"
#include <stddef.h>
#include <stdbool.h>


// Disable DMA as it doesn't currently work
#undef UART_ALLOW_DMA


// Configure these variables as needed.
#define TX_BUFFER_SIZE 256
#define RX_BUFFER_SIZE 32

#if (TX_BUFFER_SIZE > 65536)
#error "TX buffer size must be less than or equal 65536"
#endif

#if (RX_BUFFER_SIZE > 65536)
#error "RX buffer size must be less than or equal 65536"
#endif

#if (TX_BUFFER_SIZE & (TX_BUFFER_SIZE - 1)) != 0
#error "TX buffer size must be a power of two"
#endif

#if (RX_BUFFER_SIZE & (RX_BUFFER_SIZE - 1)) != 0
#error "RX buffer size must be a power of two"
#endif


// TODO: Reduce sysram usage by providing a more limited set of buffers?
static __attribute__((section(".sysram"))) uint8_t UART_BuffRX[MT3620_UART_COUNT][RX_BUFFER_SIZE] = { 0 };
static __attribute__((section(".sysram"))) uint8_t UART_BuffTX[MT3620_UART_COUNT][TX_BUFFER_SIZE] = { 0 };

struct UART {
    bool     open;
    unsigned id;
    bool     dma;

    uint32_t txRemain, txRead, txWrite;
    uint32_t rxRemain, rxRead, rxWrite;

    void (*rxCallback)(void);
};

static UART context[MT3620_UART_COUNT] = {0};

static inline unsigned UART_UnitToID(Platform_Unit unit)
{
    if ((unit < MT3620_UNIT_UART_DEBUG) || (unit > MT3620_UNIT_ISU5)) {
        return MT3620_UART_COUNT;
    }
    return (unit - MT3620_UNIT_UART_DEBUG);
}

UART *UART_Open(Platform_Unit unit, unsigned baud, UART_Parity parity, unsigned stopBits, void (*rxCallback)(void))
{
    unsigned id = UART_UnitToID(unit);
    if (id >= MT3620_UART_COUNT) {
        return NULL;
    }

    if (context[id].open) {
        return NULL;
    }

    if (baud > MT3620_UART_MAX_SPEED) {
        return NULL;
    }

    if (parity > UART_PARITY_STICK_ONE) {
        return NULL;
    }

    if ((stopBits < 1) || (stopBits > 2)) {
        return NULL;
    }

    unsigned divs  = ((MT3620_UART_CLOCK * 10) + (baud / 2)) / baud;

    unsigned dl = (divs + 2559) / 2560;
    divs /= dl;

    unsigned fract = mt3620_uart_fract_lut[divs % 10];
    unsigned count = (divs / 10);

    // LCR (enable DLL, DLM)
    mt3620_uart_lcr_t lcr = { .mask = mt3620_uart[id]->lcr };
    lcr.wls  = 3;
    lcr.stb  = true;
    lcr.pen  = true;
    lcr.eps  = true;
    lcr.sp   = true;
    lcr.sb   = false;
    lcr.dlab = true;
    mt3620_uart[id]->lcr = lcr.mask;

    // EFR (enable enhancement features)
    mt3620_uart_efr_t efr = { .mask = mt3620_uart[id]->efr };
    efr.sw_flow_cont = 0;
    efr.enable_e     = true;
    efr.auto_rts     = 0;
    efr.auto_cts     = 0;
    mt3620_uart[id]->efr = efr.mask;

    MT3620_UART_FIELD_WRITE(id, highspeed, speed, 3);
    MT3620_UART_FIELD_WRITE(id, dlm, dlm, (dl >> 8)); // Divisor Latch (MS)
    MT3620_UART_FIELD_WRITE(id, dll, dll, (dl & 0xFF)); // Divisor Latch (LS)
    MT3620_UART_FIELD_WRITE(id, sample_count, sample_count, (count - 1));
    MT3620_UART_FIELD_WRITE(id, sample_point, sample_point, ((count / 2) - 2));
    MT3620_UART_FIELD_WRITE(id, fracdiv_m, fracdiv_m, (fract >> 8));
    MT3620_UART_FIELD_WRITE(id, fracdiv_l, fracdiv_l, (fract & 0xFF));

    // LCR (8-bit word length)
    lcr.wls  = 3;
    lcr.stb  = (stopBits > 1);
    lcr.pen  = (parity != UART_PARITY_NONE);
    lcr.eps  = (parity & 1);
    lcr.sp   = (parity >= UART_PARITY_STICK_ONE);
    lcr.sb   = false;
    lcr.dlab = false;
    mt3620_uart[id]->lcr = lcr.mask;

    // FCR is write-only so we don't read an initial value.
    mt3620_uart_fcr_t fcr = { .mask = 0x00000000 };
    fcr.rftl  = 2;    // 12 element RX FIFO trigger
    fcr.tftl  = 1;    // 4 element TX FIFO trigger
    fcr.clrt  = true; // Clear Transmit FIFO
    fcr.clrr  = true; // Clear Receive FIFO
    fcr.fifoe = true; // FIFO Enable
    mt3620_uart[id]->fcr = fcr.mask;

    bool dma = false;
#ifdef UART_ALLOW_DMA
    // Debug UART doesn't seem to have DMA support.
    if (unit != MT3620_UNIT_UART_DEBUG) {
        dma = true;
    }
#endif

    MT3620_UART_FIELD_WRITE(id, vfifo_en, vfifo_en, dma);

    if (dma) {
        mt3620_dma_global->ch_en_set = (1U << MT3620_UART_DMA_TX(id));
        MT3620_DMA_FIELD_WRITE(MT3620_UART_DMA_TX(id), start, str, false);

        volatile mt3620_dma_t * const tx_dma = &mt3620_dma[MT3620_UART_DMA_TX(id)];
        tx_dma->fixaddr = (void *)&mt3620_uart[id]->thr;
        tx_dma->pgmaddr = (void *)UART_BuffTX[id];
        tx_dma->ffsize  = TX_BUFFER_SIZE;
        tx_dma->count   = 0;

        mt3620_dma_con_t dma_con_tx = { .mask = tx_dma->con };
        dma_con_tx.dir  = 0;
        dma_con_tx.iten = false;
        dma_con_tx.toen = false;
        dma_con_tx.dreq = false;//true;
        dma_con_tx.size = 0;
        tx_dma->con = dma_con_tx.mask;

        tx_dma->swptr = tx_dma->hwptr;

        mt3620_dma_global->ch_en_set = (1U << MT3620_UART_DMA_RX(id));
        MT3620_DMA_FIELD_WRITE(MT3620_UART_DMA_RX(id), start, str, false);

        volatile mt3620_dma_t * const rx_dma = &mt3620_dma[MT3620_UART_DMA_RX(id)];
        rx_dma->fixaddr = (void *)&mt3620_uart[id]->thr;
        rx_dma->pgmaddr = (void *)UART_BuffRX[id];
        rx_dma->ffsize  = RX_BUFFER_SIZE;
        rx_dma->count = 0;

        mt3620_dma_con_t dma_con_rx = { .mask = rx_dma->con };
        dma_con_rx.dir  = 1;
        dma_con_rx.iten = false;
        dma_con_rx.toen = false;
        dma_con_rx.dreq = true;
        dma_con_rx.size = 0;
        rx_dma->con = dma_con_rx.mask;

        rx_dma->swptr = rx_dma->hwptr;

        mt3620_uart_extend_add_t extend_add = { .mask = mt3620_uart[id]->extend_add };
        extend_add.rx_dma_hsk_en = true;
        extend_add.tx_dma_hsk_en = true;
        extend_add.tx_auto_trans = true;
        mt3620_uart[id]->extend_add = extend_add.mask;
    }

    // If an RX callback was supplied then enable the Receive Buffer Full Interrupt.
    if (rxCallback) {
        // Enable Receiver Buffer Full Interrupt
        MT3620_UART_FIELD_WRITE(id, ier, erbfi, true);
    }

    NVIC_EnableIRQ(MT3620_UART_INTERRUPT(id), UART_PRIORITY);

    if (dma) {
        // Only start RX DMA as TX DMA will be unused until first transmissiom.
        MT3620_DMA_FIELD_WRITE(MT3620_UART_DMA_RX(id), start, str, true);
    }

    context[id].id   = id;
    context[id].open = true;
    context[id].dma  = dma;

    context[id].txRemain = TX_BUFFER_SIZE;
    context[id].txRead   = 0;
    context[id].txWrite  = 0;

    context[id].rxRemain = RX_BUFFER_SIZE;
    context[id].rxRead   = 0;
    context[id].rxWrite  = 0;

    context[id].rxCallback = rxCallback;

    return &context[id];
}

void UART_Close(UART *handle)
{
    if (!handle || !handle->open) {
        return;
    }

    MT3620_DMA_FIELD_WRITE(MT3620_UART_DMA_TX(handle->id), start, str, false);
    MT3620_DMA_FIELD_WRITE(MT3620_UART_DMA_RX(handle->id), start, str, false);

    mt3620_dma_global->ch_en_clr = (1U << MT3620_UART_DMA_TX(handle->id));
    mt3620_dma_global->ch_en_clr = (1U << MT3620_UART_DMA_RX(handle->id));

    mt3620_uart[handle->id]->ier = 0x00000000;
    NVIC_DisableIRQ(MT3620_UART_INTERRUPT(handle->id));

    handle->open = false;
}

int32_t UART_Write(UART *handle, const void *data, uintptr_t size)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    if (!handle->open) {
        return ERROR_HANDLE_CLOSED;
    }

    if (!data || (size == 0)) {
        return ERROR_PARAMETER;
    }

    if (handle->dma) {
        volatile mt3620_dma_t * const tx_dma = &mt3620_dma[MT3620_UART_DMA_TX(handle->id)];
        while (size > 0) {
            // We can't send any bytes until TX fifo is not full.
            while (MT3620_DMA_FIELD_READ(MT3620_UART_DMA_TX(handle->id), ffsta, full));

            MT3620_DMA_FIELD_WRITE(MT3620_UART_DMA_TX(handle->id), start, str, false);

            uintptr_t remain = (tx_dma->ffsize - tx_dma->ffcnt);
            uintptr_t chunk = (remain >= size ? size : remain);

            uintptr_t i;
            for (i = 0; i < chunk; i++) {
                UART_BuffTX[handle->id][tx_dma->swptr++] = ((const uint8_t *)data)[i];
#if (TX_BUFFER_SIZE < 65536)
                // When the buffer isn't exactly 16-bits we need to handle the wrap bit.
                if ((tx_dma->swptr & 0xFFFF) > TX_BUFFER_SIZE) {
                    tx_dma->swptr &= 0xFFFF0000;
                    tx_dma->swptr ^= 0x00010000;
                }
#endif
            }

            MT3620_DMA_FIELD_WRITE(MT3620_UART_DMA_TX(handle->id), start, str, true);

            data = (const void *)((uintptr_t)data + chunk);
            size -= chunk;
        }
    } else {
        // If nothing is queued in hardware, queue that first.
        if ((handle->txRemain == TX_BUFFER_SIZE)
            && MT3620_UART_FIELD_READ(handle->id, lsr, thre)) {
            uint32_t offset = MT3620_UART_FIELD_READ(handle->id, tx_offset, tx_offset);
            uint32_t remain = MT3620_UART_TX_FIFO_DEPTH - offset;

            uint32_t chunk = (remain > size ? size : remain);

            uint32_t i;
            for (i = 0; i < chunk; i++) {
                mt3620_uart[handle->id]->thr = ((const uint8_t *)data)[i];
            }

            data = (const void *)((uintptr_t)data + chunk);
            size -= chunk;
        }

        // Queue remaining bytes in buffer to be handled by interrupt.
        while (size > 0) {
            while (handle->txRemain == 0) {
                __asm__("wfi");
            }

            uint32_t chunk = (handle->txRemain >= size ? size : handle->txRemain);

            // We can't use memcpy here because the buffer wraps.
            uint32_t i;
            for (i = 0; i < chunk; i++) {
                UART_BuffTX[handle->id][handle->txWrite++] = ((const uint8_t *)data)[i];
                handle->txWrite %= TX_BUFFER_SIZE;
            }
            handle->txRemain -= chunk;

            // Enable interrupt so queued data is processed.
            MT3620_UART_FIELD_WRITE(handle->id, ier, etbei, true);

            data = (const void *)((uintptr_t)data + chunk);
            size -= chunk;
        }
    }

    return ERROR_NONE;
}

inline bool UART_IsWriteComplete(UART *handle)
{
    return MT3620_UART_FIELD_READ(handle->id, lsr, temt);
}

int32_t UART_Read(UART *handle, void *data, uintptr_t size)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    if (!handle->open) {
        return ERROR_HANDLE_CLOSED;
    }

    if (!data || (size == 0)) {
        return ERROR_PARAMETER;
    }

    if (handle->dma) {
        volatile mt3620_dma_t * const rx_dma = &mt3620_dma[MT3620_UART_DMA_RX(handle->id)];
        while (size > 0) {
            // We can't receive any bytes while RX fifo is empty.
            while (MT3620_DMA_FIELD_READ(MT3620_UART_DMA_RX(handle->id), ffsta, empty));

            MT3620_DMA_FIELD_WRITE(MT3620_UART_DMA_RX(handle->id), start, str, false);

            uintptr_t avail = rx_dma->ffcnt;
            uintptr_t chunk = (avail >= size ? size : avail);

            uintptr_t i;
            for (i = 0; i < chunk; i++) {
                ((uint8_t *)data)[i] = UART_BuffRX[handle->id][rx_dma->swptr++];
#if (RX_BUFFER_SIZE < 65536)
                // When the buffer isn't exactly 16-bits we need to handle the wrap bit.
                if ((rx_dma->swptr & 0xFFFF) > RX_BUFFER_SIZE) {
                    rx_dma->swptr &= 0xFFFF0000;
                    rx_dma->swptr ^= 0x00010000;
                }
#endif
            }

            MT3620_DMA_FIELD_WRITE(MT3620_UART_DMA_RX(handle->id), start, str, true);

            data = (void *)((uintptr_t)data + chunk);
            size -= chunk;
        }
    } else {
        while (size > 0) {
            uintptr_t avail = RX_BUFFER_SIZE - handle->rxRemain;

            uintptr_t chunk = (avail > size ? size : avail);

            uintptr_t i;
            for (i = 0; i < chunk; i++) {
                ((uint8_t *)data)[i] = UART_BuffRX[handle->id][handle->rxRead++];
                handle->rxRead %= RX_BUFFER_SIZE;
            }
            handle->rxRemain += chunk;

            data = (void *)((uintptr_t)data + chunk);
            size -= chunk;

            if ((size > 0) && handle->rxCallback) {
                __asm__("wfi");
            }
        }
    }

    return ERROR_NONE;
}

uintptr_t UART_ReadAvailable(UART *handle)
{
    if (!handle || !handle->open) {
        return 0;
    }

    if (handle->dma) {
        return mt3620_dma[MT3620_UART_DMA_RX(handle->id)].ffcnt;
    } else {
        return (RX_BUFFER_SIZE - handle->rxRemain);
    }
}

static void UART_HandleIRQ(Platform_Unit unit)
{
    unsigned id = UART_UnitToID(unit);
    if (id >= MT3620_UART_COUNT) {
        return;
    }

    UART *handle = &context[id];
    if (!handle->open) {
        return;
    }

    mt3620_uart_iir_id_e iirId;
    do {
        // Interrupt Identification Register
        iirId = MT3620_UART_FIELD_READ(handle->id, iir, iir_id);
        switch (iirId) {
        case MT3620_UART_IIR_ID_NO_INTERRUPT_PENDING:
            // The TX FIFO can accept more data.
            break;

        case MT3620_UART_IIR_ID_TX_HOLDING_REGISTER_EMPTY: {
            uint32_t offset = MT3620_UART_FIELD_READ(id, tx_offset, tx_offset);
            uint32_t remain = MT3620_UART_TX_FIFO_DEPTH - offset;

            uint32_t i;
            for (i = 0; (i < remain) && (handle->txRemain < TX_BUFFER_SIZE); i++, handle->txRemain++) {
                mt3620_uart[id]->thr = UART_BuffTX[id][handle->txRead++];
                handle->txRead %= TX_BUFFER_SIZE;
            }

            // If sent all enqueued data then disable TX interrupt.
            if (handle->txRemain == TX_BUFFER_SIZE) {
                // Interrupt Enable Register
                MT3620_UART_FIELD_WRITE(handle->id, ier, etbei, false);
            }
        } break;

        // Read from the FIFO if it has passed its trigger level, or if a timeout
        // has occurred, meaning there is unread data still in the FIFO.
        case MT3620_UART_IIR_ID_RX_DATA_TIMEOUT:
        case MT3620_UART_IIR_ID_RX_DATA_RECEIVED:
            if (!handle->dma) {
                for (; (handle->rxRemain > 0) && MT3620_UART_FIELD_READ(handle->id, lsr, dr); handle->rxRemain--) {
                    UART_BuffRX[id][handle->rxWrite++] = MT3620_UART_FIELD_READ(handle->id, rbr, rbr);
                    handle->rxWrite %= RX_BUFFER_SIZE;
                }
            }

            if (handle->rxCallback) {
                handle->rxCallback();
            }
            break;

        default:
            // Do nothing.
            break;
        } // switch (iirId) {
    } while (iirId != MT3620_UART_IIR_ID_NO_INTERRUPT_PENDING);
}

void uart_irq_b(void)        { UART_HandleIRQ(MT3620_UNIT_UART_DEBUG); }
void isu_g0_uart_irq_b(void) { UART_HandleIRQ(MT3620_UNIT_ISU0      ); }
void isu_g1_uart_irq_b(void) { UART_HandleIRQ(MT3620_UNIT_ISU1      ); }
void isu_g2_uart_irq_b(void) { UART_HandleIRQ(MT3620_UNIT_ISU2      ); }
void isu_g3_uart_irq_b(void) { UART_HandleIRQ(MT3620_UNIT_ISU3      ); }
void isu_g4_uart_irq_b(void) { UART_HandleIRQ(MT3620_UNIT_ISU4      ); }
void isu_g5_uart_irq_b(void) { UART_HandleIRQ(MT3620_UNIT_ISU5      ); }
