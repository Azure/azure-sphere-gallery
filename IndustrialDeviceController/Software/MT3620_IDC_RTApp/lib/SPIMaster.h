/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef AZURE_SPHERE_SPIMASTER_H_
#define AZURE_SPHERE_SPIMASTER_H_

#include "Common.h"
#include "Platform.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
 extern "C" {
#endif

/// <summary>SPI specific errors.</summary>
#define ERROR_SPI_MODE            (ERROR_SPECIFIC - 1)
#define ERROR_SPI_FRAME_FORMAT    (ERROR_SPECIFIC - 2)
#define ERROR_SPI_DATA_BITS       (ERROR_SPECIFIC - 3)
#define ERROR_SPI_BIT_ORDER       (ERROR_SPECIFIC - 4)
#define ERROR_SPI_CS_SELECT       (ERROR_SPECIFIC - 5)
#define ERROR_SPI_TRANSFER_FAIL   (ERROR_SPECIFIC - 6)
#define ERROR_SPI_TRANSFER_CANCEL (ERROR_SPECIFIC - 7)

/// <summary>SPI Master opaque handle.</summary>
typedef struct SPIMaster SPIMaster;

/// <summary>SPI transfer entry, used for queueing multiple transfers.</summary>
typedef struct {
    /// <summary>
    /// <para>Pointer to data to be transmitted.</para>
    /// <para>The writeData is a variable for transfering data to the SDOR buffer.</para>
    /// </summary>
    const void *writeData;
    /// <summary>
    /// <para>Pointer to buffer where received data will be written.</para>
    /// <para>The readData is a variable for receiving data from the SDIR buffer.</para>
    /// </summary>
    void *readData;
    /// <summary>
    /// <para>Length of data to be transmitted or received.</para>
    /// </summary>
    uintptr_t length;
} SPITransfer;

/// <summary>
/// <para>Sets the subordinate device select channel.</para>
/// </summary>
/// <param name="handle">SPI handle to be configured.</param>
/// <param name="csLine">Sets the CS line to be used for hardware chip-select functionality.
/// Note: If the value is valid, then this function also enables the CS line.</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
int32_t SPIMaster_Select(SPIMaster *handle, unsigned csLine);

/// <summary>
/// <para>Allows user to enable/disable hardware chip-select functionality, enabling
/// resets the csLine to the value it had prior to disabling (if csCallback is not in use).</para>
/// </summary>
/// <param name="handle">SPI handle to be configured.</param>
/// <param name="enable">Whether to enable the csLine</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
int32_t SPIMaster_SelectEnable(SPIMaster *handle, bool enable);

/// <summary>
/// <para>Sets callback alternative to HW chip select; allowing for GPIO CS. This allows the user
/// to work-around the limitations of hardware chip-select functionality.</para>
/// </summary>
/// <param name="handle">SPI handle to update.</param>
/// <param name="selectLineCallback">Callback to associate with CS - called when transaction starts
/// with select=true and when transaction ends or is cancelled with select=false. Note that SPI
/// subordinate devices expect the line to go to logic low (0) for select, and high (1) for
/// unselected. Can be NULL - which means that the callback will be unset. Note: If the value is
/// a valid callback, then this function also enables the CS line, otherwise it's disabled.</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
int32_t SPIMaster_SetSelectLineCallback(
    SPIMaster *handle,
    void      (*csCallback)(SPIMaster *handle, bool select));

/// <summary>
/// <para>Sets the configuration parameters for the SPI transaction.</para>
/// </summary>
/// <param name="handle">SPI handle to be configured.</param>
/// <param name="cpol">SPI clock polarity.</param>
/// <param name="cpha">SPI clock phase.</param>
/// <param name="busSpeed">Selects the closest compatible baud rate below this value.</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
int32_t SPIMaster_Configure(SPIMaster *handle, bool cpol, bool cpha, uint32_t busSpeed);

/// <summary>
/// <para>Enable or disable DMA acceleration for the SPI device.</para>
/// </summary>
/// <param name="handle">SPI handle for which DMA will be configured.</param>
/// <param name="enable">Whether the DMA unit will be enabled or not.</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
int32_t SPIMaster_DMAEnable(SPIMaster *handle, bool enable);

/// <summary>
/// <para>Configures the drive strength on the SPI interface provided.</para>
/// <para>This should only be used if glitches or faults are seen on the SPI line as high values
/// may cause EMI issues.</para>
/// </summary>
/// <param name="handle">Selects the SPI handle to configure the drive strength of.</param>
/// <param name="drive">Drive strength in milliamps</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
int32_t SPIMaster_ConfigureDriveStrength(SPIMaster *handle, unsigned drive);

/// <summary>
/// <para>Initialize the SPIMaster handle and return a pointer to it.</para>
/// </summary>
/// <param name="unit">SPI HW unit to initialize and acquire a handle for.</param>
/// <returns>A handle of an SPI interface or NULL on failure.</returns>
SPIMaster *SPIMaster_Open(Platform_Unit unit);

/// <summary>
/// <para>Cleanup and close any outstanding transactions and release the Platform unit for a
/// subsequent Open().</para>
/// </summary>
/// <param name="handle">SPI handle to be closed.</param>
void SPIMaster_Close(SPIMaster *handle);

/// <summary>
/// <para>Executes a sequence of asynchronous SPI transactions
/// (Read, Write or WriteThenRead) on the interface provided.</para>
/// </summary>
/// <param name="handle">SPI handle to perform the transfer on.</param>
/// <param name="transfer">A pointer to the base of an array of transfers, for more information
/// look at <see cref="SPITransfer"/>.</param>
/// <param name="count">The total number of transfers in the transfer array.</param>
/// <param name="callback">Callback called once the transfer is completed.</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
int32_t SPIMaster_TransferSequentialAsync(
    SPIMaster *handle, SPITransfer *transfer,
    uint32_t count, void (*callback)(int32_t status, uintptr_t data_count));

/// <summary>
/// <para>Identical to SPIMaster_TransferSequentialAsync, but allows for user
/// to provide pointer to data that can be accessed on transfer completion
/// or cancelation. Note that this callback happens within an IRQ, so if there
/// is significant computation, it might be best to defer execution.</para>
/// </summary>
/// <param name="handle">SPI handle to perform the transfer on.</param>
/// <param name="transfer">A pointer to the base of an array of transfers, for more information
/// look at <see cref="SPITransfer"/>.</param>
/// <param name="count">The total number of transfers in the transfer array.</param>
/// <param name="callback">Callback called once the transfer is completed.</param>
/// <param name="userData">Data available in callback.</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
int32_t SPIMaster_TransferSequentialAsync_UserData(
    SPIMaster *handle, SPITransfer *transfer,
    uint32_t count, void (*callback)(
        int32_t status, uintptr_t data_count, void *userData), void *userData);

/// <summary>
/// <para>Cancels the ongoing transfer.</para>
/// </summary>
/// <param name="handle">SPI handle to cancel transfer on.</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
int32_t SPIMaster_TransferCancel(SPIMaster *handle);

/// <summary>
/// <para>Executes a single write operation on the SPI interface provided.</para>
/// </summary>
/// <param name="handle">SPI handle to perform the transfer on.</param>
/// <param name="data">A pointer to the data to be written.</param>
/// <param name="length">The length of the data to be written.</param>
/// <param name="callback">Callback called once the transfer is completed.</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
static inline int32_t SPIMaster_WriteAsync(
    SPIMaster *handle, const void *data,
    uintptr_t length, void (*callback)(int32_t status, uintptr_t dataCount));

/// <summary>
/// <para>Executes a single read operation on the SPI interface provided.</para>
/// </summary>
/// <param name="handle">Selects the SPI handle to perform the transfer on.</param>
/// <param name="data">A pointer to the data to be read.</param>
/// <param name="length">The length of the data to be read.</param>
/// <param name="callback">Callback called once the transfer is completed.</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
static inline int32_t SPIMaster_ReadAsync(
    SPIMaster *handle, void *data,
    uintptr_t length, void (*callback)(int32_t status, uintptr_t dataCount));

/// <summary>
/// <para>Executes back-to-back write then read operations on the SPI interface provided.</para>
/// </summary>
/// <param name="handle">SPI handle to perform the transfer on.</param>
/// <param name="writeData">A pointer to the data to be written</param>
/// <param name="writeLength">The length of the data to be written.</param>
/// <param name = "readData">A pointer to the data to be read.</param>
/// <param name="readLength">The length of the data to be read.</param>
/// <param name="callback">Callback called once the transfer is completed.</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
static inline int32_t SPIMaster_WriteThenReadAsync (
    SPIMaster *handle, const void *writeData,
    uintptr_t writeLength, void *readData, uintptr_t readLength,
    void (*callback)(int32_t status, uintptr_t dataCount));

/// <summary>
/// <para>Executes a sequence of SPI operations on the interface provided.</para>
/// <para>This is a synchronous wrapper around
/// <see cref="SPIMaster_TransferSequentialAsync"/>.</para>
/// </summary>
/// <param name="handle">SPI handle to perform the transfer on.</param>
/// <param name="transfer">A pointer to the base of an array of transfers, for more information
/// look at <see cref="SPITransfer"/>.</param>
/// <param name="count">Number of sequential SPI transactions.</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
int32_t  SPIMaster_TransferSequentialSync(
    SPIMaster *handle, SPITransfer *transfer, uint32_t count);

/// <summary>
/// <para>Executes a single write operation on the SPI interface provided.</para>
/// <para>This is a synchronous wrapper around <see cref="SPIMaster_WriteAsync"/>.</para>
/// </summary>
/// <param name="handle">Selects the SPI handle to perform the transfer on.</param>
/// <param name="data">A pointer to the data to be written.</param>
/// <param name="length">The length of the data to be written.</param>
/// <returns>Returns ERROR_NONE if successful, else returns an error code.</returns>
static inline int32_t SPIMaster_WriteSync(
    SPIMaster *handle, const void *data, uintptr_t length);

/// <summary>
/// <para>Executes a single read operation on the SPI interface provided.</para>
/// <para>This is a synchronous wrapper around <see cref="SPIMaster_ReadAsync"/>.</para>
/// </summary>
/// <param name="handle">Selects the SPI handle to perform the transfer on.</param>
/// <param name="data">A pointer to the data to be read.</param>
/// <param name="length">The length of the data to be read.</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
static inline int32_t SPIMaster_ReadSync(
    SPIMaster *handle, void *data, uintptr_t length);

/// <summary>
/// <para>Executes back-to-back write then read operations on the SPI interface provided.</para>
/// <para>This is a synchronous wrapper around <see cref="SPIMaster_WriteThenReadAsync"/>.</para>
/// </summary>
/// <param name="handle">Selects the SPI handle to perform the transfer on.</param>
/// <param name="writeData">A pointer to the data to be written.</param>
/// <param name="writeLength">The length of the data to be written.</param>
/// <param name="readData">A pointer to the data to be read.</param>
/// <param name="readLength">The length of the data to be read.</param>
/// <returns>Returns ERROR_NONE on success, or an error code on failure.</returns>
static inline int32_t SPIMaster_WriteThenReadSync(
    SPIMaster *handle, const void *writeData,
    uintptr_t writeLength, void *readData, uintptr_t readLength);

static inline int32_t SPIMaster_WriteAsync(
    SPIMaster *handle, const void *data, uintptr_t length,
    void (*callback)(int32_t status, uintptr_t dataCount))
{
    SPITransfer transfer = {
        .writeData = data,
        .readData  = NULL,
        .length    = length,
    };
    return SPIMaster_TransferSequentialAsync(handle, &transfer, 1, callback);
}

static inline int32_t SPIMaster_ReadAsync(
    SPIMaster *handle, void *data, uintptr_t length,
    void (*callback)(int32_t status, uintptr_t dataCount))
{
    SPITransfer transfer = {
        .writeData = NULL,
        .readData  = data,
        .length    = length,
    };
    return SPIMaster_TransferSequentialAsync(handle, &transfer, 1, callback);
}

static inline int32_t  SPIMaster_WriteThenReadAsync(
    SPIMaster *handle, const void *writeData, uintptr_t writeLength,
    void *readData, uintptr_t readLength, void (*callback)(int32_t status, uintptr_t dataCount))
{
    SPITransfer transfer[2] = {
        {.writeData = writeData, .readData = NULL    , .length = writeLength },
        {.writeData = NULL     , .readData = readData, .length = readLength  },
    };
    return SPIMaster_TransferSequentialAsync(handle, transfer, 2, callback);
}

static inline int32_t SPIMaster_WriteSync(SPIMaster *handle, const void *data, uintptr_t length)
{
    SPITransfer transfer = {
        .writeData = data,
        .readData  = NULL,
        .length    = length,
    };
    return SPIMaster_TransferSequentialSync(handle, &transfer, 1);
}

static inline int32_t SPIMaster_ReadSync(SPIMaster *handle, void *data, uintptr_t length)
{
    SPITransfer transfer = {
        .writeData = NULL,
        .readData  = data,
        .length    = length,
    };
    return SPIMaster_TransferSequentialSync(handle, &transfer, 1);
}

static inline int32_t  SPIMaster_WriteThenReadSync(
    SPIMaster *handle, const void *writeData, uintptr_t writeLength, void *readData, uintptr_t readLength)
{
    SPITransfer transfer[2] = {
        { .writeData = writeData, .readData = NULL ,    .length = writeLength },
        { .writeData = NULL ,     .readData = readData, .length = readLength },
    };
    return SPIMaster_TransferSequentialSync(handle, transfer, 2);
}

#ifdef __cplusplus
 }
#endif

#endif // #ifndef AZURE_SPHERE_SPIMASTER_H_
