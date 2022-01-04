/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <stdint.h>
#include <stddef.h>

#define BLOCK_SIZE 512

struct dirEntry
{
	uint32_t	maxFiles;		// maxFiles * maxFileSize cannot exceed number of storage blocks
	uint32_t	maxFileSize;	// max file size in bytes
	uint8_t		dirName[8];		// directory name
};

struct fileEntry
{
	uint8_t fileName[32];		// filename
	uint32_t fileSize;			// fileSize is number of bytes for the file 'on disk'
	uint32_t datetime;			// system time
};

// Callbacks for user supplied read/write functions.
typedef int (*WriteBlockCallback)(uint32_t blockNumber, uint8_t* data, size_t size);
typedef int (*ReadBlockCallback)(uint32_t blockNumber, uint8_t* data, size_t size);

// File System API implementation

// Initialization
int FS_Init(WriteBlockCallback writeBlockCallback, ReadBlockCallback readBlockCallback, uint32_t totalBlocks);
int FS_Mount(void);
int FS_Format(void);

// Directory APIs
int FS_AddDirectory(struct dirEntry* dir);
int FS_GetNumberOfDirectories(void);
int FS_GetDirectoryByIndex(int dirNumber, struct dirEntry* dir);
int FS_GetDirectoryByName(char* dirName, struct dirEntry* dir);
int FS_GetNumberOfFilesInDirectory(char* dirName);

// File APIs
int FS_WriteFile(char* dirName, char* fileName, uint8_t* data, size_t size);
int FS_GetOldestFileInfo(char* dirName, struct fileEntry*);
int FS_ReadOldestFile(char* dirName, uint8_t* data, size_t size);
int FS_DeleteOldestFileInDirectory(char* dirName);

int FS_GetFileInfoForIndex(char* dirName, size_t fileIndex, struct fileEntry* fileInfo);
int FS_ReadFileForIndex(char* dirName, size_t fileIndex, uint8_t* data, size_t size);




