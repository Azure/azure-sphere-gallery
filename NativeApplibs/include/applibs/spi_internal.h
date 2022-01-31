/// \file spi_internal.h
/// \brief This header contains internal functions of the SPI API and should not be included
/// directly; include applibs/spi.h instead.
#pragma once

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define z__SPI_VERSIONED_STRUCT_BASE(name) \
    typedef struct z__##name##_Base {      \
        uint32_t z__magicAndVersion;       \
    } z__##name##_Base;

#define z__SPIMASTER_CONFIG_MAGIC 0x45170000U
z__SPI_VERSIONED_STRUCT_BASE(SPIMaster_Config);

#define z__SPIMASTER_TRANSFER_MAGIC 0x22130000U
z__SPI_VERSIONED_STRUCT_BASE(SPIMaster_Transfer);

int z__SPIMaster_InitConfig(z__SPIMaster_Config_Base *config, uint32_t structVersion);
static inline int SPIMaster_InitConfig(SPIMaster_Config *config)
{
    return z__SPIMaster_InitConfig((z__SPIMaster_Config_Base *)config, SPI_STRUCTS_VERSION);
}

int z__SPIMaster_Open(SPI_InterfaceId interfaceId, SPI_ChipSelectId chipSelectId,
                      const z__SPIMaster_Config_Base *config);
static inline int SPIMaster_Open(SPI_InterfaceId interfaceId, SPI_ChipSelectId chipSelectId,
                                 const SPIMaster_Config *config)
{
    return z__SPIMaster_Open(interfaceId, chipSelectId, (struct z__SPIMaster_Config_Base *)config);
}

int z__SPIMaster_InitTransfers(z__SPIMaster_Transfer_Base *transfer, size_t transferCount,
                               uint32_t structVersion);
static inline int SPIMaster_InitTransfers(SPIMaster_Transfer *transfers, size_t transferCount)
{
    return z__SPIMaster_InitTransfers((z__SPIMaster_Transfer_Base *)transfers, transferCount,
                                      SPI_STRUCTS_VERSION);
}

ssize_t z__SPIMaster_TransferSequential(int fd, const z__SPIMaster_Transfer_Base *transfers,
                                        size_t transferCount, uint32_t structVersion);
static inline ssize_t SPIMaster_TransferSequential(int fd, const SPIMaster_Transfer *transfers,
                                                   size_t transferCount)
{
    return z__SPIMaster_TransferSequential(fd, (const z__SPIMaster_Transfer_Base *)transfers,
                                           transferCount, SPI_STRUCTS_VERSION);
}

static inline ssize_t SPIMaster_WriteThenRead(int fd, const uint8_t *writeData, size_t lenWriteData,
                                              uint8_t *readData, size_t lenReadData)
{
    const size_t transferCount = 2;
    SPIMaster_Transfer transfers[transferCount];

    if (writeData == NULL || readData == NULL) {
        errno = EFAULT;
        return -1;
    }

    if (lenWriteData == 0 || lenReadData == 0) {
        errno = EINVAL;
        return -1;
    }

    int ret = SPIMaster_InitTransfers(transfers, transferCount);
    if (ret < 0) {
        return -1;
    }

    transfers[0].flags = SPI_TransferFlags_Write;
    transfers[0].writeData = writeData;
    transfers[0].length = lenWriteData;
    transfers[1].flags = SPI_TransferFlags_Read;
    transfers[1].readData = readData;
    transfers[1].length = lenReadData;

    return SPIMaster_TransferSequential(fd, transfers, transferCount);
}

#ifdef __cplusplus
}
#endif
