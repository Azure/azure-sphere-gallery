/// \file spi.h
/// \brief This header contains the functions and types that access an SPI
/// (Serial Peripheral Interface) interface on a device. To access individual SPI interfaces,
/// your application must identify them in the SpiMaster field of the application manifest.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

// Default the SPI_STRUCTS_VERSION version to 1
#ifndef SPI_STRUCTS_VERSION
#define SPI_STRUCTS_VERSION 1
#endif

/// <summary>
///     The maximum number of transfer requests that you can pass to
///     <see cref="SPIMaster_TransferSequential"/>.
/// </summary>
#define SPI_MASTER_MAX_TRANSFERS 256

/// <summary>
///     The ID for an instance of an SPI master interface.
/// </summary>
typedef int SPI_InterfaceId;

/// <summary>
///     An SPI chip select ID.
/// </summary>
typedef int SPI_ChipSelectId;

/// <summary>
///     The possible communication mode values for an SPI interface.
///     The communication mode defines timings for device communication.
/// </summary>
typedef enum SPI_Mode {
    /// <summary>
    /// An invalid mode.
    /// </summary>
    SPI_Mode_Invalid = 0x0,

    /// <summary>
    /// SPI mode 0: clock polarity (CPOL) = 0, clock phase (CPHA) = 0.
    /// </summary>
    SPI_Mode_0 = 0x1,

    /// <summary>
    /// SPI mode 1: clock polarity (CPOL) = 0, clock phase (CPHA) = 1.
    /// </summary>
    SPI_Mode_1 = 0x2,

    /// <summary>
    /// SPI mode 2: clock polarity (CPOL) = 1, clock phase (CPHA) = 0.
    /// </summary>
    SPI_Mode_2 = 0x3,

    /// <summary>
    /// SPI mode 3: clock polarity (CPOL) = 1, clock phase (CPHA) = 1.
    /// </summary>
    SPI_Mode_3 = 0x4
} SPI_Mode;

/// <summary>
///     The possible SPI bit order values.
/// </summary>
typedef enum SPI_BitOrder {
    /// <summary>
    /// An invalid order.
    /// </summary>
    SPI_BitOrder_Invalid = 0x0,

    /// <summary>
    /// The least-significant bit is sent first.
    /// </summary>
    SPI_BitOrder_LsbFirst = 0x1,

    /// <summary>
    /// The most-significant bit is sent first.
    /// </summary>
    SPI_BitOrder_MsbFirst = 0x2
} SPI_BitOrder;

/// <summary>
///     The possible chip select polarity values for an SPI interface.
/// </summary>
typedef enum SPI_ChipSelectPolarity {
    /// <summary>
    /// An invalid polarity.
    /// </summary>
    SPI_ChipSelectPolarity_Invalid = 0x0,

    /// <summary>
    /// Active low.
    /// </summary>
    SPI_ChipSelectPolarity_ActiveLow = 0x1,

    /// <summary>
    /// Active high.
    /// </summary>
    SPI_ChipSelectPolarity_ActiveHigh = 0x2
} SPI_ChipSelectPolarity;

/// <summary>
///    The possible flags values for a <see cref="SPIMaster_Transfer"/> struct.
/// </summary>
typedef enum SPI_TransferFlags {
    /// <summary>
    /// No flags present.
    /// </summary>
    SPI_TransferFlags_None = 0x0,

    /// <summary>
    /// Read from the subordinate device.
    /// </summary>
    SPI_TransferFlags_Read = 0x1,

    /// <summary>
    /// Write to the subordinate device.
    /// </summary>
    SPI_TransferFlags_Write = 0x2
} SPI_TransferFlags;

#include <applibs/spi_structs_v1.h>

#if SPI_STRUCTS_VERSION == 1
/// <summary>
///     An alias to the z__SPIMaster_Config_v1 struct. After you define
///     SPI_STRUCTS_VERSION, you can refer to z__SPIMaster_Config_v1
///     structs with this alias.
/// </summary>
typedef struct z__SPIMaster_Config_v1 SPIMaster_Config;
/// <summary>
///     An alias to the z__SPIMaster_Transfer_v1 struct. After you define
///     SPI_STRUCTS_VERSION, you can refer to z__SPIMaster_Transfer_v1
///     structs with this alias.
/// </summary>
typedef struct z__SPIMaster_Transfer_v1 SPIMaster_Transfer;
#else
#error To use applibs/spi.h you must first #define a supported SPI_STRUCTS_VERSION
#endif

/// <summary>
///     Initializes a <see cref="SPIMaster_Config"/> struct with the default SPI master interface
///     settings.
/// </summary>
/// <param name="config">
///     A pointer to a <see cref="SPIMaster_Config"/> struct that receives the default SPI master
///     interface settings.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static inline int SPIMaster_InitConfig(SPIMaster_Config *config);

/// <summary>
///     Opens and configures an SPI master interface for exclusive use by an application, and
///     returns a file descriptor for the interface. The file descriptor can be used by Applibs
///     SPI functions, and standard read(2) and write(2) functions.
///     <para>The interface is initialized with the default settings: SPI_Mode_0,
///     SPI_BitOrder_MsbFirst. You can change these settings with SPI functions after the interface
///     is opened.</para>
///     <para>The config paramter points to another group of settings that you
///     can't change after <see cref="SPIMaster_Open"/> is called. Before you call <see
///     cref="SPIMaster_Open"/>, you must call <see cref="SPIMaster_InitConfig"/> to initialize the
///     <see cref="SPIMaster_Config"/> struct for the config parameter. After you initialize the
///     struct, you can change the configuration until <see cref="SPIMaster_Open"/> is
///     called.</para>
///     <para> **Errors** </para>
///     <para>EACCES: access to this SPI interface is not permitted because the <paramref
///     name="interfaceId"/> parameter is not listed in the SpiMaster field of the application
///     manifest.</para>
///     <para>Any other errno may also be specified, such errors aren't deterministic and there's no
///     guarantee that the same behavior will be retained through system updates.</para>
/// </summary>
/// <param name="interfaceId">
///     The ID of the SPI master interface to open.
/// </param>
/// <param name="chipSelectId">
///     The chip select ID to use with the SPI master interface.
/// </param>
/// <param name="config">
///     The configuration for the SPI master interface. This struct doesn't contain all of the
///     SPI master interface settings; it only contains setttings that can't be changed after
///     <see cref="SPIMaster_Open"/> is called.
///     Before you call this function, you must call <see cref="SPIMaster_InitConfig"/> to
///     initialize this <see cref="SPIMaster_Config"/> struct. After you initialize the struct, any
///     changes you make to the configuration must be made before <see cref="SPIMaster_Open"/> is
///     called.
/// </param>
/// <returns>
///     The file descriptor of the SPI interface if if was opened successfully, or -1 for failure,
///     in which case errno is set to the error value. You can use this file descriptor to perform
///     transactions with Applibs SPI functions, and with standard read(2) and write(2)
///     functions.
/// </returns>
static inline int SPIMaster_Open(SPI_InterfaceId interfaceId, SPI_ChipSelectId chipSelectId,
                                 const SPIMaster_Config *config);

/// <summary>
///     Sets the bus speed for operations on an SPI master interface.
/// </summary>
/// <param name="fd">
///     The file descriptor for the SPI master interface.
/// </param>
/// <param name="speedInHz">
///     The maximum speed for transfers on the interface, in Hz. Not all speeds are supported on
///     all devices. The actual speed used by the interface may be lower than this value.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int SPIMaster_SetBusSpeed(int fd, uint32_t speedInHz);

/// <summary>
///     Sets the communication mode for an SPI master interface.
/// </summary>
/// <param name="fd">
///     The file descriptor for the SPI master interface.
/// </param>
/// <param name="mode">
///     The requested <see cref="SPI_Mode"/> value.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int SPIMaster_SetMode(int fd, SPI_Mode mode);

/// <summary>
///     Sets the bit order for data transfers on an SPI master interface.
/// </summary>
/// <param name="fd">
///     The file descriptor for the SPI master interface.
/// </param>
/// <param name="order">
///     The requested bit order for data transfers.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
int SPIMaster_SetBitOrder(int fd, SPI_BitOrder order);

/// <summary>
///     Performs a sequence of half-duplex writes followed by a half-duplex read in a single
///     transaction using a SPI master interface. This function enables chip select once before the
///     sequence, and disables it when it ends.
/// </summary>
/// <param name="fd">
///     The file descriptor for the SPI master interface.
/// </param>
/// <param name="writeData">
///     The data to write.
/// </param>
/// <param name="lenWriteData">
///     The number of bytes to write.
/// </param>
/// <param name="readData">
///     The output buffer that receives the data. This buffer must be large enough to
///     receive up to <paramref name="lenReadData" /> bytes.
/// </param>
/// <param name="lenReadData">
///     The number of bytes to read.
/// </param>
/// <returns>
///     The number of bytes transferred; or -1 for failure, in which case errno is set to the error
///     value.
/// </returns>
static inline ssize_t SPIMaster_WriteThenRead(int fd, const uint8_t *writeData, size_t lenWriteData,
                                              uint8_t *readData, size_t lenReadData);

/// <summary>
///     Initializes an array of <see cref="SPIMaster_Transfer"/> structs with the default SPI master
///     transfer settings.
/// </summary>
/// <param name="transfers">
///     A pointer to the array of <see cref="SPIMaster_Transfer"/> structs to initialize.
/// </param>
/// <param name="transferCount">
///     The number of structs in the <paramref name="transfers"/> array.
/// </param>
/// <returns>
///     0 for success, or -1 for failure, in which case errno is set to the error value.
/// </returns>
static inline int SPIMaster_InitTransfers(SPIMaster_Transfer *transfers, size_t transferCount);

/// <summary>
///     Performs a sequence of half-duplex read or write transfers using the SPI master interface.
///     This function enables chip select once before the sequence, and disables it when it ends.
///     This function does not support simultaneously reading and writing in a single transaction.
/// </summary>
/// <param name="fd">
///     The file descriptor for the SPI master interface.
/// </param>
/// <param name="transfers">
///     An array of <see cref="SPIMaster_Transfer"/> structures that specify the transactions to
///     perform. You must call <see cref="SPIMaster_InitTransfers"/> to initialize the array with
///     default settings before filling it.
/// </param>
/// <param name="transferCount">
///     The number of <see cref="SPIMaster_Transfer"/> structures in the <paramref
///     name="transfers"/> array.
/// </param>
/// <returns>
///     The number of bytes transferred; or -1 for failure, in which case errno is set to the error
///     value.
/// </returns>
static inline ssize_t SPIMaster_TransferSequential(int fd, const SPIMaster_Transfer *transfers,
                                                   size_t transferCount);

#include <applibs/spi_internal.h>

#ifdef __cplusplus
}
#endif
