/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "dram.h"

// Dummy data
#define DUMMY 0x00

// File descriptors - initialized to an invalid value
static int spiFd = -1;

int dram_init(int spi_interface, int cs_pin, int io3, int io2)
{
    // Create SPI config object
    SPIMaster_Config spi_cfg;

    // Initialize SPI config object
    int ret = SPIMaster_InitConfig(&spi_cfg);
    if (ret != 0) {
        Log_Debug("ERROR: SPIMaster_InitConfig = %d errno = %s (%d)\n", ret, strerror(errno),
                  errno);
        return -1;
    }

    // Set polarity
    spi_cfg.csPolarity = SPI_ChipSelectPolarity_ActiveLow;

    // Open SPI Master peripheral based on the interface specified
    spiFd = SPIMaster_Open(spi_interface, cs_pin, &spi_cfg);
    if (spiFd == -1) {
        Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }

    // Set bus speed for SPI Master
    int result = SPIMaster_SetBusSpeed(spiFd, 39999999); // MT3620 SPI bus speed < 40MHz
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }

    // Set mode of SPI communication, SPI_Mode_2 for Avnet rev1 kit
    // and SPI_Mode_0 for Avnet rev2 kit
    result = SPIMaster_SetMode(spiFd, SPI_Mode_2);
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_SetMode: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }

    // Configuring DRAM pins for QPI
    GPIO_OpenAsOutput(io3, GPIO_OutputMode_PushPull, GPIO_Value_Low);
    GPIO_OpenAsOutput(io2, GPIO_OutputMode_PushPull, GPIO_Value_Low);

    int exitCode = dram_reset();

    // 50ns sleep before next command

    if (exitCode != 0) {
        Log_Debug("Software reset failure!\n");
        return exitCode;
    }

    exitCode = dram_check_communication();
    if (exitCode != 0) {
        Log_Debug("Communication check failure!\n");
        return exitCode;
    }

    // DRAM chip default
    linear_burst_mode = true;
    max_per_transfer = 1000;

    return 0;
}

int dram_memory_write(uint32_t address, uint8_t *data_in, uint32_t len)
{
    // Check validity of write address
    if ((data_in == NULL) || (address > DRAM_MAX_ADDRESS)) {
        Log_Debug("ERROR: No data input OR invalid address\n");
        return -1;
    }

    // Check if payload size is valid
    if (len > DRAM_MAX_ADDRESS) {
        Log_Debug("ERROR: Data size exceeds chip size (8M)");
        return -1;
    }

    // Check if overflow will occur
    if ((address + len - 1) > DRAM_MAX_ADDRESS) {
        unsigned long int ovf_addr = (address + len - 1) % DRAM_MAX_ADDRESS;
        Log_Debug("Overflow write! Data terminates at at %#08X\n", ovf_addr);
    }

    // 2 transfers per Transfer Sequential call (command & address bytes and data bytes)
    SPIMaster_Transfer transfers_array[2];

    // init transfers
    int result = SPIMaster_InitTransfers(transfers_array, 2);
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_Open (dram_memory_write): errno=%d (%s)\n", errno,
                  strerror(errno));
        return -1;
    }

    // Mandatory transfer to signal a write to the starting address
    uint8_t data_buf[4] = {0};
    data_buf[0] = DRAM_CMD_WRITE;
    data_buf[1] = (uint8_t)((address >> 16) & 0xFF);
    data_buf[2] = (uint8_t)((address >> 8) & 0xFF);
    data_buf[3] = (uint8_t)(address & 0xFF);

    // "write_data" is a pointer to an address on the Azure Sphere (not DRAM) that indicates where
    // to begin the transfer from
    const uint8_t *writeData;

    // Initiate transfer
    ssize_t tb;

    // "length" is an integer value of how many bytes to read starting from the "writeData" address
    size_t t_length;

    // Max number of data bytes per transfer
    uint32_t temp_max = max_per_transfer - sizeof(data_buf);

    // Number of calls to SPIMaster_Transfer_Sequential (transfer) depends of max bytes per transfer
    // (1000 for Linear Burst and 4096 for Wrap32)
    size_t transfer_count =
        (len % temp_max == 0)
            ? (len / temp_max)
            : ((len / temp_max) + 1); //(subtracting 4 because the first four bytes of each transfer
                                      // is dedicated to the command & address bytes)

    // Data address offset
    uint32_t offset = 0;

    // Number of bytes left to transfer
    uint32_t sizeLeft = len;

    // Add the rest of the bytes to the initial transfer (max_per_transfer -  4)
    for (uint16_t i = 1; i <= transfer_count; i++) {

        transfers_array[0].flags = SPI_TransferFlags_Write; // Signal a write
        transfers_array[0].writeData = data_buf;            // Add data to write buff property
        transfers_array[0].length = sizeof(data_buf);       // 24 bit address + 4 bit command code

        // Signal a write on the SPI bus
        transfers_array[1].flags = SPI_TransferFlags_Write;

        // Start writing from starting address + offset
        writeData = data_in + offset;
        transfers_array[1].writeData = writeData;

        // Data size for transfer is either temp max or sizeLeft % temp_max
        if (sizeLeft > temp_max) {

            t_length = temp_max;
            transfers_array[1].length = t_length;

            // Starting address (on DRAM) for the next transfer is the address immediately after the
            // last transfer
            address += temp_max;

            // Subtract transfer length from the number of bytes left to send
            sizeLeft -= temp_max;

            // Adjust the offset to start the data transfer at the the next byte after the maximum
            // transfer length (pointer arithmetic)
            offset += temp_max;
        } else {
            t_length = sizeLeft;
            transfers_array[1].length = t_length;
        }

        // number of bytes transferred
        tb = SPIMaster_TransferSequential(spiFd, transfers_array, 2);

        // Check transfer size
        if (!CheckTransferSize("SPIMaster_TransferSequential (dram_memory_write)",
                               (sizeof(data_buf) + t_length), tb)) {
            return -1;
        }
    }

    return 0;
}

int dram_memory_read(uint32_t address, uint8_t *data_out, uint32_t len)
{
    if ((data_out == NULL) || (address > DRAM_MAX_ADDRESS)) {
        Log_Debug("ERROR: No data input OR invalid address\n");
        return -1;
    }

    if (len > DRAM_MAX_ADDRESS) {
        Log_Debug("ERROR: Data size exceeds chip size (8M)");
        return -1;
    }

    if ((address + len - 1) > DRAM_MAX_ADDRESS) {
        unsigned long int ovf_addr = (address + len - 1) % DRAM_MAX_ADDRESS;
        Log_Debug("Overflow read! Data terminates at at %#08X\n", ovf_addr);
    }

    SPIMaster_Transfer transfers_array[2];

    int result = SPIMaster_InitTransfers(transfers_array, 2);
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_Open (dram_memory_read): errno=%d (%s)\n", errno,
                  strerror(errno));
        return -1;
    }

    uint8_t data_buf[4] = {0};
    data_buf[0] = DRAM_CMD_READ;
    data_buf[1] = (uint8_t)((address >> 16) & 0xFF);
    data_buf[2] = (uint8_t)((address >> 8) & 0xFF);
    data_buf[3] = (uint8_t)(address & 0xFF);

    uint8_t *readData;
    ssize_t tb;
    size_t t_length;
    uint32_t temp_max = max_per_transfer - sizeof(data_buf);
    size_t transfer_count =
        (len % temp_max == 0)
            ? (len / temp_max)
            : ((len / temp_max) + 1); 
    uint32_t offset = 0;
    uint32_t sizeLeft = len;

    for (uint16_t i = 1; i <= transfer_count; i++) {

        transfers_array[0].flags = SPI_TransferFlags_Write;
        transfers_array[0].writeData = data_buf;
        transfers_array[0].length = sizeof(data_buf);

        transfers_array[1].flags = SPI_TransferFlags_Read;

        readData = data_out + offset;
        transfers_array[1].readData = readData;

        if (sizeLeft > temp_max) {
            t_length = temp_max;
            transfers_array[1].length = t_length;
            address += temp_max;
            sizeLeft -= temp_max;
            offset += temp_max;
        } else {
            t_length = sizeLeft;
            transfers_array[1].length = t_length;
        }

        result = SPIMaster_SetBusSpeed(spiFd, 33000000);
        if (result != 0) {
            Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
            return -1;
        }

        tb = SPIMaster_TransferSequential(spiFd, transfers_array, 2);

        result = SPIMaster_SetBusSpeed(spiFd, 39999999);
        if (result != 0) {
            Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
            return -1;
        }

        if (!CheckTransferSize("SPIMaster_TransferSequential (dram_memory_read)",
                               (sizeof(data_buf) + t_length), tb)) {
            return -1;
        }
    }

    return 0;
}

int dram_memory_read_fast(uint32_t address, uint8_t *data_out, uint32_t len)
{
    if ((data_out == NULL) || (address > DRAM_MAX_ADDRESS)) {
        Log_Debug("ERROR: No data input OR invalid address\n");
        return -1;
    }

    if (len > DRAM_MAX_ADDRESS) {
        Log_Debug("ERROR: Data size exceeds chip size (8M)");
        return -1;
    }

    if ((address + len - 1) > DRAM_MAX_ADDRESS) {
        unsigned long int ovf_addr = (address + len - 1) % DRAM_MAX_ADDRESS;
        Log_Debug("Overflow read! Data terminates at at %#08X\n", ovf_addr);
    }

    SPIMaster_Transfer transfers_array[2];

    int result = SPIMaster_InitTransfers(transfers_array, 2);
    if (result < 0) {
        Log_Debug("ERROR: SPIMaster_Open (dram_memory_read_fast): errno=%d (%s)\n", errno,
                  strerror(errno));
        return -1;
    }

    uint8_t data_buf[5] = {0};
    data_buf[0] = DRAM_CMD_FAST_READ;
    data_buf[1] = (uint8_t)((address >> 16) & 0xFF);
    data_buf[2] = (uint8_t)((address >> 8) & 0xFF);
    data_buf[3] = (uint8_t)(address & 0xFF);
    data_buf[4] = DUMMY;

    uint8_t *readData;
    ssize_t tb;
    size_t t_length;
    uint32_t temp_max = max_per_transfer - sizeof(data_buf);
    size_t transfer_count =
        (len % temp_max == 0)
            ? (len / temp_max)
            : ((len / temp_max) + 1); 
    uint32_t offset = 0;
    uint32_t sizeLeft = len;

    for (uint16_t i = 1; i <= transfer_count; i++) {

        transfers_array[0].flags = SPI_TransferFlags_Write;
        transfers_array[0].writeData = data_buf;
        transfers_array[0].length = sizeof(data_buf);

        transfers_array[1].flags = SPI_TransferFlags_Read;

        readData = data_out + offset;
        transfers_array[1].readData = readData;

        if (sizeLeft > temp_max) {
            t_length = temp_max;
            transfers_array[1].length = t_length;
            address += temp_max;
            sizeLeft -= temp_max;
            offset += temp_max;
        } else {
            t_length = sizeLeft;
            transfers_array[1].length = t_length;
        }

        tb = SPIMaster_TransferSequential(spiFd, transfers_array, 2);

        if (!CheckTransferSize("SPIMaster_TransferSequential (dram_memory_read_fast)",
                               (sizeof(data_buf) + t_length), tb)) {
            return -1;
        }
    }

    return 0;
}

int dram_reset()
{
    const size_t transferCount = 1;
    SPIMaster_Transfer transfer1[transferCount];
    SPIMaster_Transfer transfer2[transferCount];

    // init transfers
    int result = SPIMaster_InitTransfers(transfer1, transferCount);
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_InitTransfers (dram_reset transfer1)  errno=%d (%s)\n", errno,
                  strerror(errno));
        return -1;
    }

    result = SPIMaster_InitTransfers(transfer2, transferCount);
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_InitTransfers (dram_reset transfer2)  errno=%d (%s)\n", errno,
                  strerror(errno));
        return -1;
    }

    uint8_t data = DRAM_CMD_RESET_ENABLE;
    uint8_t data2 = DRAM_CMD_RESET;

    transfer1[0].flags = SPI_TransferFlags_Write; // Signal a write
    transfer1[0].writeData = &data;               // Add data to write buff property
    transfer1[0].length = 1;

    transfer2[0].flags = SPI_TransferFlags_Write; // Signal a write
    transfer2[0].writeData = &data2;              // Add data to write buff property
    transfer2[0].length = 1;

    ssize_t transferredBytes = SPIMaster_TransferSequential(spiFd, transfer1, transferCount);
    ssize_t transferredBytes2 = SPIMaster_TransferSequential(spiFd, transfer2, transferCount);

    if (!CheckTransferSize("SPIMaster_TransferSequential (dram_reset) transfer1", sizeof(data),
                           transferredBytes)) {
        return -1;
    }

    if (!CheckTransferSize("SPIMaster_TransferSequential (dram_reset) transfer2", sizeof(data),
                           transferredBytes2)) {
        return -1;
    }

    // 50ns sleep time is required after reset operation
    const struct timespec sleepTime = {.tv_sec = 0, .tv_nsec = 50};
    nanosleep(&sleepTime, NULL);

    return 0;
}

bool dram_toggle_wrap_boundary()
{
    const size_t transferCount = 1;
    SPIMaster_Transfer transfers[transferCount];

    // init transfers
    int result = SPIMaster_InitTransfers(transfers, transferCount);
    if (result == -1) {
        Log_Debug("ERROR: SPIMaster_InitTransfers (dram_toggle_wrap_boundary): errno=%d (%s)\n",
                  errno, strerror(errno));
        return -1;
    }

    uint8_t data = DRAM_CMD_WRAP_BOUNDARY_TOGGLE;

    transfers[0].flags = SPI_TransferFlags_Write;
    transfers[0].writeData = &data;
    transfers[0].length = 1;

    ssize_t transferredBytes = SPIMaster_TransferSequential(spiFd, transfers, transferCount);

    if (!CheckTransferSize("SPIMaster_TransferSequential (dram_toggle_wrap_boundary)", sizeof(data),
                           transferredBytes)) {
        return -1;
    }

    // toggle state variable
    linear_burst_mode = !linear_burst_mode;

    if (linear_burst_mode) {
        Log_Debug("Linear Burst Mode is active.\n");
        max_per_transfer = 1000;
    } else {
        Log_Debug("Wrap32 Mode is active.\n");
        max_per_transfer = 4096;
    }

    return 0;
}

bool dram_read_id(uint8_t *device_id)
{
    if (NULL == device_id) {
        Log_Debug("Device ID is null\n");
        return false;
    }
    uint8_t data_buf[4] = {0};
    data_buf[0] = DRAM_CMD_READ_ID;
    data_buf[1] = DUMMY;
    data_buf[2] = DUMMY;
    data_buf[3] = DUMMY;

    ssize_t transferredBytes =
        SPIMaster_WriteThenRead(spiFd, data_buf, sizeof(data_buf), device_id, sizeof(device_id));

    if (!CheckTransferSize("SPIMaster_WriteThenRead (dram_read_id)",
                           sizeof(data_buf) + sizeof(device_id), transferredBytes)) {
        return false;
    }

    return true;
}

int dram_check_communication()
{
    uint8_t device_id[8] = {0};
    if (dram_read_id(device_id)) {
        if (DRAM_MANUFACTURER_ID == device_id[0]) {
            return 0;
        }
    }
    return -1;
}

// Support functions

/// <summary>
/// Checks the number of transferred bytes for SPI functions and prints an error
/// message if the functions failed or if the number of bytes is different than
/// expected number of bytes to be transferred.
/// </summary>
/// <param name="desc">
/// String description to identify where errors happen should they occur
/// </param>
/// <param name="expectedBytes">
/// Number of bytes that were expected to be transferred
/// </param>
/// <param name="actualBytes">
/// Number of bytes that were transferred
/// </param>
/// <returns>
/// true on success, or false on failure
/// </returns>
bool CheckTransferSize(const char *desc, size_t expectedBytes, ssize_t actualBytes)
{
    if (actualBytes < 0) {
        Log_Debug("ERROR: %s: errno=%d (%s)\n", desc, errno, strerror(errno));
        return false;
    }

    if (actualBytes != (ssize_t)expectedBytes) {
        Log_Debug("ERROR: %s: transferred %zd bytes; expected %zd\n", desc, actualBytes,
                  expectedBytes);
        return false;
    }

    return true;
}