/// \file spi_structs_v1.h
/// \brief This header contains the v1 definition of SPI API structures and
/// associated types.
///
/// You should not include this header directly; include applibs/spi.h instead.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// <summary>
/// The configuration options that must be passed to the <see cref="SPIMaster_Open function"/>.
/// Call <see cref="SPIMaster_InitConfig"/> to initialize an instance.
///
/// After you define SPI_STRUCTS_VERSION, you can use the SPIMaster_Config alias to access this
/// structure.
/// </summary>
struct z__SPIMaster_Config_v1 {
    /// <summary>A unique identifier of the struct type and version. Do not edit.</summary>
    uint32_t z__magicAndVersion;
    /// <summary>The chip select polarity.</summary>
    SPI_ChipSelectPolarity csPolarity;
};

/// <summary>
/// The description of an SPI master transfer operation. Call <see
/// cref="SPIMaster_InitTransfer"/> to initialize an instance.
///
/// After you define SPI_STRUCTS_VERSION, you can use the SPIMaster_Transfer alias to access this
/// structure.
/// </summary>
struct z__SPIMaster_Transfer_v1 {
    /// <summary>A unique identifier of the struct type and version. Do not edit.</summary>
    uint32_t z__magicAndVersion;
    /// <summary>Transfer flags.</summary>
    SPI_TransferFlags flags;
    /// <summary>Data for write operations. This value is ignored for half-duplex read
    /// operations.</summary>
    const uint8_t *writeData;
    /// <summary>The buffer for read operations. This value is ignored for half-duplex write
    /// operations.</summary>
    uint8_t *readData;
    /// <summary>Number of bytes to transfer.</summary>
    size_t length;
};

#ifdef __cplusplus
}
#endif
