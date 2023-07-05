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
#include <assert.h>

#include "littlefs/lfs.h"
#include "curlFunctions.h"
#include "encrypted_storage.h"

int main(void)
{
    static lfs_t lfs;
    static lfs_file_t file;
    static char* content = 
"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
"Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure"
"dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non"
"proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

    static char buffer[512] = { 0 };

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
    assert(lfs_file_write(&lfs, &file, content, strlen(content)) == strlen(content));
    Log_Debug("Seek to the start of the file\n");
    assert(lfs_file_seek(&lfs, &file, 0, LFS_SEEK_SET) == 0);
    Log_Debug("Read from the file\n"); 
    int read_len = lfs_file_read(&lfs, &file, buffer, sizeof(buffer));
    Log_Debug("Read %d bytes of content = %s\n", read_len, buffer);
    assert(read_len == strlen(content));
    Log_Debug("Close the file\n");
    assert(lfs_file_close(&lfs, &file) == LFS_ERR_OK);

    cleanupCurl();

    while (true);   // spin...

    return 0;
}