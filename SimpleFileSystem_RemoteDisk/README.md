# Simple File System Remote Disk project

The goal of this project is to show how to add file system support to an Azure Sphere project, in this case using a simple file system and 'remote storage', the sample project can be extended to talk to physical hardware.

The simple file system is designed to:
- Minimize write/read operations to underlying storage
- Support up to 15 directories (one level deep from root) with a FIFO and quota based (circular buffer/storage) model

## Contents

| File/folder | Description |
|-------------|-------------|
| `src\HighLevelApp`       | Azure Sphere Sample App source code |
| `src\PyDiskHost`       | Python app that hosts a 4MB Remote Disk |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://docs.microsoft.com/en-us/azure-sphere/install/overview) for more information).

Note that the Azure Sphere High Level application is configured for the 21.01 SDK release.

## How to use

**Simple File System Storage layout**

The simple file system is designed to use 512 byte blocks, the first block in storage (root block) contains the file system signature, the number of provisioned directories (up to 15 can be provisioned), the number of storage blocks used by the complete file system, and then the list of directories (each directory entry uses 32 bytes), a directory entry contains the maximum number of files for the directory, the maximum file size of a file in the directory, the directory name, the storage block number for the first usable directory block, the number of blocks used by the directory, and the head/tail indexes for files in the directory.

The simple file system keeps a cached copy of the root block in memory, requests for the number of files in a directory, the index of a directory, or getting a directory by name uses the cached copy of the root block.

The root block is only written (and the cached copy updated) when the root block is updated - this can occur when a new directory is created, a file is written to a directory, or a file is deleted.

The file system supports a First In/First Out write/read/delete model, the oldest files in a directory are overwritten if the directory quota overflows (based on max files for the directory, which is allocated when the directory is created).

Each file has a file header block (containing file name, date/time created, and file size) and at least one file data block (the number of data blocks per file is based on the max file size for files in the directory). If a developer creates a directory that defines the max file size as being 255 bytes this would require 1 file header block, and one data block (only using 255 of the 512 byte block). For a directory that defines the max file size as 1KB this would require one file header block and two data blocks.

Note that simple file system functions return -1 on failure, other return values could indicate success or a value associated with a specific API call (for example returning the number of files in a directory, which could be 0 to max files for the directory).

### Simple File System Initialization functions

The simple file system supports three initialization functions:

```cpp
int FS_Init(WriteBlockCallback writeBlockCallback, ReadBlockCallback readBlockCallback, uint32_t totalBlocks);
int FS_Mount(void);
int FS_Format(void);
```

**FS_Init** initializes the simple file system (call this function first), you pass callback functions for reading/writing storage blocks, and the total number of blocks (512 bytes per block) supported by your storage. Note that calling other functions before FS_Init will return -1 for all calls.

**FS_Mount** will attempt to mount the simple file system - this reads the root block which contains the file system signature, total number of blocks for the file system, and a list of provisioned directories (up to 15 directories are supported). If FS_Mount fails (return of -1) you might need to format the file system (write a clean root block).

**FS_Format** will write a new root block, the root block will contain a file system signature and the total number of blocks from the FS_Init call, and empty directory data. Note that formatting only impacts the root block.

### **Directory functions**

The simple file system supports creating/adding directories, getting a count of directories, getting directory information (based on index, or name), and getting the number of files in a directory. It's assumed that a directory layout for an embedded system is fixed. If you want to create a different layout then you should format the storage media and create a new layout (possibly reading all remaining files and sending to your back end for processing before formatting).

```cpp
int FS_AddDirectory(struct dirEntry* dir);
int FS_GetNumberOfDirectories(void);
int FS_GetDirectoryByIndex(int dirNumber, struct dirEntry* dir);
int FS_GetDirectoryByName(char* dirName, struct dirEntry* dir);
int FS_GetNumberOfFilesInDirectory(char* dirName);
```

**FS_AddDirectory** adds a new directory to the root block (note that 'data' and 'Data' are considered to be the same directory name, attempting to add 'data' and 'Data' will fail to add the second directory), the function will return error (-1) if the file system is not initialized, if the directory already exists, and the addition of the directory will exceed the total number of blocks allocated by the file system (total file system blocks are assigned in FS_Init).

Several of the directory functions use a `struct dirEntry` which is defined in **sfs.h** - here's the definition.

```cpp
struct dirEntry
{
	uint32_t maxFiles;
	uint32_t maxFileSize;
	uint8_t dirName[8];
};
```

**FS_GetNumberOfDirectories** returns -1 if the file system hasn't been initialized, otherwise returns the number of directories (might be 0 if the file system has been initialized, and formatted, but no directories are added).

**FS_GetDirectortyByName** returns -1 if the file system hasn't been initialized or the directory doesn't exist, returns 0 if the directory exists and also fills in the `dirEntry` structure.

**FS_GetDirectoryByIndex** returns -1 if the index doesn't exist, 0 if the directory exists, also fills in the `dirEntry` structure.

**FS_GetNumberOfFilesInDirectory** returns -1 if the directory doesn't exist, otherwise returns the number of files in a directory (could be 0).

### File functions

The simple file system supports a First In/First Out (FIFO) model for storing/retrieving file data, this is ideal for an embedded system that wants to store data when offline, and then forward to a back-end when online. To support the FIFO model the simple file system includes APIs to create/write to a file, read the oldest file (including reading the file information to determine size or date/time created), delete the oldest file, and also walk files in a directory based on file index (this might be used to read a collection of data within a date/time range). Note that the simple file system supports deleting the oldest file, not deleting arbitrary files within a directory.

```cpp
int FS_WriteFile(char* dirName, char* fileName, uint8_t* data, size_t size);
int FS_GetOldestFileInfo(char* dirName, struct fileEntry*);
int FS_ReadOldestFile(char* dirName, uint8_t* data, size_t size);
int FS_DeleteOldestFileInDirectory(char* dirName);
int FS_GetFileInfoForIndex(char* dirName, size_t fileIndex, struct fileEntry* fileInfo);
int FS_ReadFileForIndex(char* dirName, size_t fileIndex, uint8_t* data, size_t size);
```

**FS_WriteFile** returns -1 if the file system isn't initialized, if writing to the underlying media fails (this is determined by the user callback functions), or the file size is larger than the directory supports. FS_WriteFile writes the file to the head of the FIFO/circular buffer, this may overwrite the oldest file in the directory.

**FS_GetOldestFileInfo** returns -1 if the file system isn't initialized, the directory doesn't exist, or there aren't any files in the directory. The function fills in a `struct fileEntry` (defined below), content will be based on the oldest file in the directory, this contains the filename, file size (in bytes), and the date/time the file was created.

```cpp
struct fileEntry
{
	uint8_t fileName[32];
	uint32_t fileSize;
	uint32_t datetime;
};
```

**FS_ReadOldestFile** returns -1 if the file system isn't initialized, the directory doesn't exist, the directory is empty, or read of the underlying media failed (this is determined by the user callback functions), or the file size is larger than the supplied buffer. Returns 0 on a successful read.

**FS_DeleteOldestFileInDirectory** returns -1 if the file system isn't initialized, the directory doesn't exist, there aren't any files in the directory, or the write to underlying media fails (this is determined by the user callback functions). Returns 0 on success.

**FS_GetFileInfoForIndex** returns -1 if the file system isn't initialized, the directory doesn't exist, there aren't any files in the directory, the index number is greater than the number of files in the directory, or reading the underlying media fails. Returns 0 on success and also fills in the user supplied `struct fileEntry`. Note, call the FS_GetNumberOfFilesInDirectory API to get the file count for a directory, then then index into the files.

**FS_ReadFileForIndex** returns -1 if the file system isn't initialized, the directory doesn't exist, there aren't any files in the directory, the index number is greater than the number of files in the directory, the file size is larger than the supplied buffer, or reading the underlying media fails. Returns 0 on success. Call the FS_ReadFileForIndex API to get the file size before allocating memory to read the file.

**Python Remote Storage app** 
The project contains a Python Flask application (PyDiskHost.py) that supports 4MB storage (matching the defined storage layout of the high-level Azure Sphere application) - The Python application supports HTTP Get (read), and HTTP Post (Write) functions - the 4MB storage is supported by an in-memory bytearray (but could be easily modified to use a file on disk). The Python app is configured to use port 5000.

Start the Python application before running the Azure Sphere application.

**Azure Sphere High Level Application** 

The high level application sets up the storage size (line 64)

```cpp
// 4MB Storage.
assert(FS_Init(WriteBlock, ReadBlock, FILE_SYSTEM_BLOCKS) == 0);
```

The pointers to read, write, and storage size information are used to initialize SimpleFs in main.c.

The Azure Sphere high level application uses Curl to make HTTP Get/Post calls to read/write to Python remote storage.

You will need to modify the high level application app_manifest.json to provide the IP address of the PC running the Python application - modify **AllowedConnections** to include the PC host IP address, an example is below.

```cpp
{
  "SchemaVersion": 1,
  "Name": "LittleFs_HighLevelApp",
  "ComponentId": "1689d8b2-c835-2e27-27ad-e894d6d15fa9",
  "EntryPoint": "/bin/app",
  "CmdArgs": [],
  "Capabilities": {
    "AllowedConnections": ["192.168.1.65"]
  },
  "ApplicationType": "Default"
}
```

You will also need to set the IP address of the PC running the Python application in the projects CMakeLists.txt, modify `line 25` per the example below:

```makefile
# // TODO: define the PC HOST IP Address
add_compile_definitions(PC_HOST_IP="192.168.1.65")
```

The application will initialize and format the storage, create a file, write some test data, read the data and print to the debug console, and then close the file.

When the application runs you should see the following output in the Visual Studio/Code debug window:

```makefile
Initialize, Format and Mount
Add directory 'data'
Failed to add second 'data' directory (expected)
Failed to add 'Data' directory (expected)
Attempt to get 'logs' directory info, failed (expected)
Write 'lorem.txt'
First directory name by index: data
Get number of files in 'data' directory
1 files in 'data' directory
Read Oldest file in 'data' directory
'lorem.txt' Size: 122, data: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliquaDelete oldest file
0 files in 'data' directory
Add directory 'logs'
Add directory 'info'
Write 'info' files
Write log files
Write 'log0000.log' - 1 files in directory
Write 'log0001.log' - 2 files in directory
Write 'log0002.log' - 3 files in directory
Write 'log0003.log' - 4 files in directory
Write 'log0004.log' - 5 files in directory
Write 'log0005.log' - 6 files in directory
Write 'log0006.log' - 7 files in directory
Write 'log0007.log' - 8 files in directory
Write 'log0008.log' - 9 files in directory
Write 'log0009.log' - 10 files in directory
Write 'log0010.log' - 10 files in directory
Write 'log0011.log' - 10 files in directory
Write 'log0012.log' - 10 files in directory
Write 'log0013.log' - 10 files in directory
Write 'log0014.log' - 10 files in directory
Write 'log0015.log' - 10 files in directory
Write 'log0016.log' - 10 files in directory
10 files in 'logs' directory
'log0007.log' Size: 128, data: 0007: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua
'log0008.log' Size: 128, data: 0008: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua
'log0009.log' Size: 128, data: 0009: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua
'log0010.log' Size: 128, data: 0010: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua
'log0011.log' Size: 128, data: 0011: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua
'log0012.log' Size: 128, data: 0012: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua
'log0013.log' Size: 128, data: 0013: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua
'log0014.log' Size: 128, data: 0014: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua
'log0015.log' Size: 128, data: 0015: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua
'log0016.log' Size: 128, data: 0016: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua
10 files in 'logs' directory
Delete oldest file
9 files in 'logs' directory
Get oldest file in 'logs' directory
File: log0008.log
Done

```

### Curl memory tracing

The application can also track Curl memory allocations, to enable this simply uncomment `line 22` in CMakeLists.txt

```makefile
# add_compile_definitions(ENABLE_CURL_MEMORY_TRACE)
```

Enabling Curl memory tracing combined with the [Gallery Heap Tracker project](https://github.com/Azure/azure-sphere-gallery/tree/main/HeapTracker) enables you to analyze memory usage within your application, specifically when using Curl API calls.

With Curl memory tracking enabled you will see several memory tracking Log_Debug messages, this includes output from the following functions (malloc, free, calloc, strdup, realloc).

## Project expectations

* The code has been developed to show how to integrate a File System into an Azure Sphere project -  It is not official, maintained, or production-ready code.

### Expected support for the code

This code is not formally maintained, but we will make a best effort to respond to/address any issues you encounter.

### How to report an issue

If you run into an issue with this code, please open a GitHub issue against this repo.

## Contributing

This project welcomes contributions and suggestions. Most contributions require you to
agree to a Contributor License Agreement (CLA) declaring that you have the right to,
and actually do, grant us the rights to use your contribution. For details, visit
https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need
to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the
instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/)
or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## License

See [LICENSE.txt](./LICENCE.txt)

Littlefs is included via a submodule. The license for Littlefs can be [found here](https://github.com/littlefs-project/littlefs/blob/master/LICENSE.md).