/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "SDCardViaRtCore.h"
#include "intercore_messages.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>

#include "utils.h"

#include <applibs/log.h>
#include <applibs/application.h>


static const char rtAppComponentId[] = "005180bc-402f-4cb3-a662-72937dbcde47";

static int sockFd = -1;
static uint8_t recvBuffer[sizeof(struct SD_CMD_WITH_DATA)];
static ssize_t ReadRTData(uint8_t* data, size_t size);
static ssize_t WriteRTData(uint8_t *data, size_t size);

/// <summary>
/// Initializes the connection to the M4 application
/// </summary>
int SDCard_Init(void)
{
    // Open a connection to the RTApp.
    sockFd = Application_Connect(rtAppComponentId);
    if (sockFd == -1) {
        return -1;
    }

    // Set timeout, to handle case where real-time capable application does not respond.
    static const struct timeval recvTimeout = { .tv_sec = 5, .tv_usec = 0 };
    int result = setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));
    if (result == -1) {
        return -1;
    }

    return 0;
}

/// <summary>
/// Cleanup the inter-core socket
/// </summary>
void SDCard_Cleanup(void)
{
    if (sockFd != -1)
    {
        close(sockFd);
        sockFd = -1;
    }
}

/// <summary>
/// Write SD Card block
/// block number is passed from LittleFs
/// data size is defined by the BLOCK_SIZE in main (controlled by Littlefs)
/// </summary>
int SDCard_WriteBlock(uint32_t block, uint8_t* data, uint32_t size)
{
    struct SD_CMD_WITH_DATA writeRequest;
    writeRequest.id = MSG_BLOCK_WRITE;
    writeRequest.blockNumber = block;
    memcpy(writeRequest.blockData, data, size);

#ifdef SHOW_DEBUG_INFO
    Log_Debug("Writing Block %d\n", block);
    DumpBuffer(&writeRequest, sizeof(writeRequest));
#endif
    if (WriteRTData(&writeRequest,sizeof(writeRequest)) == -1)
    {
        // failed to write to the M4
        return -1;
    }

    // get result.
    ssize_t numBytes = ReadRTData(&recvBuffer[0], sizeof(recvBuffer));

    if (numBytes == sizeof(struct SD_CMD))
    {
        struct SD_CMD* pData = (struct SD_CMD*)&recvBuffer[0];
        if (pData->read_write_result == 0)  // OK
        {
            return 0;
        }
    }
    else
    {
        Log_Debug("write block %d - result not sizeof(SD_CMD) [%d bytes returned]\n", block, numBytes);
    }

    return -1;
}

/// <summary>
/// Write SD Card block
/// block number is passed from LittleFs
/// data size is defined by the BLOCK_SIZE in main (controlled by Littlefs)
/// </summary>
int SDCard_ReadBlock(uint32_t block, uint8_t* data, uint32_t size)
{
    struct SD_CMD readRrequest;
    readRrequest.id = MSG_BLOCK_READ;
    readRrequest.blockNumber = block;

    if (WriteRTData(&readRrequest,sizeof(readRrequest)) == -1)
    {
        // failed to write to the M4
        return -1;
    }

    ssize_t numBytes = ReadRTData(&recvBuffer[0], sizeof(recvBuffer));
    if (numBytes == sizeof(struct SD_CMD_WITH_DATA))
    {
        struct SD_CMD_WITH_DATA* pData = (struct SD_CMD_WITH_DATA*)&recvBuffer[0];
        memcpy(data, pData->blockData, size);
        return 0;
    }

    if (numBytes == sizeof(struct SD_CMD))
    {
        struct SD_CMD* pData = (struct SD_CMD*)&recvBuffer[0];
        Log_Debug("Read return: %d\n", pData->read_write_result);
    }
    else
    {
        Log_Debug("read block %d - result not sizeof(SD_CMD) [%d bytes returned]\n", block, numBytes);
    }

    return -1;
}

static ssize_t WriteRTData(uint8_t *data, size_t size)
{
    ssize_t ret=send(sockFd, data, size, 0);
    if (ret == -1)
        return -1;

    return 0;
}

/// <summary>
/// Reads data from the M4, this might be status, or block data, depending on the command and result
/// </summary>
static ssize_t ReadRTData(uint8_t *data, size_t size)
{
    ssize_t ret = read(sockFd, data, size);

#ifdef SHOW_DEBUG_INFO
    Log_Debug("Received %d bytes\n", ret);
    if (ret != -1)
    {
        DumpBuffer(recvBuffer, ret);
    }
#endif
    return ret;
}
