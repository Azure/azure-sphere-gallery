/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include "CPUFreq.h"
#include "MBox.h"
#include "NVIC.h"

#include "mt3620/mbox.h"

#include <stdbool.h>
#include <stddef.h>


#define MBOX_MAX_INDEX (MT3620_UNIT_MBOX_CA7 + MT3620_UNIT_MBOX_COUNT - 1)

#define MBOX_DEFAULT_PRIORITY 2

struct MBox {
    bool           fifo_open;
    Platform_Unit  unit;
    void          (*rx_cb)               (void*);
    void          (*tx_confirmed_cb)     (void*);
    void          (*fifo_state_change_cb)(void*, MBox_FIFO_State state);
    void          *user_data;
    int8_t         non_full_threshold;
    int8_t         non_empty_threshold;

    void          (*sw_int_cb)           (void*, uint8_t port);
};

static MBox context[MT3620_UNIT_MBOX_COUNT] = {0};


static inline int32_t MBox__Get_Index(Platform_Unit unit)
{
    int32_t index = unit - MT3620_UNIT_MBOX_CA7;
    return ((index > MBOX_MAX_INDEX) || (index < 0)) ? -1 : index;
}


static void MBox_FIFO__Toggle_Interrupts(MBox *handle, bool enable)
{
    if (!handle) {
        return;
    }
    int32_t index = MBox__Get_Index(handle->unit);
    if (index == -1) {
        return;
    }
    MT3620_MBOX_FIELD_WRITE(
        index, mbox_int_en, int_fifo_wr, enable && !!handle->rx_cb);
    MT3620_MBOX_FIELD_WRITE(
        index, mbox_int_en, int_fifo_rd, enable && !!handle->tx_confirmed_cb);
    MT3620_MBOX_FIELD_WRITE(
        index, mbox_int_en, int_fifo_nf, enable && !!handle->fifo_state_change_cb);
    MT3620_MBOX_FIELD_WRITE(
        index, mbox_int_en, int_fifo_ne, enable && !!handle->fifo_state_change_cb);

    if (enable && handle->rx_cb) {
        NVIC_EnableIRQ(MT3620_MBOX_INTERRUPT(
            index, MT3620_MBOX_INT_RX), MBOX_DEFAULT_PRIORITY);
    }
    else {
        NVIC_DisableIRQ(MT3620_MBOX_INTERRUPT(index, MT3620_MBOX_INT_RX));
    }

    if (enable && handle->tx_confirmed_cb) {
        NVIC_EnableIRQ(MT3620_MBOX_INTERRUPT(
            index, MT3620_MBOX_INT_TX_CONFIRMED), MBOX_DEFAULT_PRIORITY);
    }
    else {
        NVIC_DisableIRQ(MT3620_MBOX_INTERRUPT(
            index, MT3620_MBOX_INT_TX_CONFIRMED));
    }

    if (enable && handle->fifo_state_change_cb) {
        NVIC_EnableIRQ(MT3620_MBOX_INTERRUPT(
            index, MT3620_MBOX_INT_TX_FIFO_NF), MBOX_DEFAULT_PRIORITY);
        NVIC_EnableIRQ(MT3620_MBOX_INTERRUPT(
            index, MT3620_MBOX_INT_TX_FIFO_NE), MBOX_DEFAULT_PRIORITY);
    }
    else {
        NVIC_DisableIRQ(MT3620_MBOX_INTERRUPT(
            index, MT3620_MBOX_INT_TX_FIFO_NF));
        NVIC_DisableIRQ(MT3620_MBOX_INTERRUPT(
            index, MT3620_MBOX_INT_TX_FIFO_NE));
    }
}


MBox* MBox_FIFO_Open(
    Platform_Unit  unit,
    void          (*rx_cb)               (void*),
    void          (*tx_confirmed_cb)     (void*),
    void          (*fifo_state_change_cb)(void*, MBox_FIFO_State state),
    void          *user_data,
    int8_t         non_full_threshold,
    int8_t         non_empty_threshold)
{
    int32_t index = MBox__Get_Index(unit);
    if (index == -1) {
        return NULL;
    }

    MBox *handle = &context[index];

    if (handle->fifo_open) {
        return NULL;
    }

    // TODO: determine if MB needs a reset on open
    MBox_FIFO_Reset(handle, true);

    handle->fifo_open = true;
    handle->unit      = unit;

    handle->rx_cb                = rx_cb;
    handle->tx_confirmed_cb      = tx_confirmed_cb;
    handle->fifo_state_change_cb = fifo_state_change_cb;
    handle->user_data            = user_data;

    handle->non_full_threshold  = non_full_threshold;
    handle->non_empty_threshold = non_empty_threshold;

    // Configure Thresholds
    if ((non_full_threshold > 0) &&
        (non_full_threshold <= MT3620_MBOX_FIFO_COUNT_MAX))
    {
        mt3620_mbox[index]->mbox_nf_thrs = non_full_threshold;
    }

    if ((non_empty_threshold > 0) &&
        (non_empty_threshold <= MT3620_MBOX_FIFO_COUNT_MAX))
    {
        mt3620_mbox[index]->mbox_ne_thrs = non_empty_threshold;
    }

    // Configure Interrupts
    MBox_FIFO__Toggle_Interrupts(handle, true);

    return handle;
}


void MBox_FIFO_Close(MBox *handle)
{
    if (!handle || !handle->fifo_open) {
        return;
    }

    // TODO: is this required?
    MBox_FIFO_Reset(handle, false);

    handle->fifo_open = false;

    handle->rx_cb                = NULL;
    handle->tx_confirmed_cb      = NULL;
    handle->fifo_state_change_cb = NULL;

    handle->non_full_threshold   = -1;
    handle->non_empty_threshold  = -1;

    MBox_FIFO__Toggle_Interrupts(handle, false);
}


void MBox_FIFO_Reset(MBox *handle, bool both)
{
    if (!handle) {
        return;
    }
    // Note that the MB on the partner core must also be reset
    int32_t index = MBox__Get_Index(handle->unit);

    if (index != -1) {
        if (both) {
            MT3620_MBOX_FIELD_WRITE(
                index, mbox_gen_ctrl, soft_rst, 1);
        }
        else {
            MT3620_MBOX_FIELD_WRITE(
                index, mbox_gen_ctrl, soft_rst_myself, 1);
        }
    }
}


int32_t MBox_FIFO_Write(
    MBox           *handle,
    const uint32_t *cmd,
    const uint32_t *data,
    uintptr_t       length)
{
    if (!handle || !handle->fifo_open ||
        length > MT3620_MBOX_FIFO_COUNT_MAX)
    {
        return ERROR_PARAMETER;
    }

    int32_t index = MBox__Get_Index(handle->unit);

    if (index == -1) {
        return ERROR_HARDWARE_STATE;
    }

    if (length > (MT3620_MBOX_FIFO_COUNT_MAX -
        mt3620_mbox[index]->fifo_push_cnt))
    {
        return ERROR_MBOX_FIFO_INSUFFICIENT_SPACE;
    }

    // CMD write increments the write ptr, so we write any DATA first
    for (unsigned i = 0; i < length; i++) {
        if (data) {
            mt3620_mbox[index]->data_push = data[i];
        }
        mt3620_mbox[index]->cmd_push = cmd[i];
    }

    return ERROR_NONE;
}


int32_t MBox_FIFO_ReadSync(
    MBox      *handle,
    uint32_t  *cmd,
    uint32_t  *data,
    uintptr_t  length)
{
    if (!handle || !handle->fifo_open ||
        length > MT3620_MBOX_FIFO_COUNT_MAX)
    {
        return ERROR_PARAMETER;
    }

    int32_t index = MBox__Get_Index(handle->unit);

    if (index == -1) {
        return ERROR_HARDWARE_STATE;
    }

    for (unsigned i = 0; i < length; i++) {
        // wait until data is available to read
        while (mt3620_mbox[index]->fifo_pop_cnt == 0);
        // As above, CMD read decrements the read ptr, so we read DATA first
        if (data) {
            data[i] = mt3620_mbox[index]->data_pop;
        }
        cmd[i] = mt3620_mbox[index]->cmd_pop;
    }

    return ERROR_NONE;
}


uintptr_t MBox_FIFO_Writes_Pending(const MBox *handle)
{
    if (!handle || !handle->fifo_open) {
        return 0;
    }

    int32_t index = MBox__Get_Index(handle->unit);

    if (index == -1) {
        return 0;
    }

    return mt3620_mbox[index]->fifo_push_cnt;
}


uintptr_t MBox_FIFO_Reads_Available(const MBox *handle)
{
    if (!handle || !handle->fifo_open) {
        return 0;
    }

    int32_t index = MBox__Get_Index(handle->unit);

    if (index == -1) {
        return 0;
    }

    return mt3620_mbox[index]->fifo_pop_cnt;
}



/// Semaphore
int32_t MBox_Semaphore_Acquire(MBox* handle)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    if (handle->unit == MT3620_UNIT_MBOX_CM4) {
        return ERROR_MBOX_SEMAPHORE_NOT_PRESENT;
    }

    int32_t index = MBox__Get_Index(handle->unit);

    if (index == -1) {
        return ERROR_HARDWARE_STATE;
    }

    if (mt3620_mbox[index]->semaphore_p != 1U) {
        return ERROR_MBOX_SEMAPHORE_REQUEST_DENIED;
    }
    else {
        return ERROR_NONE;
    }
}


int32_t MBox_Semaphore_Release(MBox* handle)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    if (handle->unit == MT3620_UNIT_MBOX_CM4) {
        return ERROR_MBOX_SEMAPHORE_NOT_PRESENT;
    }

    int32_t index = MBox__Get_Index(handle->unit);

    if (index == -1) {
        return ERROR_HARDWARE_STATE;
    }

    if (mt3620_mbox[index]->semaphore_p != 1U) {
        return ERROR_MBOX_SEMAPHORE_REQUEST_DENIED;
    }
    else {
        mt3620_mbox[index]->semaphore_p = 0;
        return ERROR_NONE;
    }
}



/// SW Interrupts
int32_t MBox_SW_Interrupt_Setup(
    MBox    *handle,
    uint8_t  int_enable_flags,
    void    (*sw_int_cb)(void*, uint8_t port))
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    handle->sw_int_cb = sw_int_cb;

    int32_t index = MBox__Get_Index(handle->unit);

    if (index == -1) {
        return ERROR_HARDWARE_STATE;
    }

    mt3620_mbox[index]->sw_rx_int_en = int_enable_flags;

    // Configure Interrupts
    if (sw_int_cb) {
        NVIC_EnableIRQ(MT3620_MBOX_INTERRUPT(
            index, MT3620_MBOX_INT_SW_INT), MBOX_DEFAULT_PRIORITY);
    }
    else {
        NVIC_DisableIRQ(MT3620_MBOX_INTERRUPT(index, MT3620_MBOX_INT_SW_INT));
    }

    return ERROR_NONE;
}


void MBox_SW_Interrupt_Teardown(MBox *handle)
{
    if (!handle) {
        return;
    }

    handle->sw_int_cb = NULL;

    int32_t index = MBox__Get_Index(handle->unit);

    if (index == -1) {
        return;
    }

    mt3620_mbox[index]->sw_rx_int_en = 0;

    NVIC_DisableIRQ(MT3620_MBOX_INTERRUPT(index, MT3620_MBOX_INT_SW_INT));
}


int32_t MBox_SW_Interrupt_Trigger(MBox *handle, uint8_t port)
{
    if (!handle || (port >= MBOX_SW_INT_PORT_COUNT)) {
        return ERROR_PARAMETER;
    }

    int32_t index = MBox__Get_Index(handle->unit);

    if (index == -1) {
        return ERROR_HARDWARE_STATE;
    }

    mt3620_mbox[index]->sw_tx_int_port = 1U << port;

    return ERROR_NONE;
}



/// IRQs
static void MBox_IRQ(MBox *handle, mt3620_mbox_int cb_type)
{
    if (!handle || !handle->fifo_open) {
        return;
    }

    switch (cb_type) {
    case MT3620_MBOX_INT_RX:
        if (handle->rx_cb) {
            handle->rx_cb(handle->user_data);
        }
        break;

    case MT3620_MBOX_INT_TX_CONFIRMED:
        if (handle->tx_confirmed_cb) {
            handle->tx_confirmed_cb(handle->user_data);
        }
        break;

    case MT3620_MBOX_INT_TX_FIFO_NF:
        if (handle->fifo_state_change_cb) {
            handle->fifo_state_change_cb(
                handle->user_data, MBOX_FIFO_STATE_NOT_FULL);
        }
        break;

    case MT3620_MBOX_INT_TX_FIFO_NE:
        if (handle->fifo_state_change_cb) {
            handle->fifo_state_change_cb(
                handle->user_data, MBOX_FIFO_STATE_NOT_EMPTY);
        }
        break;

    case MT3620_MBOX_INT_SW_INT:
    {
        if (!handle->sw_int_cb) {
            break;
        }
        uint8_t flags =
            (mt3620_mbox[MBox__Get_Index(handle->unit)]->sw_rx_int_sts) &
            (mt3620_mbox[MBox__Get_Index(handle->unit)]->sw_rx_int_en);

        for (uint8_t port = 0;
             port < MBOX_SW_INT_PORT_COUNT;
             port++, flags >>= port) {
            if (flags & 1U) {
                handle->sw_int_cb(handle->user_data, port);
            }
        }
        break;
    }

    default:
        break;
    }
}

static inline void mbox_rd_int(int32_t index)
{
    if (MT3620_MBOX_FIELD_READ(index, mbox_int_en, int_fifo_rd) &&
        MT3620_MBOX_FIELD_READ(index, mbox_int_sts, int_fifo_rd))
    {
        MBox *handle = &(context[index]);
        MBox_IRQ(handle, MT3620_MBOX_INT_TX_CONFIRMED);
    }

    MT3620_MBOX_FIELD_WRITE(index, mbox_int_sts, int_fifo_rd, 1);
}

static inline void mbox_nf_int(int32_t index)
{
    if (MT3620_MBOX_FIELD_READ(index, mbox_int_en, int_fifo_nf) &&
        MT3620_MBOX_FIELD_READ(index, mbox_int_sts, int_fifo_nf))
    {
        MBox *handle = &(context[MBox__Get_Index(MT3620_UNIT_MBOX_CA7)]);
        MBox_IRQ(handle, MT3620_MBOX_INT_TX_FIFO_NF);
    }

    MT3620_MBOX_FIELD_WRITE(index, mbox_int_sts, int_fifo_nf, 1);
}

static inline void mbox_wr_int(int32_t index)
{
    if (MT3620_MBOX_FIELD_READ(index, mbox_int_en, int_fifo_wr) &&
        MT3620_MBOX_FIELD_READ(index, mbox_int_sts, int_fifo_wr))
    {
        MBox *handle = &(context[MBox__Get_Index(MT3620_UNIT_MBOX_CA7)]);
        MBox_IRQ(handle, MT3620_MBOX_INT_RX);
    }

    MT3620_MBOX_FIELD_WRITE(index, mbox_int_sts, int_fifo_wr, 1);
}

static inline void mbox_ne_int(int32_t index)
{
    if (MT3620_MBOX_FIELD_READ(index, mbox_int_en, int_fifo_ne) &&
        MT3620_MBOX_FIELD_READ(index, mbox_int_sts, int_fifo_ne))
    {
        MBox *handle = &(context[MBox__Get_Index(MT3620_UNIT_MBOX_CA7)]);
        MBox_IRQ(handle, MT3620_MBOX_INT_TX_FIFO_NE);
    }

    MT3620_MBOX_FIELD_WRITE(index, mbox_int_sts, int_fifo_ne, 1);
}

static inline void mbox_sw_int(int32_t index)
{
    if ((mt3620_mbox[index]->sw_rx_int_en &
         mt3620_mbox[index]->sw_rx_int_sts) != 0)
    {
        MBox *handle = &(context[MBox__Get_Index(MT3620_UNIT_MBOX_CA7)]);
        MBox_IRQ(handle, MT3620_MBOX_INT_SW_INT);
    }

    // Reset interrupt flags
    mt3620_mbox[index]->sw_rx_int_sts = 0xFF;
}

/// M4 <-> A7 Mailbox
void cm4_mbox_m4a2a7n_rd_int(void)
{
    mbox_rd_int(MBox__Get_Index(MT3620_UNIT_MBOX_CA7));
}

void cm4_mbox_m4a2a7n_nf_int(void)
{
    mbox_nf_int(MBox__Get_Index(MT3620_UNIT_MBOX_CA7));
}

void cm4_mbox_a7n2m4a_wr_int(void)
{
    mbox_wr_int(MBox__Get_Index(MT3620_UNIT_MBOX_CA7));
}

void cm4_mbox_a7n2m4a_ne_int(void)
{
    mbox_ne_int(MBox__Get_Index(MT3620_UNIT_MBOX_CA7));
}

void cm4_mbox_a7n_fifo_int(void)
{
    // TODO: find if is this triggered for all of above & implement
}

void cm4_mbox_a7n2m4a_sw_int(void)
{
    mbox_sw_int(MBox__Get_Index(MT3620_UNIT_MBOX_CA7));
}

/// M4 <-> M4 Mailbox
void cm4_mbox_m4a2m4b_rd_int(void)
{
    mbox_rd_int(MBox__Get_Index(MT3620_UNIT_MBOX_CM4));
}

void cm4_mbox_m4a2m4b_nf_int(void)
{
    mbox_nf_int(MBox__Get_Index(MT3620_UNIT_MBOX_CM4));
}

void cm4_mbox_m4b2m4a_wr_int(void)
{
    mbox_wr_int(MBox__Get_Index(MT3620_UNIT_MBOX_CM4));
}

void cm4_mbox_m4b2m4a_ne_int(void)
{
    mbox_ne_int(MBox__Get_Index(MT3620_UNIT_MBOX_CM4));
}

void cm4_mbox_m4b_fifo_int(void)
{
    // TODO: find if is this triggered for all of above & implement
}

void cm4_mbox_m4b2m4a_sw_int(void)
{
    mbox_sw_int(MBox__Get_Index(MT3620_UNIT_MBOX_CM4));
}
