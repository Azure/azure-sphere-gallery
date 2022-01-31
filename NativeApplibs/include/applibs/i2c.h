/// \file i2c.h
/// \brief This header contains the functions and types that interact with an I2C
/// (Inter-Integrated Circuit) interface on a device. To access an I2C master
/// interface, your application must identify it in the I2cMaster field of the
/// application manifest.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

#define I2C_BUS_SPEED_STANDARD 100000   /* 100KHz */
#define I2C_BUS_SPEED_FAST 400000       /* 400Khz */
#define I2C_BUS_SPEED_FAST_PLUS 1000000 /* 1MHz */
#define I2C_BUS_SPEED_HIGH 3400000      /* 3.4MHz */

/// <summary>
///     The ID of an I2C master interface instance.
/// </summary>
typedef int I2C_InterfaceId;

/// <summary>
///     A 7-bit or 10-bit I2C device address that specifies the target of an I2C
///     operation. This address must not contain any additional information, such as read/write
///     bits. Not all Azure Sphere devices support 10-bit addresses.
/// </summary>
typedef uint32_t I2C_DeviceAddress;

/// <summary>
///     Opens and configures an I2C master interface for exclusive use by an application, and returns
///     a file descriptor used to perform operations on the interface.
///     <para> **Errors** </para>
///     <para>EACCES: access to this I2C interface is not permitted; verify that the interface exists
///     and is in the I2cMaster field of the application manifest.</para>
///     <para>Any other errno may also be specified, but there's no guarantee that the same behavior
///     will be retained through system updates.</para>
/// </summary>
/// <param name="id">
///     The ID of the I2C interface to open.
/// </param>
/// <returns>
///     The file descriptor of the I2C master interface, or -1 for failure, in which case errno is set
///     to the error value.
/// </returns>
int I2CMaster_Open(I2C_InterfaceId id);

/// <summary>
///     Sets the bus speed for operations on an I2C master interface.
///     <para>Not all speeds are supported on all Azure Sphere devices.</para>
/// </summary>
/// <param name="fd">
///     The file descriptor for the I2C interface.
/// </param>
/// <param name="speedInHz">
///     The requested speed, in Hz.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int I2CMaster_SetBusSpeed(int fd, uint32_t speedInHz);

/// <summary>
///     Sets the timeout for operations on an I2C master interface.
/// </summary>
/// <param name="fd">
///     The file descriptor for the I2C interface.
/// </param>
/// <param name="timeoutInMs">
///     The requested timeout, in milliseconds. This value may be rounded to the nearest supported
///     value.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int I2CMaster_SetTimeout(int fd, uint32_t timeoutInMs);

/// <summary>
///     Performs a read operation on an I2C master interface. This function provides the same
///     functionality as the POSIX read(2) function except it specifies the address of the
///     subordinate I2C device that is the target of the operation.
///     <para> **Errors** </para>
///     <para>EBUSY: the interface is busy or the I2C clock line (SCL) is being held low.</para>
///     <para>ENXIO: the operation didn't receive an ACK from the subordinate device.</para>
///     <para>ETIMEDOUT: the operation timed out before completing; you can call the
///     <see cref="I2CMaster_SetTimeout"/> function to adjust the timeout duration.</para>
///     <para>Any other errno may also be specified, but there's no guarantee that the
///     same behavior will be retained through system updates.</para>
/// </summary>
/// <param name="fd">
///     The file descriptor for the I2C master interface.
/// </param>
/// <param name="address">
///     The address of the subordinate I2C device that is the target of read operation.
/// </param>
/// <param name="buffer">
///     The output buffer that receives data from the subordinate device. This buffer must contain
///     enough space to receive maxLength bytes. This can be NULL if maxLength is 0.
/// </param>
/// <param name="maxLength">
///     The maximum number of bytes to receive. The value can be 0.
/// </param>
/// <returns>
///     The number of bytes successfully read; or -1 for failure, in which case errno is set
///     to the error value. A partial read operation, including a read of 0 bytes, is considered
///     a success.
/// </returns>
ssize_t I2CMaster_Read(int fd, I2C_DeviceAddress address, uint8_t *buffer, size_t maxLength);

/// <summary>
///     Performs a write operation on an I2C master interface. This function provides the same
///     functionality as the POSIX write() function, except it specifies the address of the
///     subordinate I2C device that is the target of the operation.
///     <para> **Errors** </para>
///     <para>EBUSY: the interface is busy or the I2C line is otherwise being held low.</para>
///     <para>ENXIO: the operation didn't receive an ACK from the subordinate device.</para>
///     <para>ETIMEDOUT: the operation timed out before completing; you can call the
///     <see cref="I2CMaster_SetTimeout"/> function to adjust the timeout duration.</para>
///     <para>Any other errno may also be specified, but there's no guarantee that the
///     same behavior will be retained through system updates.</para>
/// </summary>
/// <param name="fd">
///     The file descriptor for the I2C master interface.
/// </param>
/// <param name="address">
///     The address of the subordinate I2C device that is the target for the operation.
/// </param>
/// <param name="data">
///     The data to transmit to the target device. This value can be NULL if length is 0.
/// </param>
/// <param name="length">
///     The size of the data to transmit. This value can be 0.
/// </param>
/// <returns>
///     The number of bytes successfully written, or -1 for failure, in which case errno will be
///     set to the error value. A partial write, including a write of 0 bytes, is considered a
///     success.
/// </returns>
ssize_t I2CMaster_Write(int fd, I2C_DeviceAddress address, const uint8_t *data, size_t length);

/// <summary>
///     Performs a combined write-then-read operation on an I2C master interface. The operation
///     is a single bus transaction with the following steps: start condition, write, repeated start
///     condition, read, stop condition.
///     <para> **Errors** </para>
///     <para>EBUSY: the interface is busy or the I2C line is being held low.</para>
///     <para>ENXIO: the operation did not receive an ACK from the subordinate device.</para>
///     <para>ETIMEDOUT: the operation timed out before completing; you can use the
///     <see cref="I2CMaster_SetTimeout"/> function to adjust the timeout duration.</para>
///     <para>Any other errno may also be specified, but there's no guarantee that the
///     same behavior will be retained through system updates.</para>
/// </summary>
/// <param name="fd">
///     The file descriptor for the I2C master interface.
/// </param>
/// <param name="address">
///     The address of the target I2C device for this operation.
/// </param>
/// <param name="writeData">
///     The data to transmit to the target device.
/// </param>
/// <param name="lenWriteData">
///     The byte length of the data to transmit.
/// </param>
/// <param name="readData">
///     The output buffer that receives data from the target device. This
///     buffer must contain sufficient space to receive up to <paramref name="lenReadData" />
///     bytes.
/// </param>
/// <param name="lenReadData">
///     The byte length of the data to receive.
/// </param>
/// <returns>
///     The combined number of bytes successfully written and read, or -1 for failure, in which case
///     errno is set to the error value. A partial result, including a transfer of 0 bytes, is
///     considered a success.
/// </returns>
ssize_t I2CMaster_WriteThenRead(int fd, I2C_DeviceAddress address, const uint8_t *writeData,
                                size_t lenWriteData, uint8_t *readData, size_t lenReadData);

/// <summary>
///     Sets the address of a subordinate device that is targeted by calls to read(2) and write(2)
///     POSIX functions on the I2C master interface.
///     <para><see cref="I2CMaster_SetDefaultTargetAddress"/> is not required when using
///     <see cref="I2CMaster_Read"/>, <see cref="I2CMaster_Write"/>, or
///     <see cref="I2CMaster_WriteThenRead"/>, and has no impact on the address parameter of
///     those functions.</para>
/// </summary>
/// <param name="fd">
///     The file descriptor for the I2C master interface.
/// </param>
/// <param name="address">
///     The address of the subordinate I2C device that is the target of read(2) and write(2) operations.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value. This function
///     doesn't verify whether the device exists, so if the address is well formed, it can return success
///     for an invalid subordinate device.
/// </returns>
int I2CMaster_SetDefaultTargetAddress(int fd, I2C_DeviceAddress address);

#ifdef __cplusplus
}
#endif
