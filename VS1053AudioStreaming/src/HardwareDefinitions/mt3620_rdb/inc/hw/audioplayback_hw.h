/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This file defines the mapping from the MT3620 reference development board (RDB) to the
// Adafruit VS1053 board

#pragma once
#include "mt3620_rdb.h"

#define VS1053_DREQ MT3620_RDB_HEADER2_PIN6_GPIO
#define VS1053_RST MT3620_RDB_HEADER2_PIN14_GPIO
#define VS1053_DCS MT3620_RDB_HEADER2_PIN12_GPIO
#define VS1053_SPI MT3620_RDB_HEADER4_ISU1_SPI
#define VS1053_CS MT3620_RDB_HEADER1_PIN3_GPIO
#define VS1053_SPICS MT3620_SPI_CS_B



