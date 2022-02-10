/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include "I2CMaster.h"
#include "NVIC.h"
#include "mt3620/i2c.h"
#include "mt3620/dma.h"
#include <stdbool.h>


static inline void fifoClear(volatile uint32_t *fifo, bool tx, bool rx)
{
    mt3620_i2c_mm_fifo_con0_t fifo_con0 = { .mask = 0 };
    fifo_con0.tx_fifo_clr = tx;
    fifo_con0.rx_fifo_clr = rx;
    *fifo = fifo_con0.mask;
}

static inline void fifoClearMaster(unsigned id, bool tx, bool rx)
{
    fifoClear(&mt3620_i2c[id]->mm_fifo_con0, tx, rx);
}

static inline void fifoClearSubordinate(unsigned id, bool tx, bool rx)
{
    fifoClear(&mt3620_i2c[id]->s_fifo_con0, tx, rx);
}


struct I2CMaster {
    bool                open;
    uint32_t            id;
    void              (*callback)    (int32_t, uintptr_t);
    void              (*callbackUser)(int32_t, uintptr_t, void*);
    void               *userData;
    uintptr_t           txQueued, rxQueued;
    const I2C_Transfer *transfer;
    uint32_t            count;
    bool                useDMA;
    volatile bool       error;
};

static I2CMaster context[MT3620_I2C_COUNT] = {0};

// TODO: Clean this up.
#define I2C_PRIORITY 2

static inline unsigned I2CMaster_UnitToID(Platform_Unit unit)
{
    if ((unit < MT3620_UNIT_ISU0) || (unit > MT3620_UNIT_ISU5)) {
        return MT3620_I2C_COUNT;
    }
    return (unit - MT3620_UNIT_ISU0);
}

I2CMaster *I2CMaster_Open(Platform_Unit unit)
{
    unsigned id = I2CMaster_UnitToID(unit);
    if (id >= MT3620_I2C_COUNT) {
        return NULL;
    }

    if (context[id].open) {
        return NULL;
    }

    // Detect if interface is in-use as a subordinate device.
    if (MT3620_I2C_FIELD_READ(id, s_con0, slave_en)) {
        return NULL;
    }

    // TODO: Find more atomic way of separating master and subordinate device drivers.
    // Enable master mode immediately to reduce risk of collision with subordinate device.
    MT3620_I2C_FIELD_WRITE(id, mm_con0, master_en, true);

    context[id].id           = id;
    context[id].open         = true;
    context[id].callback     = NULL;
    context[id].callbackUser = NULL;
    context[id].userData     = NULL;
    context[id].txQueued     = 0;
    context[id].rxQueued     = 0;
    context[id].transfer     = NULL;
    context[id].count        = 0;
    context[id].useDMA       = false;
    context[id].error        = false;

    // Enable Sync mode.
    MT3620_I2C_FIELD_WRITE(id, mm_pad_con0, sync_en, true);

    I2CMaster_SetBusSpeed(&context[id], I2C_BUS_SPEED_STANDARD);

    fifoClearMaster(id, true, true);
    fifoClearSubordinate(id, true, true);

    mt3620_dma_global->ch_en_set = (1U << MT3620_I2C_DMA_TX(id));
    MT3620_DMA_FIELD_WRITE(MT3620_I2C_DMA_TX(id), start, str, false);
    mt3620_dma_con_t dma_con_tx = { .mask = mt3620_dma[MT3620_I2C_DMA_TX(id)].con };
    dma_con_tx.dir   = 0;
    dma_con_tx.wpen  = false;
    dma_con_tx.wpsd  = 0;
    dma_con_tx.iten  = false;
    dma_con_tx.hiten = false;
    dma_con_tx.dreq  = true;
    dma_con_tx.dinc  = 0;
    dma_con_tx.sinc  = 1;
    dma_con_tx.size  = 0;
    mt3620_dma[MT3620_I2C_DMA_TX(id)].con = dma_con_tx.mask;
    mt3620_dma[MT3620_I2C_DMA_TX(id)].fixaddr = (uint32_t *)&mt3620_i2c[id]->mm_fifo_data;

    mt3620_dma_global->ch_en_set = (1U << MT3620_I2C_DMA_RX(id));
    MT3620_DMA_FIELD_WRITE(MT3620_I2C_DMA_RX(id), start, str, false);
    mt3620_dma_con_t dma_con_rx = { .mask = mt3620_dma[MT3620_I2C_DMA_RX(id)].con };
    dma_con_rx.dir   = 1;
    dma_con_rx.wpen  = false;
    dma_con_rx.wpsd  = 1;
    dma_con_rx.iten  = false;
    dma_con_rx.hiten = false;
    dma_con_rx.dreq  = true;
    dma_con_rx.dinc  = 1;
    dma_con_rx.sinc  = 0;
    dma_con_rx.size  = 0;
    mt3620_dma[MT3620_I2C_DMA_RX(id)].con = dma_con_rx.mask;
    mt3620_dma[MT3620_I2C_DMA_RX(id)].fixaddr = (uint32_t *)&mt3620_i2c[id]->mm_fifo_data;

    // Enable DMA handshakes for master mode.
    mt3620_i2c_dma_con0_t dma_con0 = { .mask = mt3620_i2c[id]->dma_con0 };
    dma_con0.dma_hs_sel = 0;
    dma_con0.dma_hs_en  = true;
    mt3620_i2c[id]->dma_con0 = dma_con0.mask;

    MT3620_I2C_FIELD_WRITE(id, int_ctrl, mm_int_sta, true);

    // Enable and Set the NVIC interrupt priority.
    NVIC_EnableIRQ(MT3620_I2C_INTERRUPT(id), I2C_PRIORITY);

    MT3620_I2C_FIELD_WRITE(id, int_ctrl, mm_int_en, true);

    return &context[id];
}

void I2CMaster_Close(I2CMaster *handle)
{
    if (!handle || !handle->open) {
        return;
    }

    mt3620_dma_global->ch_en_clr = (1U << MT3620_I2C_DMA_TX(handle->id));
    mt3620_dma_global->ch_en_clr = (1U << MT3620_I2C_DMA_RX(handle->id));

    MT3620_I2C_FIELD_WRITE(handle->id, int_ctrl, mm_int_en, false);
    MT3620_I2C_FIELD_WRITE(handle->id, mm_con0 , master_en, false);

    // Disable NVIC interrupts.
    NVIC_DisableIRQ(MT3620_I2C_INTERRUPT(handle->id));

    handle->open = false;
}


int32_t I2CMaster_SetBusSpeed(I2CMaster *handle, I2C_BusSpeed speed)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }
    if (!handle->open) {
        return ERROR_HANDLE_CLOSED;
    }

    if ((speed == 0) || (speed > MT3620_I2C_MAX_SPEED)) {
        return ERROR_UNSUPPORTED;
    }

    // TODO: Update this to use additional logic from Linux driver.

    uint32_t period = MT3620_I2C_CLOCK / speed;

    if (period >= (255 * 4)) {
        return ERROR_UNSUPPORTED;
    }

    mt3620_i2c_vec4_t phl = { .mask = mt3620_i2c[handle->id]->mm_cnt_val_phl };
    mt3620_i2c_vec4_t phh = { .mask = mt3620_i2c[handle->id]->mm_cnt_val_phh };

    phl.x = ((period + 1) / 4);
    phl.y = ((period + 0) / 4);
    phh.x = ((period + 2) / 4);
    phh.y = ((period + 3) / 4);

    mt3620_i2c[handle->id]->mm_cnt_val_phl = phl.mask;
    mt3620_i2c[handle->id]->mm_cnt_val_phh = phh.mask;

    return ERROR_NONE;
}

int32_t I2CMaster_GetBusSpeed(const I2CMaster *handle, I2C_BusSpeed *speed)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }
    if (!handle->open) {
        return ERROR_HANDLE_CLOSED;
    }

    mt3620_i2c_vec4_t phl = { .mask = mt3620_i2c[handle->id]->mm_cnt_val_phl };
    mt3620_i2c_vec4_t phh = { .mask = mt3620_i2c[handle->id]->mm_cnt_val_phh };

    unsigned period = phl.x + phl.y + phh.x + phh.y;
    if (period == 0) {
        return ERROR_HARDWARE_STATE;
    }

    if (speed) {
        *speed = (MT3620_I2C_CLOCK / period);
    }
    return ERROR_NONE;
}


// TODO: Move this to a dedicated/common memory/sram header.
// TODO: Reference datasheet for address ranges.
static bool addrOnBus(const void* ptr)
{
    uintptr_t addr = (uintptr_t)ptr;

    if ((addr >= 0x00010000) && (addr < 0x10000000)) {
        return false;
    }

    if ((addr >= 0x20000000) && (addr < 0x20100000)) {
        return false;
    }

    if ((addr >= 0xE000E000) && (addr < 0xE000F000)) {
        return false;
    }

    return true;
}

int32_t I2CMaster_TransferSequentialAsync_UserData(
    I2CMaster *handle, uint16_t address,
    const I2C_Transfer *transfer, uint32_t count,
    void (*callback)(int32_t status, uintptr_t count, void* userData),
    void *userData)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }
    if (!handle->open) {
        return ERROR_HANDLE_CLOSED;
    }
    if (handle->transfer) {
        return ERROR_BUSY;
    }

    // We don't support more than 7-bit addressing.
    if ((address >> 7) != 0) {
        return ERROR_UNSUPPORTED;
    }

    if (!transfer || (count == 0)) {
        return ERROR_PARAMETER;
    }

    // It's up to the user to group transfers of the same type
    if (count > MT3620_I2C_QUEUE_DEPTH) {
        return ERROR_UNSUPPORTED;
    }

    bool onBus = true;
    unsigned wcnt  = 0, rcnt  = 0;
    unsigned wdata = 0, rdata = 0;
    unsigned i;
    for (i = 0; i < count; i++) {
        if (transfer[i].length > MT3620_I2C_PACKET_SIZE_MAX) {
            return ERROR_UNSUPPORTED;
        }

        // Transfer must be a read or a write.
        if (!transfer[i].writeData && !transfer[i].readData) {
            return ERROR_PARAMETER;
        }

        // I2C doesn't support duplex transfers.
        if (transfer[i].writeData && transfer[i].readData) {
            return ERROR_UNSUPPORTED;
        }

        if (transfer[i].writeData) {
            if (!addrOnBus(transfer[i].writeData)) {
                onBus = false;
            }

            wcnt++;
            wdata += transfer[i].length;
        }

        if (transfer[i].readData) {
            if (!addrOnBus(transfer[i].readData)) {
                onBus = false;
            }

            rcnt++;
            rdata += transfer[i].length;
        }
    }

    if (MT3620_I2C_FIELD_READ(handle->id, mm_status, bus_busy)) {
        return ERROR_BUSY;
    }

    bool useDMA = false;
    if ((wdata > MT3620_I2C_TX_FIFO_DEPTH)
        || (rdata > MT3620_I2C_RX_FIFO_DEPTH)) {
        // DMA can only access data on the bus (i.e. not TCM).
        if (!onBus) {
            return ERROR_DMA_SOURCE;
        }

        // DMA can queue a maximum of two transactions in each direction.
        if ((wcnt > 2) || (rcnt > 2)) {
            return ERROR_UNSUPPORTED;
        }

        useDMA = true;
    }

    handle->callbackUser = callback;
    handle->userData     = userData;
    handle->useDMA       = useDMA;
    handle->txQueued     = wdata;
    handle->rxQueued     = rdata;
    handle->transfer     = transfer;
    handle->count        = count;

    mt3620_i2c[handle->id]->mm_slave_id = address;

    for (i = 0; i < count; i++) {
        mt3620_i2c[handle->id]->mm_cnt_byte_val_pk[i] = transfer[i].length;
    }

    mt3620_i2c_mm_pack_con0_t mm_pack_con0 = { .mask = mt3620_i2c[handle->id]->mm_pack_con0 };
    mm_pack_con0.mm_pack_rw0 = (transfer[0].readData != NULL);
    if (count >= 2) mm_pack_con0.mm_pack_rw1 = (transfer[1].readData != NULL);
    if (count >= 3) mm_pack_con0.mm_pack_rw2 = (transfer[2].readData != NULL);
    mm_pack_con0.mm_pack_val = (count - 1);
    mt3620_i2c[handle->id]->mm_pack_con0 = mm_pack_con0.mask;

    if (useDMA) {
        volatile mt3620_dma_t * const tx_dma = &mt3620_dma[MT3620_I2C_DMA_TX(handle->id)];
        volatile mt3620_dma_t * const rx_dma = &mt3620_dma[MT3620_I2C_DMA_RX(handle->id)];

        unsigned w = 0, r = 0;
        for (i = 0; i < count; i++) {
            if (transfer[i].writeData) {
                if (w++ == 0) {
                    tx_dma->pgmaddr = (uint32_t *)transfer[i].writeData;
                    tx_dma->wppt    = transfer[i].length;
                    tx_dma->count   = transfer[i].length;
                } else {
                    tx_dma->wpto    = (uint32_t *)transfer[i].writeData;
                    tx_dma->count  += transfer[i].length;
                }
            } else {
                if (r++ == 0) {
                    rx_dma->pgmaddr = transfer[i].readData;
                    rx_dma->wppt    = transfer[i].length;
                    rx_dma->count   = transfer[i].length;
                } else {
                    rx_dma->wpto    = transfer[i].readData;
                    rx_dma->count  += transfer[i].length;
                }
            }
        }

        if (wcnt > 0) {
            MT3620_DMA_FIELD_WRITE(MT3620_I2C_DMA_TX(handle->id), con, wpen, (wcnt > 1));

            // Start DMA TX transfer.
            MT3620_DMA_FIELD_WRITE(MT3620_I2C_DMA_TX(handle->id), start, str, true);
        }

        if (rcnt > 0) {
            MT3620_DMA_FIELD_WRITE(MT3620_I2C_DMA_RX(handle->id), con, wpen, (rcnt > 1));

            // Start DMA RX transfer.
            MT3620_DMA_FIELD_WRITE(MT3620_I2C_DMA_RX(handle->id), start, str, true);
        }
    } else {
        // Pre-fill TX buffer with data.
        for (i = 0; i < count; i++) {
            const uint8_t *writeData = transfer[i].writeData;
            if (writeData) {
                unsigned j;
                for (j = 0; j < transfer[i].length; j++) {
                    mt3620_i2c[handle->id]->mm_fifo_data = writeData[j];
                }
            }
        }
    }

    // Wait until I2C is ready after setting config
    while (!MT3620_I2C_FIELD_READ(handle->id, mm_status, mm_start_ready)) {
        // Do nothing.
    }

    mt3620_i2c_mm_con0_t mm_con0 = { .mask = mt3620_i2c[handle->id]->mm_con0 };
    mm_con0.mm_gmode    = true;
    mm_con0.mm_start_en = true;
    mt3620_i2c[handle->id]->mm_con0 = mm_con0.mask;

    return ERROR_NONE;
}

int32_t I2CMaster_TransferSequentialAsync(
    I2CMaster *handle, uint16_t address,
    const I2C_Transfer *transfer, uint32_t count,
    void (*callback)(int32_t status, uintptr_t count))
{
    if (!handle || !callback) {
        return ERROR_PARAMETER;
    }

    handle->callback = callback;

    int32_t error = I2CMaster_TransferSequentialAsync_UserData(
        handle, address, transfer, count, NULL, NULL);

    if (error != ERROR_NONE) {
        handle->callback = NULL;
    }

    return error;
}

typedef struct {
    volatile bool ready;
    int32_t       status;
    int32_t       count;
} I2CMaster_TransferSequentialSync_State;

static void I2CMaster_TransferSequentialSync_Callback(
    int32_t status, uintptr_t count, void *userData)
{
    if (!userData) {
        return;
    }

    I2CMaster_TransferSequentialSync_State *state = userData;
    state->status = status;
    state->count  = count;
    state->ready  = true;
}

int32_t I2CMaster_TransferSequentialSync(
    I2CMaster *handle, uint16_t address,
    const I2C_Transfer *transfer, uint32_t count)
{
    I2CMaster_TransferSequentialSync_State state = {0};
    int32_t status = I2CMaster_TransferSequentialAsync_UserData(
        handle, address, transfer, count,
        I2CMaster_TransferSequentialSync_Callback, &state);

    if (status != ERROR_NONE) {
        return status;
    }

    while (!state.ready) {
        __asm__("wfi");
    }

    return state.status;
}


static void I2CMaster_IRQ(Platform_Unit unit)
{
    unsigned id = I2CMaster_UnitToID(unit);
    if (id >= MT3620_I2C_COUNT) {
        return;
    }

    MT3620_I2C_FIELD_WRITE(id, int_ctrl, mm_int_sta, true);

    I2CMaster* handle = &context[id];

    // This should never happen
    if (!handle->open) {
        return;
    }

    int32_t status = ERROR_NONE;
    if ((MT3620_I2C_FIELD_READ(id, mm_ack_val, mm_ack_id) & 0x1) != 0) {
        status = ERROR_I2C_ADDRESS_NACK;
    } else if (MT3620_I2C_FIELD_READ(id, mm_status, mm_arb_had_lose)) {
        MT3620_I2C_FIELD_WRITE(id, mm_status, mm_arb_had_lose, 1);
        status = ERROR_I2C_ARBITRATION_LOST;
    }

    uintptr_t txRemain = 0;
    uintptr_t rxRemain = 0;
    if (handle->useDMA) {
        MT3620_DMA_FIELD_WRITE(MT3620_I2C_DMA_TX(id), start, str, false);
        MT3620_DMA_FIELD_WRITE(MT3620_I2C_DMA_RX(id), start, str, false);

        txRemain += mt3620_dma[MT3620_I2C_DMA_TX(id)].rlct;
        rxRemain += mt3620_dma[MT3620_I2C_DMA_RX(id)].rlct;
    } else {
        unsigned i;
        for (i = 0; i < handle->count; i++) {
            uint8_t *readData = handle->transfer[i].readData;
            if (readData) {
                unsigned j;
                for (j = 0; j < handle->transfer[i].length; j++) {
                    if (!MT3620_I2C_FIELD_READ(id, mm_fifo_status, rx_fifo_emp)) {
                        readData[j] = mt3620_i2c[handle->id]->mm_fifo_data;
                    } else {
                        rxRemain++;
                    }
                }
            }
        }
    }

    mt3620_i2c_fifo_status_t fifo_status = { .mask = mt3620_i2c[id]->mm_fifo_status };
    mt3620_i2c_fifo_ptr_t    fifo_ptr    = { .mask = mt3620_i2c[id]->mm_fifo_ptr    };

    bool txClear = !fifo_status.tx_fifo_emp;
    if (txClear) {
        txRemain += (fifo_status.tx_fifo_full ? MT3620_I2C_TX_FIFO_DEPTH
            : (fifo_ptr.tx_fifo_wptr - fifo_ptr.tx_fifo_rptr));
    }

    bool rxClear = !fifo_status.rx_fifo_emp;
    if (rxClear) {
        rxRemain += (fifo_status.rx_fifo_full ? MT3620_I2C_RX_FIFO_DEPTH
            : (fifo_ptr.rx_fifo_wptr - fifo_ptr.rx_fifo_rptr));
    }

    if (txClear || rxClear) {
        fifoClearMaster(id, txClear, rxClear);
        if (status == ERROR_NONE) status = ERROR_I2C_TRANSFER_INCOMPLETE;
    }

    uintptr_t txCount = (txRemain > handle->txQueued
        ? 0 : (handle->txQueued - txRemain));
    uintptr_t rxCount = (rxRemain > handle->rxQueued
        ? 0 : (handle->rxQueued - rxRemain));

    uintptr_t dataCount = (txCount + rxCount);

    if (handle->callback) {
        handle->callback(status, dataCount);
    } else if (handle->callbackUser) {
        handle->callbackUser(status, dataCount, handle->userData);
    }

    handle->error        = (status != ERROR_NONE);
    handle->transfer     = NULL;
    handle->count        = 0;
    handle->txQueued     = 0;
    handle->rxQueued     = 0;
    handle->callback     = NULL;
    handle->callbackUser = NULL;
    handle->userData     = NULL;
}

void isu_g0_i2c_irq(void) { I2CMaster_IRQ(MT3620_UNIT_ISU0); }
void isu_g1_i2c_irq(void) { I2CMaster_IRQ(MT3620_UNIT_ISU1); }
void isu_g2_i2c_irq(void) { I2CMaster_IRQ(MT3620_UNIT_ISU2); }
void isu_g3_i2c_irq(void) { I2CMaster_IRQ(MT3620_UNIT_ISU3); }
void isu_g4_i2c_irq(void) { I2CMaster_IRQ(MT3620_UNIT_ISU4); }
void isu_g5_i2c_irq(void) { I2CMaster_IRQ(MT3620_UNIT_ISU5); }
