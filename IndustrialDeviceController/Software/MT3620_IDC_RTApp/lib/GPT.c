/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include "GPT.h"
#include "CPUFreq.h"
#include "NVIC.h"
#include "mt3620/gpt.h"

#include <stddef.h>


// Define this to enable setting of speed in GPT3
#define GPT3_ENABLE_NON_STANDARD_SPEED

#define MAX_TIMER_INDEX (MT3620_UNIT_GPT_COUNT + MT3620_UNIT_GPT0 - 1)

#define GPT_PRIORITY 2
#define GPT01_IRQ    1
#define GPT3_IRQ     2

struct GPT {
    bool               open;
    uint32_t           id;
    GPT_Mode           mode;
    void             (*callback)(GPT *);
    uint32_t           numCycles;
    volatile uint32_t  initCnt;
    volatile bool      paused;
};

static GPT context[MT3620_UNIT_GPT_COUNT] = {0};


static inline int32_t GPT__GetCtrlValue(
    GPT       *handle,
    bool      *enabled,
    GPT_Mode  *mode,
    bool      *speed,
    bool      *restart)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    int32_t error = ERROR_NONE;
    switch (handle->id) {
    case MT3620_UNIT_GPT0:
        if (enabled) {
            *enabled = MT3620_GPT_FIELD_READ(gpt0_ctrl, en);
        }
        if (mode) {
            *mode = MT3620_GPT_FIELD_READ(gpt0_ctrl, mode);
        }
        if (speed) {
            *speed = MT3620_GPT_FIELD_READ(gpt0_ctrl, speed);
        }
        if (restart) {
            *restart = MT3620_GPT_FIELD_READ(gpt0_ctrl, restart);
        }
        break;

    case MT3620_UNIT_GPT1:
        if (enabled) {
            *enabled = MT3620_GPT_FIELD_READ(gpt1_ctrl, en);
        }
        if (mode) {
            *mode = MT3620_GPT_FIELD_READ(gpt1_ctrl, mode);
        }
        if (speed) {
            *speed = MT3620_GPT_FIELD_READ(gpt1_ctrl, speed);
        }
        if (restart) {
            *restart = MT3620_GPT_FIELD_READ(gpt1_ctrl, restart);
        }
        break;

    case MT3620_UNIT_GPT2:
        if (enabled) {
            *enabled = MT3620_GPT_FIELD_READ(gpt2_ctrl, en);
        }
        if (mode) {
            *mode = GPT_MODE_NONE;
        }
        if (speed) {
            *speed = MT3620_GPT_FIELD_READ(gpt2_ctrl, speed);
        }
        if (restart) {
            error = ERROR_UNSUPPORTED;
        }
        break;

    case MT3620_UNIT_GPT3:
        if (enabled) {
            *enabled = MT3620_GPT_FIELD_READ(gpt3_ctrl, en);
        }
        if (mode) {
            *mode = handle->mode;
        }
        if (speed) {
            error = ERROR_UNSUPPORTED;
        }
        if (restart) {
            error = ERROR_UNSUPPORTED;
        }
        break;

    case MT3620_UNIT_GPT4:
        if (enabled) {
            *enabled = MT3620_GPT_FIELD_READ(gpt4_ctrl, en);
        }
        if (mode) {
            *mode = GPT_MODE_NONE;
        }
        if (speed) {
            *speed = MT3620_GPT_FIELD_READ(gpt4_ctrl, speed);
        }
        if (restart) {
            error = ERROR_UNSUPPORTED;
        }
        break;

    default:
        error = ERROR_UNSUPPORTED;
        break;
    }

    return error;
}

static inline int32_t GPT__SetCtrlValue(
    GPT       *handle,
    bool      *enabled,
    GPT_Mode  *mode,
    bool      *speed,
    bool      *restart)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    int32_t error = ERROR_NONE;
    switch (handle->id) {
    case MT3620_UNIT_GPT0:
        if (enabled) {
            MT3620_GPT_FIELD_WRITE(gpt0_ctrl, en, *enabled);
        }
        if (mode) {
            MT3620_GPT_FIELD_WRITE(gpt0_ctrl, mode, *mode);
        }
        if (speed) {
            MT3620_GPT_FIELD_WRITE(gpt0_ctrl, speed, *speed);
        }
        if (restart) {
            MT3620_GPT_FIELD_WRITE(gpt0_ctrl, restart, *restart);
        }
        break;

    case MT3620_UNIT_GPT1:
        if (enabled) {
            MT3620_GPT_FIELD_WRITE(gpt1_ctrl, en, *enabled);
        }
        if (mode) {
            MT3620_GPT_FIELD_WRITE(gpt1_ctrl, mode, *mode);
        }
        if (speed) {
            MT3620_GPT_FIELD_WRITE(gpt1_ctrl, speed, *speed);
        }
        if (restart) {
            MT3620_GPT_FIELD_WRITE(gpt1_ctrl, restart, *restart);
        }
        break;

    case MT3620_UNIT_GPT2:
        if (enabled) {
            MT3620_GPT_FIELD_WRITE(gpt2_ctrl, en, *enabled);
        }
        if (mode && (*mode != GPT_MODE_NONE)) {
            error = ERROR_UNSUPPORTED;
        }
        if (speed) {
            MT3620_GPT_FIELD_WRITE(gpt2_ctrl, speed, *speed);
        }
        if (restart) {
            error = ERROR_UNSUPPORTED;
        }
        break;

    case MT3620_UNIT_GPT3:
        if (enabled) {
            MT3620_GPT_FIELD_WRITE(gpt3_ctrl, en, *enabled);
        }
        if (mode) {
            // GPT3 doesn't support repeat in HW, but we can emulate it in HW
            handle->mode = *mode;
        }
        if (speed || restart) {
            error = ERROR_UNSUPPORTED;
        }
        break;

    case MT3620_UNIT_GPT4:
        if (enabled) {
            MT3620_GPT_FIELD_WRITE(gpt4_ctrl, en, *enabled);
        }
        if (mode && (*mode != GPT_MODE_NONE)) {
            error = ERROR_UNSUPPORTED;
        }
        if (speed) {
            MT3620_GPT_FIELD_WRITE(gpt4_ctrl, speed, *speed);
        }
        if (restart) {
            error = ERROR_UNSUPPORTED;
        }
        break;

    default:
        error = ERROR_UNSUPPORTED;
        break;
    }

    return error;
}


bool GPT_IsEnabled(GPT *handle)
{
    if (!handle) {
        return false;
    }
    bool enabled;
    if (GPT__GetCtrlValue(
            handle, &enabled, NULL, NULL, NULL) != ERROR_NONE) {
        return false;
    }
    return enabled;
}

static inline int32_t GPT__SetEnabled(
    GPT *handle,
    bool       enabled)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    int32_t error = GPT__SetCtrlValue(handle, &enabled, NULL, NULL, NULL);

    return error;
}

static inline int32_t GPT__ToggleRestart(
    GPT *handle,
    bool       restart)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    int32_t error = GPT__SetCtrlValue(handle, NULL, NULL, NULL, &restart);
    return error;
}

static inline int32_t getAvailableSpeeds(
    const uint32_t id, float *lowSpeed, float *highSpeed)
{
    switch (id) {
    case MT3620_UNIT_GPT0:
    case MT3620_UNIT_GPT1:
    case MT3620_UNIT_GPT2:
        *lowSpeed  = MT3620_GPT_012_LOW_SPEED;
        *highSpeed = MT3620_GPT_012_HIGH_SPEED;
        break;

    case MT3620_UNIT_GPT4:
        *highSpeed = (float)(CPUFreq_Get());
        *lowSpeed  = (*highSpeed) / 2;
        break;

    default:
        return ERROR_UNSUPPORTED;
    }
    return ERROR_NONE;
}

static inline int32_t GPT__GetSpeed3(GPT *handle, float *speedHz)
{
    if (!handle || !speedHz) {
        return ERROR_PARAMETER;
    }

    // Calculate speed (26MHz / osc_cnt_1us)
    uint32_t osc_count_value, speedTemp;
    osc_count_value = MT3620_GPT_FIELD_READ(gpt3_ctrl, osc_cnt_1us) + 1;
    speedTemp       = MT3620_GPT_3_SRC_CLK_HZ / osc_count_value;

    if (speedTemp == 0) {
        return ERROR_GPT_SPEED_INVALID;
    } else {
        *speedHz = speedTemp;
    }
    return ERROR_NONE;
}

int32_t GPT_GetSpeed(GPT* handle, float *speedHz)
{
    if (!handle || !speedHz) {
        return ERROR_PARAMETER;
    }

    if (handle->id == MT3620_UNIT_GPT3) {
        return GPT__GetSpeed3(handle, speedHz);
    }

    // Determine available speeds
    float lowSpeed, highSpeed;
    getAvailableSpeeds(handle->id, &lowSpeed, &highSpeed);

    bool speed_reg;
    int32_t error;
    if ((error = GPT__GetCtrlValue(
            handle, NULL, NULL, &speed_reg, NULL)) == ERROR_NONE) {
        *speedHz = speed_reg ? highSpeed : lowSpeed;
    }
    return error;
}

static inline int32_t GPT__SetSpeed3(
    GPT *handle,
    float      speedHz)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

#ifdef GPT3_ENABLE_NON_STANDARD_SPEED
    uint32_t osc_count_value = MT3620_GPT_3_SRC_CLK_HZ / speedHz;

    if (osc_count_value == 0) {
        return ERROR_GPT_SPEED_INVALID;
    }
    MT3620_GPT_FIELD_WRITE(gpt3_ctrl, osc_cnt_1us, osc_count_value - 1);
#endif // #ifdef GPT3_ENABLE_NON_STANDARD_SPEED

    return ERROR_NONE;
}

int32_t GPT_SetSpeed(GPT *handle, float speedHz)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    if (GPT_IsEnabled(handle)) {
        return ERROR_BUSY;
    }

    if (handle->id == MT3620_UNIT_GPT3) {
        return GPT__SetSpeed3(handle, speedHz);
    }

    float lowSpeed, highSpeed;
    getAvailableSpeeds(handle->id, &lowSpeed, &highSpeed);

    bool registerValue;
    if ((speedHz <= lowSpeed) ||
        ((speedHz - lowSpeed) < (highSpeed - speedHz))) {
        registerValue = false;
    } else {
        registerValue = true;
    }

    int32_t error = GPT__SetCtrlValue(
        handle, NULL, NULL, &registerValue, NULL);
    return error;
}

int32_t GPT_GetMode(
    GPT *handle,
    GPT_Mode  *mode)
{
    if (!handle || !mode) {
        return ERROR_PARAMETER;
    }

    GPT_Mode m;
    int32_t error;
    if ((error = GPT__GetCtrlValue(
        handle, NULL, &m, NULL, NULL)) == ERROR_NONE) {
        *mode = m;
    }
    return error;
}

int32_t GPT_SetMode(GPT *handle, GPT_Mode mode)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    if (mode == GPT_MODE_NONE) {
        return ERROR_NONE;
    }

    return GPT__SetCtrlValue(handle, NULL, &mode, NULL, NULL);
}

uint32_t GPT_GetCount(GPT *handle)
{
    if (!handle || (handle->id > MAX_TIMER_INDEX)) {
        return 0;
    }

    uint32_t count = 0;
    switch (handle->id) {
    case MT3620_UNIT_GPT0:
        count = mt3620_gpt->gpt0_cnt;
        break;
    case MT3620_UNIT_GPT1:
        count = mt3620_gpt->gpt1_cnt;
        break;
    case MT3620_UNIT_GPT2:
        count = mt3620_gpt->gpt2_cnt;
        break;
    case MT3620_UNIT_GPT3:
        count = mt3620_gpt->gpt3_cnt;
        break;
    case MT3620_UNIT_GPT4:
        count = mt3620_gpt->gpt4_cnt;
        break;
    default:
        break;
    }

    return count;
}

uint32_t GPT_GetRunningTime(GPT *handle, GPT_Units units)
{
    if (!handle || (handle->id > MAX_TIMER_INDEX)) {
        return 0;
    }

    float speed;
    if ((GPT_GetSpeed(handle, &speed) != ERROR_NONE) || (speed == 0)) {
        return 0;
    }

    // Get relative count (some timers count backwards, others from non-zero start)
    uint32_t count = GPT_GetCount(handle);
    uint32_t init_count;
    switch (handle->id) {
    case MT3620_UNIT_GPT0:
        init_count = mt3620_gpt->gpt0_icnt;
        count      = init_count - count;
        break;
    case MT3620_UNIT_GPT1:
        init_count = mt3620_gpt->gpt1_icnt;
        count      = init_count - count;
        break;
    case MT3620_UNIT_GPT2:
        // for GPT2, we have to rely on cached initcnt
        init_count = handle->initCnt;
        count      = count - init_count;
        break;
    case MT3620_UNIT_GPT3:
        init_count = mt3620_gpt->gpt3_init;
        count      = count - init_count;
        break;
    case MT3620_UNIT_GPT4:
        init_count = mt3620_gpt->gpt4_init;
        count      = count - init_count;
        break;
    default:
        return 0;
    }

    return ((uint64_t)count * (uint64_t)units) / speed;
}

int32_t GPT_GetNumCycles(GPT* handle, uint32_t* numCycles)
{
    if (!handle || !numCycles) {
        return ERROR_PARAMETER;
    }
    if (handle->id > MAX_TIMER_INDEX) {
        return ERROR_UNSUPPORTED;
    }

    *numCycles = handle->numCycles;
    return ERROR_NONE;
}

static int32_t getGPTIndex(int32_t id)
{
    return id - MT3620_UNIT_GPT0;
}

static void GPT_ToggleInterrupts(GPT *handle, bool enable)
{
    // Just enables the NVIC control, so timer can interrupt processor
    int  irq;
    bool ableToDisable = true;
    switch (handle->id) {
    case MT3620_UNIT_GPT0:
    case MT3620_UNIT_GPT1:
        if (context[getGPTIndex(MT3620_UNIT_GPT0)].open ||
            context[getGPTIndex(MT3620_UNIT_GPT1)].open)
        {
            ableToDisable = false;
        }
        irq = GPT01_IRQ;
        break;
    case MT3620_UNIT_GPT3:
        irq = GPT3_IRQ;
        break;
    default:
        return;
    }

    if (enable) {
        NVIC_EnableIRQ(irq, GPT_PRIORITY);
    } else if (ableToDisable) {
        NVIC_DisableIRQ(irq);
    }
}

GPT* GPT_Open(int32_t id, float speedHz, GPT_Mode mode)
{
    int32_t index = getGPTIndex(id);
    if ((id > MAX_TIMER_INDEX) || (index < 0)) {
        return NULL;
    }

    if (context[index].open) {
        // user already has this handle
        return NULL;
    }

    if (GPT_IsEnabled(&(context[index]))) {
        return NULL;
    }

    context[index].open = true;
    context[index].id   = id;

    // Setup Timer control registers
    if (GPT_SetSpeed(&context[index], speedHz) != ERROR_NONE) {
        context[index].open = false;
        return NULL;
    }
    if (GPT_SetMode(&context[index], mode) != ERROR_NONE) {
        context[index].open = false;
        return NULL;
    }

    context[index].callback  = NULL;
    context[index].numCycles = 0;
    context[index].initCnt   = 0;
    context[index].paused    = false;

    GPT_ToggleInterrupts(&context[index], true);

    return &context[index];
}

void GPT_Close(GPT *handle)
{
    if (!handle || !handle->open) {
        return;
    }

    if (GPT_IsEnabled(handle)) {
        GPT_Stop(handle);
    }

    handle->open = false;

    // Disable interrupts
    GPT_ToggleInterrupts(handle, false);
}


int32_t GPT_GetId(GPT *handle)
{
    if (!handle) {
        return -1;
    }
    return (int32_t)handle->id;
}

int32_t GPT_Stop(GPT *handle)
{
    if (!handle || !handle->open) {
        return ERROR_PARAMETER;
    }

    if (!GPT_IsEnabled(handle)) {
        return ERROR_GPT_NOT_RUNNING;
    }

    // Disable timer
    GPT__SetEnabled(handle, false);

    // Handle different timer types
    switch (handle->id) {
    case MT3620_UNIT_GPT0:
        // Disable interrupt register
        MT3620_GPT_FIELD_WRITE(gpt_ier, gpt0_int_en, false);
        break;
    case MT3620_UNIT_GPT1:
        // Disable interrupt register
        MT3620_GPT_FIELD_WRITE(gpt_ier, gpt1_int_en, false);
        break;
    case MT3620_UNIT_GPT2:
    case MT3620_UNIT_GPT3:
    case MT3620_UNIT_GPT4:
        break;
    default:
        return ERROR_UNSUPPORTED;
    }

    handle->numCycles = 0;

    return ERROR_NONE;
}

int32_t GPT_Pause(GPT *handle)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    if (!GPT_IsEnabled(handle)) {
        return ERROR_GPT_NOT_RUNNING;
    }

    if (handle->paused) {
        return ERROR_GPT_ALREADY_PAUSED;
    }

    handle->initCnt = GPT_GetCount(handle);

    int32_t error;
    if ((error = GPT__SetEnabled(handle, false)) != ERROR_NONE) {
        return error;
    }

    handle->paused = true;
    return ERROR_NONE;
}

int32_t GPT_Resume(GPT *handle)
{
    if (!handle) {
        return ERROR_PARAMETER;
    }

    if (GPT_IsEnabled(handle)) {
        return ERROR_GPT_ALREADY_RUNNING;
    }

    if (!handle->paused) {
        return ERROR_GPT_NOT_PAUSED;
    }

    // Set initial count to cached value
    switch (handle->id) {
    case MT3620_UNIT_GPT0:
        mt3620_gpt->gpt0_icnt = handle->initCnt;
        break;
    case MT3620_UNIT_GPT1:
        mt3620_gpt->gpt1_icnt = handle->initCnt;
        break;
    case MT3620_UNIT_GPT2:
        // GPT2 is different; you have to write to cnt to set init count
        mt3620_gpt->gpt2_cnt  = handle->initCnt;
        break;
    case MT3620_UNIT_GPT3:
        mt3620_gpt->gpt3_init = handle->initCnt;
        break;
    case MT3620_UNIT_GPT4:
        mt3620_gpt->gpt4_init = handle->initCnt;
        break;
    default:
        break;
    }

    int32_t error;
    if ((error = GPT__SetEnabled(handle, true)) != ERROR_NONE) {
        return error;
    }
    handle->paused = false;
    return ERROR_NONE;
}

static inline uint32_t calculateNumCounts(
    uint32_t  timeout,
    float     speed,
    GPT_Units units)
{
    // Calculate nCount required for timeout [units]
    // TODO: add overflow detection here?
    return units > speed ? ((float)timeout * speed) / units : (speed / (float)units) * timeout;

}

int32_t GPT_StartTimeout(
    GPT       *handle,
    uint32_t   timeout,
    GPT_Units  units,
    void       (*callback)(GPT *))
{
    if (!handle || !handle->open) {
        return ERROR_PARAMETER;
    }

    if (GPT_IsEnabled(handle)) {
        return ERROR_GPT_ALREADY_RUNNING;
    }

    // Fail silently if not supported
    GPT__ToggleRestart(handle, true);

    int32_t error;
    float   actualSpeed;
    if ((error = GPT_GetSpeed(handle, &actualSpeed)) != ERROR_NONE) {
        return error;
    }

    uint32_t nCount = calculateNumCounts(timeout, actualSpeed, units);
    if (nCount == 0) {
        // the timeout is smaller than the resolution of the timer
        return ERROR_GPT_TIMEOUT_INVALID;
    }

    // Handle different timer types
    switch (handle->id) {
    case MT3620_UNIT_GPT0:
        // Since 0 & 1 share INT1; we need to enable interrupt register.
        MT3620_GPT_FIELD_WRITE(gpt_isr, gpt0_int, false);
        MT3620_GPT_FIELD_WRITE(gpt_ier, gpt0_int_en, true);
        // Set initial count (0 & 1 count down)
        mt3620_gpt->gpt0_icnt = nCount - 1;
        break;

    case MT3620_UNIT_GPT1:
        // Enable interrupt register
        MT3620_GPT_FIELD_WRITE(gpt_isr, gpt1_int, false);
        MT3620_GPT_FIELD_WRITE(gpt_ier, gpt1_int_en, true);
        // Set initial count
        mt3620_gpt->gpt1_icnt = nCount - 1;
        break;

    case MT3620_UNIT_GPT3:
        mt3620_gpt->gpt3_init   = 0;
        mt3620_gpt->gpt3_expire = nCount - 1;
        MT3620_GPT_FIELD_WRITE(gpt3_ctrl, gpt3_iclr, true);
        break;

    case MT3620_UNIT_GPT2:
    case MT3620_UNIT_GPT4:
        return ERROR_UNSUPPORTED;

    default:
        return ERROR_UNSUPPORTED;
    }

    // Save callback and enable timer
    handle->callback = callback;
    handle->paused   = false;
    GPT__SetEnabled(handle, true);
    return ERROR_NONE;
}

int32_t GPT_Start_Freerun(GPT *handle)
{
    if (!handle || !handle->open) {
        return ERROR_PARAMETER;
    }

    if (GPT_IsEnabled(handle)) {
        return ERROR_GPT_ALREADY_RUNNING;
    }

    // Set timer ctrl to run maximum length (if not free running timer)
    // Handle different timer types
    switch (handle->id) {
    case MT3620_UNIT_GPT0:
        // Disable interrupts
        MT3620_GPT_FIELD_WRITE(gpt_isr, gpt0_int, false);
        MT3620_GPT_FIELD_WRITE(gpt_ier, gpt0_int_en, false);
        // Set initial count (0 & 1 count down)
        mt3620_gpt->gpt0_icnt = MT3620_GPT_MAX_COUNT;
        break;

    case MT3620_UNIT_GPT1:
        // Disable interrupts
        MT3620_GPT_FIELD_WRITE(gpt_isr, gpt1_int, false);
        MT3620_GPT_FIELD_WRITE(gpt_ier, gpt1_int_en, false);
        // Set initial count
        mt3620_gpt->gpt1_icnt = MT3620_GPT_MAX_COUNT;
        break;

    case MT3620_UNIT_GPT2:
        // GPT2 is different; you have to write to the cnt reg
        mt3620_gpt->gpt2_cnt = 0;
        // Cache initCnt since there is no dedicated register.
        handle->initCnt      = 0;
        // NB: You shouldn't read the GPT2 cnt register for 3T 32Hz cycles
        //     after setting it; else the change doesn't stick.
        break;

    case MT3620_UNIT_GPT3:
        // GPT3 counts from init to expire
        mt3620_gpt->gpt3_init   = 0;
        mt3620_gpt->gpt3_expire = MT3620_GPT_MAX_COUNT;
        MT3620_GPT_FIELD_WRITE(gpt3_ctrl, gpt3_iclr, true);
        break;

    case MT3620_UNIT_GPT4:
        mt3620_gpt->gpt4_init = 0;
        break;

    default:
        break;
    }

    // Enable timer
    handle->paused = false;
    GPT__SetEnabled(handle, true);
    return ERROR_NONE;
}

int32_t GPT_WaitTimer_Blocking(
    GPT       *handle,
    uint32_t   timeout,
    GPT_Units  units)
{
    if (!handle || !handle->open) {
        return ERROR_PARAMETER;
    }

    if (GPT_IsEnabled(handle)) {
        return ERROR_GPT_ALREADY_RUNNING;
    }

    int32_t error;
    float   actualSpeed;
    if ((error = GPT_GetSpeed(handle, &actualSpeed)) != ERROR_NONE) {
        return error;
    }

    uint32_t nCount = calculateNumCounts(timeout, actualSpeed, units);

    if (nCount == 0) {
        // The timeout is smaller than the resolution of the timer.
        return ERROR_GPT_TIMEOUT_INVALID;
    }

    // This is required, as the initcnt isn't propogated immediately to cnt on enable.
    uint32_t startCount = GPT_GetCount(handle);

    // Start timeout and wait for it, don't re-set the speed
    if ((error = GPT_Start_Freerun(handle)) != ERROR_NONE) {
        GPT_Stop(handle);
        return error;
    }
    uint32_t count;
    switch (handle->id) {
    case MT3620_UNIT_GPT0:
    case MT3620_UNIT_GPT1:
        // Timer decrements to 0.
        while (((count = GPT_GetCount(handle)) > (MT3620_GPT_MAX_COUNT - nCount)) ||
            (count == startCount)) { }
        break;

    case MT3620_UNIT_GPT2:
    case MT3620_UNIT_GPT3:
    case MT3620_UNIT_GPT4:
        // Timer increments from 0 (we set init to 0 for GPT3).
        while (GPT_GetCount(handle) < nCount) {}
        break;

    default:
        return ERROR_UNSUPPORTED;
    }

    return GPT_Stop(handle);
}

static void GPT_IRQ(GPT *handle)
{
    if (!handle || !handle->open) {
        return;
    }

    if (handle->callback) {
        handle->callback(handle);
    }

    handle->numCycles++;
}

/// <summary>
/// Interrupt function for INT1 (used by GPT0 and GPT1)
/// </summary>
void gpt_int_b(void)
{
    // Get the timer that's triggered the interrupt
    bool gpt0_int = MT3620_GPT_FIELD_READ(gpt_isr, gpt0_int);
    bool gpt1_int = MT3620_GPT_FIELD_READ(gpt_isr, gpt1_int);

    // Reset status flags
    MT3620_GPT_FIELD_WRITE(gpt_isr, gpt0_int, false);
    MT3620_GPT_FIELD_WRITE(gpt_isr, gpt1_int, false);

    // Trigger user callbacks
    if (gpt0_int) {
        GPT_IRQ(&context[0]);
    }
    if (gpt1_int) {
        GPT_IRQ(&context[1]);
    }
}


void gpt3_int_b(void)
{
    GPT *handle = &context[3];
    GPT_IRQ(handle);
    if (handle->mode == GPT_MODE_REPEAT) {
        GPT__SetEnabled(handle, false);
        GPT__SetEnabled(handle, true);
    }
}


///------------------------Test helpers--------------------------

void GPT_GetTestSpeeds(
    GPT            *handle,
    GPT_TestSpeeds *testSpeeds)
{
    if (!handle) {
        return;
    }

    switch (handle->id) {
    case MT3620_UNIT_GPT0:
    case MT3620_UNIT_GPT1:
    case MT3620_UNIT_GPT2:
        // 2 speed timer
        testSpeeds->speeds[0] = MT3620_GPT_012_LOW_SPEED;
        testSpeeds->speeds[1] = MT3620_GPT_012_HIGH_SPEED;
        testSpeeds->count = 2;
        break;

    case MT3620_UNIT_GPT3:
        // multi speed timer
        testSpeeds->speeds[0] = MT3620_GPT_3_LOW_SPEED;
        testSpeeds->speeds[1] = MT3620_GPT_3_HIGH_SPEED / 8;
        testSpeeds->speeds[2] = MT3620_GPT_3_HIGH_SPEED / 4;
        testSpeeds->speeds[3] = MT3620_GPT_3_HIGH_SPEED / 2;
        testSpeeds->speeds[4] = MT3620_GPT_3_HIGH_SPEED;
        testSpeeds->count = 5;
        break;

    case MT3620_UNIT_GPT4:
        // 2 speed timer
        testSpeeds->speeds[1] = CPUFreq_Get();
        testSpeeds->speeds[0] = testSpeeds->speeds[1] / 2;
        testSpeeds->count = 2;
        break;

    default:
        break;
    }
}
