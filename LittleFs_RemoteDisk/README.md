# Littlefs Remote Disk project

The goal of this project is to show how to add file system support to an Azure Sphere project, in this case using the Littlefs file system and 'remote storage', the sample project can be extended to talk to physical hardware.

Here's a link to the [Littlefs project on Github](https://github.com/littlefs-project/littlefs).

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

Supporting Littlefs requires that you setup the storage layout (page, sector, block, and total size), and then support four functions, these are **read, write (called program in the Littlefs implementation), erase, and sync**

**Python Remote Storage app** 
The project contains a Python Flask application (PyDiskHost.py) that supports 4MB storage (matching the defined storage layout of the high-level Azure Sphere application) - The Python application supports HTTP Get (read), and HTTP Post (Write) functions - the 4MB storage is supported by an in-memory bytearray (but could be easily modified to use a file on disk). The Python app is configured to use port 5000.

Start the Python application before running the Azure Sphere application.

**Azure Sphere High Level Application** 

The high level application sets up the storage size (lines 24-27)

```cpp
// 4MB Storage.
#define PAGE_SIZE     (256)
#define SECTOR_SIZE   (16 * PAGE_SIZE)
#define BLOCK_SIZE    (16 * SECTOR_SIZE)
#define TOTAL_SIZE    (64 * BLOCK_SIZE)
```

The storage size information and pointers to read, program, erase, and sync are used to initialize Littlefs (lines 40-53) in main.c.

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

The application will initialize and format the storage, create a file, write some test data, rewind the file pointer, read the data and print to the debug console, and then close the file.

When the application runs you should see the following output in the Visual Studio/Code debug window:

```makefile
Format and Mount
Open File 'test.txt'
Write to the file
Seek to the start of the file
Read from the file
Read content = Test
Close the file
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

See [LICENSE.txt](./LICENSE.txt)

Littlefs is included via a submodule. The license for Littlefs can be [found here](https://github.com/littlefs-project/littlefs/blob/master/LICENSE.md).