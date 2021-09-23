/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/networking.h>
#include <signal.h>

#include "remoteDiskIO.h"

// include the Simple File System header.
#include "sfs.h"

#include <assert.h>
#include "curlFunctions.h"

void DoIndexReads(void);

static char* loremText = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua";
static char* fox = "The quick brown fox jumps over the lazy dog.";

#define FILE_SYSTEM_BLOCKS  8192        // Python disk server uses 4194304 bytes, which is 8192 blocks

static int ReadBlock(uint32_t block, uint8_t* buffer, size_t size);
static int WriteBlock(uint32_t block, uint8_t* buffer, size_t size);

static int ReadBlock(uint32_t block, uint8_t* buffer, size_t size)
{
    uint8_t* data = readBlockData(block * BLOCK_SIZE, size);

    if (data == NULL)
    {
        return -1;
    }

    memcpy(buffer, data, size);
    return 0;
}

static int WriteBlock(uint32_t block, uint8_t* buffer, size_t size)
{
    int result=writeBlockData(buffer, size, block * BLOCK_SIZE);
    return result;
}

int main(void)
{
    // initialize Curl based on whether ENABLE_CURL_MEMORY_TRACE is defined or not
    initCurl();

    // wait for networking...
    bool isNetworkingReady = false;
    while (!isNetworkingReady)
    {
        Networking_IsNetworkingReady(&isNetworkingReady);
    }

    Log_Debug("Initialize, Format and Mount\n");
    // Initialize the file system
    assert(FS_Init(WriteBlock, ReadBlock, FILE_SYSTEM_BLOCKS) == 0);
    // Format storage (writes the root block)
    assert(FS_Format() == 0);
    // Mount storage.
    assert(FS_Mount() == 0);

    // Create a directory, create a file in the directory, write, then read data from the file.

    struct dirEntry dir;
    dir.maxFiles = 1000;
    dir.maxFileSize = 256;
    snprintf(dir.dirName, 8, "data");

    Log_Debug("Add directory 'data'\n");
    assert(FS_AddDirectory(&dir) == 0);

    // try to add the directory again
    int result = FS_AddDirectory(&dir);
    if (result == -1)
        Log_Debug("Failed to add second 'data' directory (expected)\n");

    // try to add 'Data' (will fail since 'data' already exists).
    snprintf(dir.dirName, 8, "Data");
    // try to add the directory again (with caps)
    result = FS_AddDirectory(&dir);
    if (result == -1)
        Log_Debug("Failed to add 'Data' directory (expected)\n");

    // Logs directory hasn't been added, FS_GetDirectoryByName will fail (expected)
    if (FS_GetDirectoryByName("logs", &dir) == -1)
    {
        Log_Debug("Attempt to get 'logs' directory info, failed (expected)\n");
    }

    Log_Debug("Write 'lorem.txt'\n");
    assert(FS_WriteFile("data", "lorem.txt", loremText, strlen(loremText)) == 0);

    // Get the first directory by index (zero based)
    memset(&dir, 0x00, sizeof(struct dirEntry));
    assert(FS_GetDirectoryByIndex(0, &dir) == 0);
    Log_Debug("First directory name by index: %s\n", dir.dirName);

    Log_Debug("Get number of files in 'data' directory\n");
    int numFiles = FS_GetNumberOfFilesInDirectory("data");
    Log_Debug("%d files in 'data' directory\n", numFiles);

    Log_Debug("Read Oldest file in 'data' directory\n");
    struct fileEntry file;
    if (FS_GetOldestFileInfo("data", &file) == 0)
    {
        char* buffer = (char*)malloc(file.fileSize + 1);
        memset(buffer, 0x00, file.fileSize + 1);
        FS_ReadOldestFile("data", buffer, file.fileSize);
        Log_Debug("'%s' Size: %d, data: %s", file.fileName, file.fileSize, buffer);
        free(buffer);
    }
    else
    {
        Log_Debug("Error fetching oldest file\n");
    }

    Log_Debug("Delete oldest file\n");
    FS_DeleteOldestFileInDirectory("data");
    numFiles = FS_GetNumberOfFilesInDirectory("data");
    Log_Debug("%d files in 'data' directory\n", numFiles);

    DoIndexReads();

    cleanupCurl();

    while (true);   // spin...

    return 0;
}

/// <summary>
/// Function to excercise the file index functions.
/// </summary>
void DoIndexReads(void)
{
    struct dirEntry dir;
    dir.maxFiles = 10;
    dir.maxFileSize = 256;
    snprintf(dir.dirName, 8, "logs");

    Log_Debug("Add directory 'logs'\n");
    assert(FS_AddDirectory(&dir) == 0);

    char filename[16];
    char fileContent[300];

    dir.maxFiles = 5;
    dir.maxFileSize = 60;
    snprintf(dir.dirName, 8, "info");
    Log_Debug("Add directory 'info'\n");
    assert(FS_AddDirectory(&dir) == 0);

    Log_Debug("Write 'info' files\n");
    // put some files in the 'info' directory.
    for (size_t x = 0; x < 5; x++)
    {
        snprintf(filename, 16, "info%04d.txt", x);
        snprintf(fileContent, 300, "%02d: %s", x, fox);
        assert(FS_WriteFile("info", filename, fileContent, strlen(fileContent)) == 0);
    }

    Log_Debug("Write log files\n");
    for (int x = 0; x < 17; x++)
    {
        snprintf(filename, 16, "log%04d.log", x);
        snprintf(fileContent, 300, "%04d: %s", x, loremText);
        assert(FS_WriteFile("logs", filename, fileContent, strlen(fileContent)) == 0);
        Log_Debug("Write '%s' - %d files in directory\n", filename,FS_GetNumberOfFilesInDirectory("logs"));
    }

    int numFiles = FS_GetNumberOfFilesInDirectory("logs");
    Log_Debug("%d files in 'logs' directory\n", numFiles);

    struct fileEntry fileInfo;
    for (size_t x = 0; x < numFiles; x++)
    {
        if (FS_GetFileInfoForIndex("logs", x, &fileInfo) == 0)
        {
            char* buffer = (char*)malloc(fileInfo.fileSize + 1);
            memset(buffer, 0x00, fileInfo.fileSize + 1);
            FS_ReadFileForIndex("logs", x, buffer, fileInfo.fileSize);
            Log_Debug("'%s' Size: %d, data: %s\n", fileInfo.fileName, fileInfo.fileSize, buffer);
            free(buffer);
        }
    }

    numFiles = FS_GetNumberOfFilesInDirectory("logs");
    Log_Debug("%d files in 'logs' directory\n", numFiles);

    Log_Debug("Delete oldest file\n");
    FS_DeleteOldestFileInDirectory("logs");

    numFiles = FS_GetNumberOfFilesInDirectory("logs");
    Log_Debug("%d files in 'logs' directory\n", numFiles);

    Log_Debug("Get oldest file in 'logs' directory\n");
    FS_GetOldestFileInfo("logs", &fileInfo);
    Log_Debug("File: %s\n", fileInfo.fileName);

    Log_Debug("Done\n");
}