/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef AZURE_SPHERE_I2CMASTER_H_
#define AZURE_SPHERE_I2CMASTER_H_

#include "Common.h"
#include "Platform.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

/// <summary>Returned when an I2C transfer fails to receive an ACK.</summary>
#define ERROR_I2C_ADDRESS_NACK        (ERROR_SPECIFIC - 1)

/// <summary>Returned when an I2C transfer fails arbitration on a multi-master bus.</summary>
#define ERROR_I2C_ARBITRATION_LOST    (ERROR_SPECIFIC - 2)

/// <summary>Returned when an I2C transfer fails to complete.</summary>
#define ERROR_I2C_TRANSFER_INCOMPLETE (ERROR_SPECIFIC - 3)

/// <summary>Opaque I2C Master handle.</summary>
typedef struct I2CMaster I2CMaster;

/// <summary>Enum for standard I2C bus speeds, obviously any integer can be used provided it
/// remains within the hardware capabilities of the target device.</summary>
typedef enum {
    I2C_BUS_SPEED_LOW       =   10000,
    I2C_BUS_SPEED_STANDARD  =  100000,
    I2C_BUS_SPEED_FAST      =  400000,
    I2C_BUS_SPEED_FAST_PLUS = 1000000,
    I2C_BUS_SPEED_HIGH      = 3400000,
} I2C_BusSpeed;

/// <summary>I2C transfer entry, used for queueing multiple transfers</summary>
typedef struct {
    /// <summary>
    /// <para>Pointer to data to be transmitted.</para>
    /// <para>Must not be set at the same time as readData.</para>
    /// <para>Must reside in DMA bus accessible RAM for large transfers.</para>
    /// </summary>
    const void *writeData;
    /// <summary>
    /// <para>Pointer to buffer where received data will be written.</para>
    /// <para>Must not be set at the same time as writeData.</para>
    /// <para>Must reside in DMA bus accessible RAM for large transfers.</para>
    /// </summary>
    void *readData;
    /// <summary>Length of data to be transmitted or received.</summary>
    uintptr_t length;
} I2C_Transfer;

/// <summary>
/// <para>Acquires a handle before using a given I2C interface.</para>
/// </summary>
/// <param name="unit">Which I2C interface to initialize and acquire a handle for.</param>
/// <returns>A handle to an I2C interface or NULL on failure.</returns>
I2CMaster *I2CMaster_Open(Platform_Unit unit);

/// <summary>
/// <para>Releases a handle once it's finished using a given I2C interface.
/// Once released the handle is free to be opened again.</para>
/// </summary>
/// <param name="handle">The I2C handle which is to be released.</param>
void I2CMaster_Close(I2CMaster *handle);

/// <summary>Sets the speed of the I2C interface to the closest
/// supported hardware speed.</summary>
/// <param name="handle">The I2C handle to set the speed of.</param>
/// <param name="speed">The approximate bus speed to be set, may be an integer or
/// an enum value provided in <see cref="I2C_BusSpeed"/>.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t I2CMaster_SetBusSpeed(I2CMaster *handle, I2C_BusSpeed speed);

/// <summary>Calculates the exact speed of the I2C interface.</summary>
/// <param name="handle">The I2C handle to calculate the speed of.</param>
/// <param name="speed">A pointer to the variable where the calculated speed will be written
/// or NULL. Only written to when the function returns ERROR_NONE.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t I2CMaster_GetBusSpeed(const I2CMaster *handle, I2C_BusSpeed *speed);

/// <summary>
/// <para>Executes a queue of I2C operations on the interface provided.</para>
/// <para>The maximum queue length and transfer sizes are determined by the target hardware.</para>
/// </summary>
/// <param name="handle">The I2C handle to perform the transfer on.</param>
/// <param name="address">The subordinate device address of the target I2C device.</param>
/// <param name="transfer">A pointer to the base of an array of transfers, for more information
/// look at <see cref="I2C_Transfer"/>.</param>
/// <param name="count">The total number of transfers in the transfer array.</param>
/// <param name="callback">A pointer to a function which will be called once the transfer is
/// completed.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t I2CMaster_TransferSequentialAsync(
    I2CMaster *handle, uint16_t address,
    const I2C_Transfer *transfer, uint32_t count,
    void (*callback)(int32_t status, uintptr_t count));

/// <summary>
/// <para>Identical to I2CMaster_TransferSequentialAsync, but with facility for
/// user to provide pointer to data that can be accessed in the completetion
/// callback. Note that this callback happens in an interrupt, so if extensive
/// computation is required, it might make sense to defer execution.</para>
/// <para>The maximum queue length and transfer sizes are determined by the target hardware.</para>
/// </summary>
/// <param name="handle">The I2C handle to perform the transfer on.</param>
/// <param name="address">The subordinate device address of the target I2C device.</param>
/// <param name="transfer">A pointer to the base of an array of transfers, for more information
/// look at <see cref="I2C_Transfer"/>.</param>
/// <param name="count">The total number of transfers in the transfer array.</param>
/// <param name="callback">A pointer to a function which will be called once the transfer is
/// completed.</param>
/// <param name="userData">Pointer to data that can be accessed in completion callback.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t I2CMaster_TransferSequentialAsync_UserData(
    I2CMaster *handle, uint16_t address,
    const I2C_Transfer *transfer, uint32_t count,
    void (*callback)(int32_t status, uintptr_t count, void* userData),
    void *userData);

/// <summary>
/// <para>Executes a back-to-back write then read operations on the I2C interface provided.</para>
/// </summary>
/// <param name="handle">The I2C handle to perform the transfer on.</param>
/// <param name="address">The subordinate device address of the target I2C device.</param>
/// <param name="writeData">A pointer to the data to be written.</param>
/// <param name="writeLength">The length of the data to be written.</param>
/// <param name="readData">A pointer to the data to be read.</param>
/// <param name="readLength">The length of the data to be read.</param>
/// <param name="callback">A pointer to a function which will be called once the transfer is
/// completed.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
static inline int32_t I2CMaster_WriteThenReadAsync(
    I2CMaster *handle, uint16_t address,
    const void *writeData, uintptr_t writeLength,
    void *readData, uintptr_t readLength,
    void (*callback)(int32_t status, uintptr_t count));

/// <summary>
/// <para>Executes a single write operation on the I2C interface provided.</para>
/// </summary>
/// <param name="handle">The I2C handle to perform the transfer on.</param>
/// <param name="address">The subordinate device address of the target I2C device.</param>
/// <param name="data">A pointer to the data to be written.</param>
/// <param name="length">The length of the data to be written.</param>
/// <param name="callback">A pointer to a function which will be called once the transfer is
/// completed.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
static inline int32_t I2CMaster_WriteAsync(
    I2CMaster *handle, uint16_t address,
    const void *data, uintptr_t length,
    void (*callback)(int32_t status, uintptr_t count));

/// <summary>
/// <para>Executes a single read operation on the I2C interface provided.</para>
/// </summary>
/// <param name="handle">The I2C handle to perform the transfer on.</param>
/// <param name="address">The subordinate device address of the target I2C device.</param>
/// <param name="data">A pointer to the data to be read.</param>
/// <param name="length">The length of the data to be read.</param>
/// <param name="callback">A pointer to a function which will be called once the transfer is
/// completed.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
static inline int32_t I2CMaster_ReadAsync(
    I2CMaster *handle, uint16_t address,
    void *data, uintptr_t length,
    void (*callback)(int32_t status, uintptr_t count));

/// <summary>
/// <para>Executes a queue of I2C operations on the interface provided.</para>
/// <para>The maximum queue length and transfer sizes are determined by the target hardware.</para>
/// <para>This is a synchronous wrapper around <see cref="I2CMaster_TransferSequentialAsync"/>.</para>
/// </summary>
/// <param name="handle">The I2C handle to perform the transfer on.</param>
/// <param name="address">The subordinate device address of the target I2C device.</param>
/// <param name="transfer">A pointer to the base of an array of transfers, for more information
/// look at <see cref="I2C_Transfer"/>.</param>
/// <param name="count">The total number of transfers in the transfer array.</param>
/// completed.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
int32_t I2CMaster_TransferSequentialSync(
    I2CMaster *handle, uint16_t address,
    const I2C_Transfer *transfer, uint32_t count);

/// <summary>
/// <para>Executes a back-to-back write then read operations on the I2C interface provided.</para>
/// <para>This is a synchronous wrapper around <see cref="I2CMaster_WriteThenReadAsync"/>.</para>
/// </summary>
/// <param name="handle">The I2C handle to perform the transfer on.</param>
/// <param name="address">The subordinate device address of the target I2C device.</param>
/// <param name="writeData">A pointer to the data to be written.</param>
/// <param name="writeLength">The length of the data to be written.</param>
/// <param name="readData">A pointer to the data to be read.</param>
/// <param name="readLength">The length of the data to be read.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
static inline int32_t I2CMaster_WriteThenReadSync(
    I2CMaster *handle, uint16_t address,
    const void *writeData, uintptr_t writeLength,
    void *readData, uintptr_t readLength);

/// <summary>
/// <para>Executes a single write operation on the I2C interface provided.</para>
/// <para>This is a synchronous wrapper around <see cref="I2CMaster_WriteAsync"/>.</para>
/// </summary>
/// <param name="handle">The I2C handle to perform the transfer on.</param>
/// <param name="address">The subordinate device address of the target I2C device.</param>
/// <param name="data">A pointer to the data to be written.</param>
/// <param name="length">The length of the data to be written.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
static inline int32_t I2CMaster_WriteSync(
    I2CMaster *handle, uint16_t address,
    const void *data, uintptr_t length);

/// <summary>
/// <para>Executes a single read operation on the I2C interface provided.</para>
/// <para>This is a synchronous wrapper around <see cref="I2CMaster_ReadAsync"/>.</para>
/// </summary>
/// <param name="handle">The I2C handle to perform the transfer on.</param>
/// <param name="address">The subordinate device address of the target I2C device.</param>
/// <param name="data">A pointer to the data to be read.</param>
/// <param name="length">The length of the data to be read.</param>
/// <returns>ERROR_NONE on success or an error code.</returns>
static inline int32_t I2CMaster_ReadSync(
    I2CMaster *handle, uint16_t address,
    void *data, uintptr_t length);


static inline int32_t I2CMaster_WriteThenReadAsync(
    I2CMaster *handle, uint16_t address,
    const void *writeData, uintptr_t writeLength,
    void *readData, uintptr_t readLength,
    void (*callback)(int32_t status, uintptr_t count))
{
    I2C_Transfer transfer[2] = {
        { .writeData = writeData, .readData = NULL    , .length = writeLength },
        { .writeData = NULL     , .readData = readData, .length = readLength  },
    };
    return I2CMaster_TransferSequentialAsync(handle, address, transfer, 2, callback);
}

static inline int32_t I2CMaster_WriteAsync(
    I2CMaster *handle, uint16_t address,
    const void *data, uintptr_t length,
    void (*callback)(int32_t status, uintptr_t count))
{
    I2C_Transfer transfer = {
        .writeData = data,
        .readData  = NULL,
        .length    = length,
    };
    return I2CMaster_TransferSequentialAsync(handle, address, &transfer, 1, callback);
}

static inline int32_t I2CMaster_ReadAsync(
    I2CMaster *handle, uint16_t address,
    void *data, uintptr_t length,
    void (*callback)(int32_t status, uintptr_t count))
{
    I2C_Transfer transfer = {
        .writeData = NULL,
        .readData  = data,
        .length    = length,
    };
    return I2CMaster_TransferSequentialAsync(handle, address, &transfer, 1, callback);
}


static inline int32_t I2CMaster_WriteThenReadSync(
    I2CMaster *handle, uint16_t address,
    const void *writeData, uintptr_t writeLength,
    void *readData, uintptr_t readLength)
{
    I2C_Transfer transfer[2] = {
        { .writeData = writeData, .readData = NULL    , .length = writeLength },
        { .writeData = NULL     , .readData = readData, .length = readLength  },
    };
    return I2CMaster_TransferSequentialSync(handle, address, transfer, 2);
}

static inline int32_t I2CMaster_WriteSync(
    I2CMaster *handle, uint16_t address,
    const void *data, uintptr_t length)
{
    I2C_Transfer transfer = {
        .writeData = data,
        .readData  = NULL,
        .length    = length,
    };
    return I2CMaster_TransferSequentialSync(handle, address, &transfer, 1);
}

static inline int32_t I2CMaster_ReadSync(
    I2CMaster *handle, uint16_t address,
    void *data, uintptr_t length)
{
    I2C_Transfer transfer = {
        .writeData = NULL,
        .readData  = data,
        .length    = length,
    };
    return I2CMaster_TransferSequentialSync(handle, address, &transfer, 1);
}

#ifdef __cplusplus
 }
#endif

#endif // #ifndef AZURE_SPHERE_I2CMASTER_H_
