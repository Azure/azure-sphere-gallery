/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <fileapi.h>
#include <stdint.h>

#include "..\..\..\littlefs\lfs.h"
#include "..\..\..\littlefs\lfs_util.h"

// uncomment the define below to show littlefs debug information (contents of blocks etc...)
// #define SHOW_DEBUG_INFO

// directory stuff.
char dirBuffer[MAX_PATH];
char outputFolder[MAX_PATH];
char workingFolder[MAX_PATH];

int storage_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);
int storage_program(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);
int storage_erase(const struct lfs_config* c, lfs_block_t block);
int storage_sync(const struct lfs_config* c);

HANDLE hFile = INVALID_HANDLE_VALUE;

static lfs_t lfs;

#define BLOCK_SIZE 512

struct lfs_config g_littlefs_config = {
.read = storage_read,
.prog = storage_program,
.erase = storage_erase,
.sync = storage_sync,
.read_size = BLOCK_SIZE,
.prog_size = BLOCK_SIZE,
.block_size = BLOCK_SIZE,
.block_count = 0,				// fill in later.
.block_cycles = 1000,
.cache_size = BLOCK_SIZE,
.lookahead_size = BLOCK_SIZE,
.name_max = 255
};

static void printSDBlock(uint8_t* buff, uintptr_t blocklen, unsigned blockID)
{
    uint8_t ascBuffer[20];
    int byteCount = 0;
    long offset = BLOCK_SIZE * blockID;
    bool newLine = true;

    printf("SD Card Data (block %u):\r\n", blockID);
    uintptr_t i;
    for (i = 0; i < blocklen; i++) {
        if (newLine)
        {
            printf("%04x: ", offset);
            newLine = false;
            offset += 16;
        }

        printf("%02x ", buff[i]);
        if (buff[i] > 0x20 && buff[i] < 0x80)
        {
            ascBuffer[byteCount] = buff[i];
        }
        else
        {
            ascBuffer[byteCount] = '.';
        }
        byteCount++;
        ascBuffer[byteCount] = 0x00;

        if (byteCount == 16)
        {
            printf("    %s\r\n", ascBuffer);
            newLine = true;
            byteCount = 0;
        }
    }

    if ((blocklen % 16) != 0) {
        printf("\r\n");
    }
    printf("\r\n");
}

static int storage_read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
#ifdef SHOW_DEBUG_INFO	
    printf("storage_read - block %d\n", block);
#endif

    DWORD numRead = 0;
    LARGE_INTEGER offset;
    offset.QuadPart = (block * BLOCK_SIZE);
    LARGE_INTEGER offset2;
    SetFilePointerEx(hFile, offset, &offset2, SEEK_SET);
    ReadFile(hFile, buffer, size, &numRead, NULL);

#ifdef SHOW_DEBUG_INFO	
    printSDBlock(buffer, size, block);
#endif

    return LFS_ERR_OK;
}

static int storage_program(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
#ifdef SHOW_DEBUG_INFO	
    printf("storage_program - block %d\n", block);
#endif

    return LFS_ERR_IO;
}

static int storage_erase(const struct lfs_config* c, lfs_block_t block)
{
#ifdef SHOW_DEBUG_INFO	
    printf("storage_erase - block %d\n", block);
#endif

	return LFS_ERR_OK;
}

static int storage_sync(const struct lfs_config* c)
{
#ifdef SHOW_DEBUG_INFO	
    printf("storage_sync\n");
#endif

        return LFS_ERR_OK;
}

void WalkDirectories(char* startPath)
{
    char SlashBuffer[MAX_PATH];
    strcpy(SlashBuffer, startPath);
    for (int x = 0; x < strlen(startPath); x++)
    {
        if (SlashBuffer[x] == '/')
            SlashBuffer[x] = '\\';
    }

    snprintf(workingFolder, MAX_PATH, "%s%s", outputFolder, SlashBuffer);

    if (!PathFileExistsA(workingFolder))
    {
        CreateDirectoryA(workingFolder, NULL);
    }

    char SubDirectory[MAX_PATH];
    char FilePath[MAX_PATH];

    lfs_dir_t dir;
    struct lfs_info info;
    int dirReadResult = 0;
    lfs_dir_open(&lfs, &dir, startPath);
    do {
        dirReadResult = lfs_dir_read(&lfs, &dir, &info);
        if (dirReadResult != 0)
        {
            if (info.name[0] != '.')    // not '.' or '..'
            {
                if (info.type == 2)     // directory
                {
                    printf("Directory: %s\n", info.name);
                    if (startPath[strlen(startPath)-1] == '/')
                        snprintf(SubDirectory, MAX_PATH, "%s%s", startPath, info.name);
                    else 
                        snprintf(SubDirectory, MAX_PATH, "%s/%s", startPath, info.name);
                    WalkDirectories(SubDirectory);
                }
                else
                {
                    printf("    %s - ", info.name);
                    snprintf(FilePath, MAX_PATH, "%s/%s", startPath, info.name);

                    struct lfs_info lfsInfo;
                    lfs_stat(&lfs, FilePath, &lfsInfo);
                    printf("%d bytes\n", lfsInfo.size);
                    if (lfsInfo.size > 0)
                    {
                        uint8_t* pFile = malloc(lfsInfo.size);
                        if (pFile == NULL)
                        {
                            printf("failed to allocate space for %s (%d bytes requested)\n", info.name, lfsInfo.size);
                        }
                        else
                        {
                            memset(pFile, 0x00, lfsInfo.size);
                            lfs_file_t datafile;
                            lfs_file_open(&lfs, &datafile, FilePath, LFS_O_RDONLY);
                            lfs_file_read(&lfs, &datafile, pFile, lfsInfo.size);
                            lfs_file_close(&lfs, &datafile);

                            // we've got the data... now write it.
                            snprintf(FilePath, MAX_PATH, "%s%s\\%s", outputFolder, startPath, info.name);
                            for (int x = 0; x < strlen(FilePath); x++)
                            {
                                if (FilePath[x] == '/')
                                    FilePath[x] = '\\';
                            }

#ifdef SHOW_DEBUG_INFO
                            printf("Writing File to... %s\n", FilePath);
#endif

                            DeleteFileA(SubDirectory);
                            HANDLE hOutFile = CreateFileA(FilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
                            DWORD numWritten = 0;
                            WriteFile(hOutFile, pFile, lfsInfo.size, &numWritten, NULL);
                            CloseHandle(hOutFile);
                            free(pFile);
                        }
                    }
                }
            }
        }
    } while (dirReadResult != 0);
    lfs_dir_close(&lfs, &dir);
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Provide a littlefs disk image\n");
        return -1;
    }

    hFile = CreateFileA(argv[1], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("Cannot open the file '%s'...\n", argv[1]);
        return -1;
    }

    GetCurrentDirectoryA(MAX_PATH, &dirBuffer[0]);

    strcpy(outputFolder, dirBuffer);
    strcat(outputFolder, "\\output");
    if (!PathFileExistsA(outputFolder))
    {
        CreateDirectoryA(outputFolder, NULL);
    }

#ifdef SHOW_DEBUG_INFO	    
    printf("current directory %s\n", dirBuffer);
#endif

    LARGE_INTEGER fileSize;
    GetFileSizeEx(hFile, &fileSize);
    DWORD fSize = (DWORD)(fileSize.QuadPart/512);

    printf("File has %d blocks\n", fSize);
    g_littlefs_config.block_count = fSize;		// setup total number of blocks.

    if (lfs_mount(&lfs, &g_littlefs_config) != LFS_ERR_OK) {
        printf("LittleFs mount failed\n");
        CloseHandle(hFile);
        return -1;
    }

    printf("LittleFs initialized!\n");

    WalkDirectories("/");

    lfs_unmount(&lfs);
	CloseHandle(hFile);

	return 0;
}