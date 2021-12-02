/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef AZURE_SPHERE_MBOX_H_
#define AZURE_SPHERE_MBOX_H_

#include "Common.h"
#include "Platform.h"

#include <stdbool.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/// <summary>Returned when user tries to write to FIFO and no space.</summary>
#define ERROR_MBOX_FIFO_INSUFFICIENT_SPACE  (ERROR_SPECIFIC - 1)

/// <summary>Returned when aquire or release fail.</summary>
#define ERROR_MBOX_SEMAPHORE_REQUEST_DENIED (ERROR_SPECIFIC - 2)

/// <summary>Returned if user tries to use M4-M4 semaphore (non-existant).</summary>
#define ERROR_MBOX_SEMAPHORE_NOT_PRESENT    (ERROR_SPECIFIC - 3)

/// <summary>
/// <para>Opaque MBox handle (contains state for all MBox functionality).</para>
/// </summary>
typedef struct MBox MBox;

typedef enum {
    MBOX_FIFO_STATE_NOT_FULL,
    MBOX_FIFO_STATE_NOT_EMPTY,
} MBox_FIFO_State;

/// <summary>
/// <para>Acquires a handle to the mailbox's context and defines
/// mailbox callback functions.</para>
/// </summary>
/// <param name="unit">Which Mailbox to open (M4-M4 or M4-A7).</param>
/// <param name="rx_cb">Callback triggered when other core writes to MBox FIFO.
/// If null, interrupt disabled. MBox_FIFO_ReadSync can be used to read FIFO
/// directly.</param>
/// <param name="tx_confirmed_cb">Callback triggered when other core reads
/// from FIFO. If null, interrupt disabled.</param>
/// <param name="fifo_state_cb">Callback triggered when not-empty|full
/// thresholds are passed on the read|write FIFOs respectively. If null,
/// interrupt disabled.</param>
/// <param name="user_data">Pointer to user data used in cbs.</param>
/// <param name="non_full_threshold">Configures non-full interrupt on write
/// FIFO. Set to -1 to leave unconfigured (at reset setting)</param>
/// <param name="non_empty_threshold">Configures non-empty interrupt on read
/// FIFO. Set to -1 to leave unconfigured (at reset setting)</param>
/// <returns>A handle to a MBox context or NULL on failure.</returns>
MBox* MBox_FIFO_Open(
    Platform_Unit  unit,
    void          (*rx_cb)               (void*),
    void          (*tx_confirmed_cb)     (void*),
    void          (*fifo_state_change_cb)(void*, MBox_FIFO_State state),
    void          *user_data,
    int8_t         non_full_threshold,
    int8_t         non_empty_threshold);

/// <summary>
/// <para>Releases a mailbox handle. Resets M4 MBox registers.</para>
/// </summary>
/// <param name="handle">The MBox handle which is to be released.</param>
void MBox_FIFO_Close(MBox *handle);

/// <summary>
/// <para>Resets MBox either on just m4 side, or on both a7 and m4 sides</para>
/// </summary>
/// <param name="handle">The MBox handle which is to be released.</param>
/// <param name="both">Whether to reset bidirectionally or not.</param>
void MBox_FIFO_Reset(MBox *handle, bool both);

/// <summary>
/// <para>Write to Mbox FIFO. If configured, the interrupts to expect are
/// not_empty and read (if the corresponding read happens on the other core)
/// </para>
/// </summary>
/// <param name="handle">MBox to write to.</param>
/// <param name="cmd">CMD buffer (required).</param>
/// <param name="data">DATA buffer (optional).</param>
/// <param name="length">Length of CMD (and DATA) buffer in elements (each
/// element 32 bits). MBOX_FIFO_COUNT_MAX is limit.</param>
/// <returns>ERROR_NONE or ERROR_PARAMETER.</returns>
int32_t MBox_FIFO_Write(
    MBox           *handle,
    const uint32_t *cmd,
    const uint32_t *data,
    uintptr_t       length);

/// <summary>
/// <para>Synchronous read, blocks until `length` elements have been read.
/// Can hang indefinitely, if timeouts required, specify a rx_cb in the Open
/// method and do reads asynchronously.
/// </para>
/// </summary>
/// <param name="handle">MBox to read from.</param>
/// <param name="cmd">CMD buffer to be read into (required).</param>
/// <param name="data">DATA buffer to be read into (optional).</param>
/// <param name="length">Length of CMD (and DATA) buffer in elements (each
/// element 32 bits). MBOX_FIFO_COUNT_MAX is limit.</param>
/// <returns>ERROR_NONE or ERROR_PARAMETER.</returns>
int32_t MBox_FIFO_ReadSync(
    MBox      *handle,
    uint32_t  *cmd,
    uint32_t  *data,
    uintptr_t  length);

/// <summary>
/// <para>Check number of elements to be read by partner core.</para>
/// </summary>
/// <param name="handle">MBox to interrogate.</param>
/// <returns>Number of elements that are available for partner core to read.
/// </returns>
uintptr_t MBox_FIFO_Writes_Pending(const MBox *handle);

/// <summary>
/// <para>Check number of elements available to read on FIFO.</para>
/// </summary>
/// <param name="handle">MBox to interrogate.</param>
/// <returns>Number of elements that can be read.</returns>
uintptr_t MBox_FIFO_Reads_Available(const MBox *handle);


/// <summary>
/// <para>Acquire semaphore (useful for implementing locks on shared memory).
/// </para>
/// </summary>
/// <param name="handle">MBox to interrogate.</param>
/// <returns>ERROR_NONE, or ERROR_MBOX_SEMAPHORE_REQUEST_DENIED
/// or ERROR_MBOX_SEMAPHORE_NOT_PRESENT on failure.</returns>
int32_t MBox_Semaphore_Acquire(MBox* handle);

/// <summary>
/// <para>Release semaphore.
/// </para>
/// </summary>
/// <param name="handle">MBox to interrogate.</param>
/// <returns>ERROR_NONE, or ERROR_MBOX_SEMAPHORE_REQUEST_DENIED
/// or ERROR_MBOX_SEMAPHORE_NOT_PRESENT on failure.</returns>
int32_t MBox_Semaphore_Release(MBox* handle);


#define MBOX_SW_INT_PORT_COUNT 8

/// <summary>
/// <para>Allows user to define interrupts to enable, and assign a callback
/// to be called when the other core triggers one.
/// </para>
/// </summary>
/// <param name="handle">MBox to configure.</param>
/// <param name="int_enable_flags">Defines interrupt ports to be enabled.
/// </param>
/// <param name="int_trigger_cb">Callback to call when interrupt triggered
/// by other core.</param>
/// <returns>ERROR_NONE, or ERROR_PARAMETER.</returns>
int32_t MBox_SW_Interrupt_Setup(
    MBox    *handle,
    uint8_t  int_enable_flags,
    void    (*sw_int_cb)(void*, uint8_t port));

/// <summary>
/// <para>Removes all config from MBox_SW_Interrupt_Setup.</para>
/// </summary>
/// <param name="handle">MBox to deconfigure.</param>
void MBox_SW_Interrupt_Teardown(MBox *handle);

/// <summary>
/// <para>Fire SW interrupt #`port`.</para>
/// </summary>
/// <param name="handle">MBox to fire interrupt on.</param>
/// <param name="port">Interrupt to trigger (0-7).</param>
/// <returns>ERROR_NONE, or ERROR_PARAMETER.</returns>
int32_t MBox_SW_Interrupt_Trigger(MBox *handle, uint8_t port);


#ifdef __cplusplus
}
#endif

#endif // #ifndef AZURE_SPHERE_MBOX_H_
