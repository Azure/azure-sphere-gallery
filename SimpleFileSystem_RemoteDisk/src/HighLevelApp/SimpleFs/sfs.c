/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "sfs.h"
#include "sfs.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
	#define Log_Debug	printf
#else
#include <strings.h>
#include <applibs/log.h>
#endif

// enable this for file system runtime trace information
// #define FS_TRACE

// callbacks to user functions to read/write blocks
static WriteBlockCallback _writeBlockCallback = NULL;
static ReadBlockCallback _readBlockCallback = NULL;
static uint32_t _totalBlocks = 0;	// Number of 512 byte blocks on the storage media

static uint8_t rootBlock[BLOCK_SIZE];

#define FS_SIG	0xffaa		// magic number to determine whether the root block is 'Simple FS'

// root block, this stores the magic sig, total number of blocks (512 bytes per block), and number of provisioned directories
struct root	// root is 16 bytes long (makes 32 byte alignment of directories easy to read)
{
	uint16_t sig;
	uint16_t numDirectories;
	uint32_t storageNumBlocks;
	uint8_t reserved[8];
};

static struct directory* dirList = (struct directory*)(rootBlock + sizeof(struct root));

// root block is 512 bytes (BLOCK_SIZE)
// sig+size+numDirs+reserved is 16 bytes
// allows for 15 directories of 32 bytes each

// full directory information (maxFiles, MaxFileSize, dirName are exposed as dirEntry to users
struct directory	// directory is 32 bytes
{
	uint32_t maxFiles;			// max number of files
	uint32_t maxFileSize;		// max file size in bytes
	uint8_t dirName[8];			// directory name 
	uint32_t firstBlock;		// physical block address for start of directory files
	uint32_t head;				// 0 based block index into numberOfBlocks
	uint32_t tail;				// 0 based block index into numberOfBlocks
	uint32_t dirFull;			// 0 directory is not full, 1 directory is full
};

// Determine state of File System initialization - can be initialized but not mounted (init, format, then mount)
static bool _init = false;
static bool _mount = false;

// Internal helper functions
static int FS_GetFullDirectory(int dirOffset, struct directory* dir);
static uint32_t FS_GetNumberOfBlocksPerFile(struct directory* dir);
static int FS_WriteDirectoryToRoot(struct directory* dir, int directoryIndex);

static bool FS_IsFileSystemReady(void)
{
	if (_init && _mount)
		return true;

	return false;
}

/// <summary>
/// Initialize SimpleFs, this stores the read/write callbacks, attempts to load the root block, and verifies the root block signature
/// </summary>
/// <param name="writeBlockCallback"></param>
/// <param name="readBlockCallback"></param>
/// <returns></returns>
int FS_Init(WriteBlockCallback writeBlockCallback, ReadBlockCallback readBlockCallback, uint32_t totalBlocks)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	_writeBlockCallback = writeBlockCallback;
	_readBlockCallback = readBlockCallback;
	_totalBlocks = totalBlocks;

	// Do we have callbacks and a positive number of blocks, if not, bail
	if (_readBlockCallback == NULL || _writeBlockCallback == NULL || _totalBlocks == 0)
	{
		return -1;
	}

	_init = true;
	return 0;
}

int FS_Mount(void)
{
	if (!_init)
	{
		// Not initialized, bail.
		return -1;
	}

	// read the root block
	int result = _readBlockCallback(0, rootBlock, BLOCK_SIZE);
	if (result != 0)
	{
		return -1;
	}

	struct root* pRoot = (struct root*)rootBlock;
	if (pRoot->sig != FS_SIG || pRoot->storageNumBlocks == 0)
		return -1;

	_mount = true;
	return 0;
}

/// <summary>
/// Formats the underlying file system, it's assumed that Init is already called, this function writes the root block, other sectors are not impacted
/// </summary>
/// <param name="totalBlocks"></param>
/// <returns></returns>
int FS_Format(void)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!_init)
	{
		return -1;
	}

	struct root newRoot;
	newRoot.sig = 0xffaa;
	newRoot.storageNumBlocks = _totalBlocks;
	newRoot.numDirectories = 0;
	memset(newRoot.reserved, 0x00, sizeof(newRoot.reserved));

	memset(rootBlock, 0x00, BLOCK_SIZE);
	memcpy(rootBlock, &newRoot, sizeof(struct root));

	if (!_writeBlockCallback(0, rootBlock, BLOCK_SIZE) == 0)
	{
		return -1;
	}

	return 0;
}

/// <summary>
/// Adds a directory to the root block (up to 16 directories can be added)
/// </summary>
/// <param name="dir"></param>
/// <returns></returns>
int FS_AddDirectory(struct dirEntry* dir)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	// check to see whether the directory already exists.
	struct dirEntry existingDir;
	int result=FS_GetDirectoryByName(dir->dirName, &existingDir);
	if (result == 0)
	{
		return -1;
	}

	struct directory newDir;

	memset(&newDir, 0x00, sizeof(struct directory));
	// copy the public bits in
	memcpy(&newDir, dir, sizeof(struct dirEntry));

	// setup directory, add to root block and write 
	struct root* pRoot = (struct root*)&rootBlock;
	if (pRoot->numDirectories + 1 * sizeof(struct directory) + sizeof(struct root) > BLOCK_SIZE)
		return -1;		// not enough room for a new directory.

	uint32_t nextBlock = 0;
	// calc next start block position
	for (int x = 0; x < pRoot->numDirectories; x++)
	{
		// pDir = ; // FS_GetFullDirectory(x);
		uint32_t numBlocks = FS_GetNumberOfBlocksPerFile(&newDir);
		nextBlock += numBlocks;
	}

	nextBlock++;	// skip the root block

	// calculate the number of blocks needed for file (file size/512 - rounded up) + 1 block per file for header.
	// Get NumBlocks per file
	uint32_t numFileBlocks = FS_GetNumberOfBlocksPerFile(&newDir);

	// add the file descriptor block to the number of data blocks per file
	numFileBlocks++;

	// get total number of blocks for the directory
	uint32_t numBlocks = numFileBlocks * newDir.maxFiles;

	// set this directories first block position
	newDir.firstBlock = nextBlock;

	// if number of blocks is greater than storage, then error/return
	if (numBlocks + nextBlock > pRoot->storageNumBlocks)
		return -1;

	// copy in the directory information
	memcpy(&dirList[pRoot->numDirectories], &newDir, sizeof(struct directory));

	pRoot->numDirectories++;	// increment the number of directories
	result = _writeBlockCallback(0, rootBlock, BLOCK_SIZE);

	if (result == -1)
	{
		return -1;
	}

	return 0;
}

/// <summary>
/// Function to return the 'short' directory info (defined in sfc.h) to a user (doesn't contain head/tail, full, etc...)
/// </summary>
int FS_GetDirectoryByIndex(int dirNumber, struct dirEntry* dir)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	struct root* pRoot = (struct root*)rootBlock;
	if (dirNumber > pRoot->numDirectories)
		return -1;

	memcpy(dir, &dirList[dirNumber], sizeof(struct dirEntry));

	return 0;
}

/// <summary>
/// FS_GetDirectoryByName - Retreives directory information (name, max files, max file size)
/// </summary>
/// <param name="dirName"></param>
/// <param name="dir"></param>
/// <returns></returns>
int FS_GetDirectoryByName(char* dirName, struct dirEntry* dir)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	int numDirs = FS_GetNumberOfDirectories();
	if (numDirs == -1 || numDirs == 0)
		return -1;

	struct dirEntry sDir;
	for (int x = 0; x < numDirs; x++)
	{
		FS_GetDirectoryByIndex(x, &sDir);

		int cmpResult = 0;
#ifdef _WIN32
		cmpResult=_strnicmp(dirName, sDir.dirName, strlen(dirName));
#else
		cmpResult = strncasecmp(dirName, sDir.dirName, strlen(dirName));
#endif

		if (cmpResult == 0)
		{
			memcpy(dir, &sDir, sizeof(struct dirEntry));
			return 0;
		}
	}

	return -1;
}
/// <summary>
///  helper function to get the full directory information for a directory index
/// </summary>

static int FS_GetFullDirectory(int dirOffset, struct directory* dir)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	struct root* pRoot = (struct root*)rootBlock;
	if (dirOffset > pRoot->numDirectories)
		return -1;

	memcpy(dir, &dirList[dirOffset], sizeof(struct directory));

	return 0;
}

/// <summary>
///  returns the number of provisioned directories
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int FS_GetNumberOfDirectories(void)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	struct root* pRoot = (struct root*)rootBlock;
	return pRoot->numDirectories;
}

/// <summary>
/// Internal helper function to return a directory index based on name
/// </summary>
static int FS_GetDirectoryFromName(char* name, int* directoryIndex, struct dirEntry* dir)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	struct root* pRoot = (struct root*)rootBlock;
	if (pRoot->numDirectories == 0)
		return -1;

	int cmpResult = 0;

	for (uint16_t x = 0; x < pRoot->numDirectories; x++)
	{
#ifdef _WIN32
		cmpResult = _strnicmp(dirList[x].dirName, name, strlen(name));
#else
		cmpResult = strncasecmp(dirList[x].dirName, name, strlen(name));
#endif

		if (cmpResult == 0)
		{
			*directoryIndex = x;	// needed to update the root block after writes
			memcpy(dir, &dirList[x], sizeof(struct dirEntry));
			return 0;
		}
	}

	return 0;
}

/// <summary>
/// Helper function to update the root block with the latest directory information
/// </summary>
/// <param name="dir"></param>
/// <param name="directoryIndex"></param>
static int FS_WriteDirectoryToRoot(struct directory* dir, int directoryIndex)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	memcpy(&dirList[directoryIndex], dir, sizeof(struct directory));
	int result = _writeBlockCallback(0, rootBlock, BLOCK_SIZE);

	return result;
}

/// <summary>
/// Gets the raw number of blocks per file, doesn't include the file header block (first block of the file)
/// </summary>
/// <param name="dir"></param>
/// <returns></returns>
static uint32_t FS_GetNumberOfBlocksPerFile(struct directory* dir)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return 0;
	}

	uint32_t numBlocks = dir->maxFileSize / BLOCK_SIZE;
	if (dir->maxFileSize % BLOCK_SIZE != 0)
		numBlocks++;

	return numBlocks;
}

/// <summary>
/// Get the number of files for a named directory
/// </summary>
/// <param name="dirName"></param>
/// <returns></returns>
int FS_GetNumberOfFilesInDirectory(char* dirName)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	int dirIndex = 0;
	struct dirEntry pDirEntry;
	int result = FS_GetDirectoryFromName(dirName, &dirIndex, &pDirEntry);
	if (result == -1)
		return -1;

	struct directory pDir;
	FS_GetFullDirectory(dirIndex, &pDir);

	// if directory is full then return the max number of items.
	if (pDir.dirFull == 1)
	{
		return (int)pDir.maxFiles;
	}

	int numberOfFiles = 0;

	if (pDir.head >= pDir.tail)
	{
		numberOfFiles = (int)(pDir.head - pDir.tail);
	}
	else
	{
		numberOfFiles = (int)(pDir.maxFiles + pDir.head - pDir.tail);
	}

	return numberOfFiles;
}

/// <summary>
/// Gets the file info for the oldest file in a directory (FIFO)
/// </summary>
/// <param name="dirName">directory name to get oldest file information</param>
/// <returns> NULL on error, fileEntry structure (pointer) on success </returns>
int FS_GetOldestFileInfo(char* dirName, struct fileEntry* pFileEntry)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	uint8_t fileHeader[BLOCK_SIZE];

	int numfilesInDir = FS_GetNumberOfFilesInDirectory(dirName);
	if (numfilesInDir == 0)
		return -1;

	int dirIndex = 0;

	struct dirEntry pDirEntry;
	int result = FS_GetDirectoryFromName(dirName, &dirIndex, &pDirEntry);
	if (result == -1)
		return -1;

	struct directory pDir;
	FS_GetFullDirectory(dirIndex, &pDir);

	uint32_t numBlocksPerFile = FS_GetNumberOfBlocksPerFile(&pDir);
	result = _readBlockCallback(pDir.firstBlock + (pDir.tail * (numBlocksPerFile + 1)), fileHeader, BLOCK_SIZE);
	if (result == -1)
		return -1;

	struct file* pFile = (struct file*)fileHeader;

	memcpy(pFileEntry, pFile, sizeof(struct fileEntry));

	return 0;
}

/// <summary>
/// Reads the oldest file in the file system
/// call FS_GetOldestFileInfo to get file size information
/// File system is based on a circular buffer FIFO style
/// There's no need for random access to files in the middle of the circular buffer
/// </summary>
/// <param name="dirName"></param>
/// <param name="data"></param>
/// <param name="size"></param>
/// <returns> -1 on error, 0 on success </returns>
int FS_ReadOldestFile(char* dirName, uint8_t* data, size_t size)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	uint8_t fileHeader[BLOCK_SIZE];

	int numfilesInDir = FS_GetNumberOfFilesInDirectory(dirName);
	if (numfilesInDir == -1)
		return -1;

	int dirIndex = 0;
	struct dirEntry pDirEntry;
	int result = FS_GetDirectoryFromName(dirName, &dirIndex, &pDirEntry);
	if (result == -1)
		return -1;

	struct directory pDir;
	FS_GetFullDirectory(dirIndex, &pDir);

	uint32_t numBlocksPerFile = FS_GetNumberOfBlocksPerFile(&pDir);		// doesn't include the file header block

	// read the header of the file
	uint32_t fileHeaderBlock = pDir.firstBlock + (pDir.tail * (numBlocksPerFile + 1));
	uint32_t fileDataBlock = fileHeaderBlock + 1;

	result = _readBlockCallback(fileHeaderBlock, fileHeader, BLOCK_SIZE);
	if (result == -1)
		return -1;

	struct fileEntry* pFile = (struct fileEntry*)fileHeader;

	// requesting more bytes than are in the file.
	if (size > pFile->fileSize)
		return -1;

	uint32_t numBlocksToRead = pFile->fileSize / BLOCK_SIZE;
	if (pFile->fileSize % BLOCK_SIZE != 0)
		numBlocksToRead++;

	uint32_t bytesToRead = pFile->fileSize;

	uint8_t block[BLOCK_SIZE];

	for (uint32_t x = 0; x < numBlocksToRead; x++)
	{
		_readBlockCallback(fileDataBlock + x, block, BLOCK_SIZE);
		memcpy(data + (x * BLOCK_SIZE), block, bytesToRead >= BLOCK_SIZE ? BLOCK_SIZE : bytesToRead);
		bytesToRead -= BLOCK_SIZE;
	}

	return 0;
}

/// <summary>
/// Deletes the oldest file in the circular buffer
/// </summary>
/// <param name="dirName"></param>
/// <returns> returns -1 on error (no files, directory doesn't exist, returns 0 on success</returns>
int FS_DeleteOldestFileInDirectory(char* dirName)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	int dirIndex = 0;
	struct dirEntry pDirEntry;
	int result = FS_GetDirectoryFromName(dirName, &dirIndex, &pDirEntry);
	if (result == -1)
		return -1;

	struct directory pDir;
	FS_GetFullDirectory(dirIndex, &pDir);

	int numFiles = FS_GetNumberOfFilesInDirectory(dirName);

	// no files, nothing to delete
	if (numFiles == 0)
		return -1;

	// mark the directory as 'not full'
	pDir.dirFull = 0;
	// no need to actually delete the file, simply move the head/tail pointers in the directory
	pDir.tail = (pDir.tail + 1) % pDir.maxFiles;

	result = FS_WriteDirectoryToRoot(&pDir, dirIndex);

	return result;
}

/// <summary>
/// Writes a file to the end of the directory circular buffer. 
/// Wraps around if the file system is full
/// </summary>
/// <param name="dirName"></param>
/// <param name="fileName"></param>
/// <param name="data"></param>
/// <param name="size"></param>
/// <returns> -1 for error, 0 for success </returns>
int FS_WriteFile(char* dirName, char* fileName, uint8_t* data, size_t size)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	int currentDirectoryIndex = 0;
	struct fileEntry newFile;

	struct dirEntry pDirEntry;
	int result = FS_GetDirectoryFromName(dirName, &currentDirectoryIndex, &pDirEntry);
	if (result == -1)
		return -1;

	// Get the directory information
	struct directory currentDir;
	FS_GetFullDirectory(currentDirectoryIndex, &currentDir);

	// if the file is too large, error/return
	if (size > currentDir.maxFileSize)
	{
		return -1;
	}

	// calculate the number of blocks needed to write the file.
	uint32_t numBlocksPerFile = FS_GetNumberOfBlocksPerFile(&currentDir);	// this is the number of blocks for the data, not including the file header
	uint32_t fileHeaderBlock = (currentDir.head * (numBlocksPerFile + 1)) + currentDir.firstBlock;	// physical block number for the file header
	uint32_t fileDataBlock = fileHeaderBlock + 1;	// point to the first block of the file data

	memset(&newFile, 0x00, sizeof(struct fileEntry));
	snprintf(newFile.fileName,32, fileName);
	newFile.fileSize = size;	// set the number of bytes

	// add date time to file info
	time_t fileTime=time(NULL);
	newFile.datetime = (uint32_t)fileTime;

	// setup the file header block
	uint8_t fileHeader[BLOCK_SIZE];

	memset(fileHeader, 0x00, BLOCK_SIZE);
	memcpy(fileHeader, &newFile, sizeof(struct fileEntry));

	// determine how many chunks we need to write.
	size_t numWriteBlocks=size / BLOCK_SIZE;
	if (size % BLOCK_SIZE != 0)
		numWriteBlocks++;

#ifdef FS_TRACE
	Log_Debug("----------------------------------\n");
	Log_Debug("Directory '%s', file '%s'\n", dirName, fileName);
	Log_Debug("tail %d, head %d\n", currentDir.tail, currentDir.head);
	Log_Debug("firstBlock %d, numBlocks %d (LastBlock %d)\n", fileHeaderBlock, numWriteBlocks, (fileHeaderBlock + numWriteBlocks));
#endif

	uint8_t writeBuffer[BLOCK_SIZE];

	size_t remain = size;

	// write the file
	for (int x = 0; x < (int)numWriteBlocks; x++)
	{
		memset(writeBuffer, 0x00, BLOCK_SIZE);
		if (remain >= BLOCK_SIZE)
		{
			memcpy(writeBuffer, data + (x * BLOCK_SIZE), BLOCK_SIZE);
		}
		else
		{
			memcpy(writeBuffer, data + (x * BLOCK_SIZE), remain);
		}
		remain = remain - BLOCK_SIZE;

		_writeBlockCallback(fileDataBlock++, writeBuffer, BLOCK_SIZE);
	}

	// Write the file header.
	_writeBlockCallback(fileHeaderBlock, fileHeader, BLOCK_SIZE);

	// update the directory information/root block
	// if we've wrapped around, and are overwriting existing data.

	if (currentDir.dirFull == 1)
	{
		currentDir.tail= (currentDir.tail+1) % currentDir.maxFiles;
	}

	currentDir.head = (currentDir.head + 1) % currentDir.maxFiles;

	if (currentDir.head == currentDir.tail)
		currentDir.dirFull = 1;

	result = FS_WriteDirectoryToRoot(&currentDir, currentDirectoryIndex);

	return result;
}

/// <summary>
/// Gets the File Info for a given file index into a directory (zero based)
/// </summary>
/// <param name="dirName"></param>
/// <param name="fileIndex"></param>
/// <param name="fileInfo"></param>
/// <returns></returns>
int FS_GetFileInfoForIndex(char* dirName, size_t fileIndex, struct fileEntry* fileInfo)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	// does the directory exist?
	int dirIndex = 0;
	struct dirEntry pDirEntry;
	int result = FS_GetDirectoryFromName(dirName, &dirIndex, &pDirEntry);
	if (result == -1)
		return -1;

	struct directory pDir;
	FS_GetFullDirectory(dirIndex, &pDir);

	// Get the number of files in the directory, if zero, bail
	int numFiles = FS_GetNumberOfFilesInDirectory(dirName);

	// no files, nothing to delete
	if (numFiles == 0)
		return -1;

	// if fileIndex is out of bounds
	if (fileIndex >= (size_t)numFiles)
		return -1;

	// fixup overflow.
	size_t readIndex = (fileIndex + pDir.tail) % pDir.maxFiles;

	uint8_t fileHeader[BLOCK_SIZE];
	uint32_t numBlocksPerFile = FS_GetNumberOfBlocksPerFile(&pDir);
	result = _readBlockCallback(pDir.firstBlock + (readIndex * (numBlocksPerFile + 1)), fileHeader, BLOCK_SIZE);
	if (result == -1)
		return -1;

	struct file* pFile = (struct file*)fileHeader;
	memcpy(fileInfo, pFile, sizeof(struct fileEntry));

	return 0;
}

int FS_ReadFileForIndex(char* dirName, size_t fileIndex, uint8_t* data, size_t size)
{
#ifdef SHOW_FUNCTION_TRACE
	Log_Debug(">>> %s\n", __func__);
#endif

	// If we haven't been initialized, bail.
	if (!FS_IsFileSystemReady())
	{
		return -1;
	}

	// does the directory exist?
	int dirIndex = 0;
	struct dirEntry pDirEntry;
	int result = FS_GetDirectoryFromName(dirName, &dirIndex, &pDirEntry);
	if (result == -1)
		return -1;

	struct directory pDir;
	FS_GetFullDirectory(dirIndex, &pDir);

	// Get the number of files in the directory, if zero, bail
	int numFiles = FS_GetNumberOfFilesInDirectory(dirName);

	// no files, nothing to delete
	if (numFiles == 0)
		return -1;

	// if fileIndex is out of bounds
	if (fileIndex >= (size_t)numFiles)
		return -1;

	uint32_t numBlocksPerFile = FS_GetNumberOfBlocksPerFile(&pDir);		// doesn't include the file header block

	// fixup overflow.
	size_t readIndex = (fileIndex + pDir.tail) % pDir.maxFiles;

	// read the header of the file
	uint32_t fileHeaderBlock = pDir.firstBlock + (readIndex * (numBlocksPerFile + 1));
	uint32_t fileDataBlock = fileHeaderBlock + 1;

	uint8_t fileHeader[BLOCK_SIZE];
	result = _readBlockCallback(fileHeaderBlock, fileHeader, BLOCK_SIZE);
	if (result == -1)
		return -1;

	struct fileEntry* pFile = (struct fileEntry*)fileHeader;

	// requesting more bytes than are in the file.
	if (size > pFile->fileSize)
		return -1;

	uint32_t numBlocksToRead = pFile->fileSize / BLOCK_SIZE;
	if (pFile->fileSize % BLOCK_SIZE != 0)
		numBlocksToRead++;

	uint32_t bytesToRead = pFile->fileSize;

	uint8_t block[BLOCK_SIZE];

	for (uint32_t x = 0; x < numBlocksToRead; x++)
	{
		_readBlockCallback(fileDataBlock + x, block, BLOCK_SIZE);
		memcpy(data + (x * BLOCK_SIZE), block, bytesToRead >= BLOCK_SIZE ? BLOCK_SIZE : bytesToRead);
		bytesToRead -= BLOCK_SIZE;
	}

	return 0;
}

