/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

//
// Inter-core messages
//   First byte is the message ID
//   Remainder of payload is specific to each message ID
//

#define MSG_BLOCK_WRITE         1
#define MSG_BLOCK_READ          2
#define MSG_BLOCK_READ_RESULT   3
#define MSG_BLOCK_WRITE_RESULT  4


/// <summary>
/// used in read requests (results in a read result) and write results
/// </summary>
struct SD_CMD {
    uint8_t     id;                 // Message Type (WRITE or READ)
    uint32_t    blockNumber;        // block number on card to read or write
    int     read_write_result;  // result of read or write request (read may fail, in which case we don't return data)
};

/// <summary>
/// Use for write requests and read results
/// </summary>
struct SD_CMD_WITH_DATA {
    uint8_t     id;                 // Message Type (WRITE or READ)
    uint32_t    blockNumber;        // block number on card to read or write
    uint8_t     blockData[512];     // block data to write or read.
};

