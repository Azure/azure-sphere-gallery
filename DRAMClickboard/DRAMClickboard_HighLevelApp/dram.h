/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

/// \file dram.h
/// \brief This header contains the functions available to use a DRAM Click board on
/// one of the SPI interfaces of the Azure Sphere micro-controller. To access individual SPI
/// interfaces, your application must identify them in the SpiMaster field of the application
/// manifest.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "applibs_versions.h"
#include <applibs/spi.h>
#include <applibs/gpio.h>
#include <applibs/log.h>

#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>

/// <summary>
/// Specified list of commands for DRAM Click driver.
/// </summary>
#define DRAM_CMD_READ 0x03
#define DRAM_CMD_FAST_READ 0x0B
#define DRAM_CMD_WRITE 0x02
#define DRAM_CMD_RESET_ENABLE 0x66
#define DRAM_CMD_RESET 0x99
#define DRAM_CMD_WRAP_BOUNDARY_TOGGLE 0xC0
#define DRAM_CMD_READ_ID 0x9F

/// <summary>
/// Specified memory address range of DRAM Click driver.
/// </summary>
#define DRAM_MIN_ADDRESS 0x000000
#define DRAM_MAX_ADDRESS 0x7FFFFFul

/// <summary>
/// Manufacturer ID of chip
/// </summary>
#define DRAM_MANUFACTURER_ID 0x0D

/// <summary>
/// The ID for an instance of an SPI master interface.
/// </summary>
typedef int SPI_InterfaceId;

/// <summary>
/// An SPI chip select ID.
/// </summary>
typedef int SPI_ChipSelectId;

bool linear_burst_mode;
uint32_t max_per_transfer;

/// <summary>
/// This function initializes all necessary pins and peripherals used
/// for this click board and sets the SPI interface to communicate with the chip
/// on the socket specified by the input param. To change the configuration of the
/// SPI interface (SPI mode, bus speed, bit order, etc), you must call this function again.
/// </summary>
/// <param name="interfaceId">
/// The ID of the SPI master interface to open.
/// </param>
/// <param name="chipSelectId">
/// The chip select ID to use with the SPI master interface.
/// </param>
/// <param name="io3">
/// The IO3 pin for the Mikrobus socket that the chip is on
/// </param>
/// <param name="io2">
/// The IO2 pin for the Mikrobus socket that the chip is on
/// </param>
/// <returns>
/// An ExitCode that indicates where program exited and set errno the error value.
/// </returns>
int dram_init(SPI_InterfaceId spi_interface, SPI_ChipSelectId cs_pin, int io3, int io2);

/// <summary>
/// This function writes a desired number of data bytes starting from the
/// selected memory address.
/// </summary>
/// <param name="address">
/// 32 bit Starting memory address [0x00000-0x7FFFFF]
/// </param>
/// <param name="data_in">
/// A pointer to the data (bytes) to be written
/// </param>
/// <param name="len">
/// Number of data bytes to be written
/// </param>
/// <returns>
/// Number of bytes successfully transferred, or -1 for failure, in which case errno is set to the
/// error value.
/// </returns>
int dram_memory_write(uint32_t address, uint8_t *data_in, uint32_t len);

/// <summary>
/// This function reads a desired number of data bytes starting from the selected memory address.
/// </summary>
/// <param name="address">
/// 32 bit Starting memory address [0x00000-0x7FFFFF]
/// </param>
/// <param name="data_out">
/// A pointer to the a memory location on the computer for the data to be read to from the memory
/// address
/// </param>
/// <param name="len">
/// Number of data bytes to be read into "data_out"
/// </param>
/// <returns>
/// Number of bytes successfully transferred, or -1 for failure, in which case errno is set to the
/// error value.
/// </returns>
int dram_memory_read(uint32_t address, uint8_t *data_out, uint32_t len);

/// <summary>
/// This function reads a desired number of data bytes starting from the selected memory address
/// performing fast read feature.
/// </summary>
/// <param name="address">
/// 32 bit Starting memory address [0x00000-0x7FFFFF]
/// </param>
/// <param name="data_out">
/// A pointer to the a memory location on the computer for the data to be read to from the memory
/// address
/// </param>
/// <param name="len">
/// Number of data bytes to be read into "data_out"
/// </param>
/// <returns>
/// Number of bytes successfully transferred, or -1 for failure, in which case errno is set to the
/// error value.
/// </returns>
int dram_memory_read_fast(uint32_t address, uint8_t *data_out, uint32_t len);

/// <summary>
/// This function resets the device by putting the device in SPI standby mode which is
/// also the default mode after power-up.
/// </summary>
/// <returns>
/// ExitCode indicating the success of the operation or -1 for failure, in which case errno is set
/// to the error value.
/// </returns>
int dram_reset(void);

/// <summary>
/// This function switches the device's wrapped boundary between Linear Burst which
/// crosses the 1K page boundary (CA[9:0]) and Wrap 32 (CA[4:0]) bytes. Default setting is Linear
/// Burst.
/// </summary>
/// <returns>
/// 0 for success or -1 for failure, in which case errno is set to the error value.
/// </returns>
bool dram_toggle_wrap_boundary(void);

/// <summary>
/// This function reads the device ID (8 bytes).
/// </summary>
/// <param name="device_id">
/// A pointer to the a memory location on the computer for the device ID to be read to
/// </param>
/// <returns>
/// Returns true if the transfer size is accurate
/// </returns>
bool dram_read_id(uint8_t *device_id);

/// <summary>
/// This function checks the communication by reading the device ID bytes and
/// verifying the manufacturer ID.
/// </summary>
/// <returns>
/// ExitCode indicating the success of the operation or -1 for failure, in which case errno is set
/// to the error value.
/// </returns>
int dram_check_communication(void);
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
bool CheckTransferSize(const char *desc, size_t expectedBytes, ssize_t actualBytes);

#ifdef __cplusplus
}
#endif