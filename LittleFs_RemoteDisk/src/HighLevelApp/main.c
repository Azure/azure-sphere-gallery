/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/networking.h>
#include <signal.h>

#include "remoteDiskIO.h"

#include "littlefs/lfs.h"
#include "littlefs/lfs_util.h"
#include <assert.h>
#include "curlFunctions.h"

// 4MB Storage.
#define PAGE_SIZE     (256)
#define SECTOR_SIZE   (16 * PAGE_SIZE)
#define BLOCK_SIZE    (16 * SECTOR_SIZE)
#define TOTAL_SIZE    (64 * BLOCK_SIZE)

static int storage_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);
static int storage_program(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);
static int storage_erase(const struct lfs_config* c, lfs_block_t block);
static int storage_sync(const struct lfs_config* c);

static lfs_t lfs;
static lfs_file_t file;

char* content = "Test";
char buffer[512] = { 0 };

const struct lfs_config g_littlefs_config = {
    // block device operations
    .read = storage_read,
    .prog = storage_program,
    .erase = storage_erase,
    .sync = storage_sync,
    .read_size = 16,
    .prog_size = PAGE_SIZE,
    .block_size = SECTOR_SIZE,
    .block_count = TOTAL_SIZE / SECTOR_SIZE,
    .block_cycles = 1000000,
    .cache_size = PAGE_SIZE,
    .lookahead_size = 16,
};

static int storage_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
    uint8_t* data = readBlockData(off, size);

    if (data != NULL)
    {
        memcpy(buffer, data, size);
    }

    return LFS_ERR_OK;
}

static int storage_program(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
    writeBlockData(buffer, size, off);

    return LFS_ERR_OK;
}

static int storage_erase(const struct lfs_config* c, lfs_block_t block)
{
    return LFS_ERR_OK;
}

static int storage_sync(const struct lfs_config* c)
{
    return LFS_ERR_OK;
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

    if (lfs_mount(&lfs, &g_littlefs_config) != LFS_ERR_OK) {
        Log_Debug("Format and Mount\n");
        assert(lfs_format(&lfs, &g_littlefs_config) == LFS_ERR_OK);
        assert(lfs_mount(&lfs, &g_littlefs_config) == LFS_ERR_OK);
    }

    // do something with the file system
    Log_Debug("Open File 'test.txt'\n");
    assert(lfs_file_open(&lfs, &file, "test.txt", LFS_O_RDWR | LFS_O_CREAT) == LFS_ERR_OK);
    Log_Debug("Write to the file\n");
    assert(lfs_file_write(&lfs, &file, content, strlen(content)) == 4);
    Log_Debug("Seek to the start of the file\n");
    assert(lfs_file_seek(&lfs, &file, 0, LFS_SEEK_SET) == 0);
    Log_Debug("Read from the file\n"); 
    assert(lfs_file_read(&lfs, &file, buffer, 512) == 4);
    Log_Debug("Read content = %s\n", buffer);
    Log_Debug("Close the file\n");
    assert(lfs_file_close(&lfs, &file) == LFS_ERR_OK);

    cleanupCurl();

    while (true);   // spin...

    return 0;
}