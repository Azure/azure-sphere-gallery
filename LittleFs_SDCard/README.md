# Littlefs SD Card project

The goal of this project is to show how to add file system and SD Card support to an Azure Sphere project.

Here's a link to the [Littlefs project on Github](https://github.com/littlefs-project/littlefs).

## Contents

| File/folder | Description |
|-------------|-------------|
| `src\AzureSphere`       | Azure Sphere A7 and M4 source code |
| `src\Desktop`       | Windows desktop projects to read raw SD Cards and extract files/folders |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://docs.microsoft.com/en-us/azure-sphere/install/overview) for more information).

## How to use

### Azure Sphere applications 

The LittleFs/SD Card implementation uses two applications:
**High Level Application** (running on the A7 Core) which implements LittleFs (read, write, erase, and sync functions) and an inter-core messaging system used to talk to one of the Real-Time capable (M4) cores.
**Real-Time Capable Application** (running on one of the M4 Cores) that implements the inter-core messaging system and the SPI/SD Card interface for reading/writing blocks to a physical SD card (the real-time capable application doesn't have any knowledge of the file system or data being read/written).

The SPI driver and SD Card implementation is based on the [Codethink Labs SPI_SDCard_RTApp_MT3620_BareMetal project](https://github.com/CodethinkLabs/mt3620-m4-samples/tree/master/SPI_SDCard_RTApp_MT3620_BareMetal)

Supporting Littlefs requires that you setup the storage layout for the SD Card you are using, this is configured in the High Level application. 

Modify the TOTAL_BLOCKS definition to match the size of your SD Card - the default configuration is setup to use 4MB (8192 blocks), this is useful to demonstrate that LittleFs/SD Card is working correctly (reading 4MB SD Card to the desktop, and extracting files from the image will be much faster than a 16GB card!).

```C
// SD Card uses 512 Byte blocks
// 4MB Card size = 4,194,304 bytes - 8192 blocks
// 2GB Card size = 2,147,483,648 bytes = 4194304 total blocks
// 
// The Project is configured for 4MB Storage (8192 blocks)
#define BLOCK_SIZE     512
#define TOTAL_BLOCKS   8192     // TODO: Modify TOTAL_BLOCKS to match your SD Card configuration (total bytes/512)
```

**Note:** The project is configured to use the Avnet MT3620 Starter Kit Version 2 which uses ISU0 on Click Socket 1 and a [Mikroe SD Click board](https://www.mikroe.com/microsd-click). If you want to use the project with the Avnet MT3620 Starter Kit Version 1 you will need to modify the Real-Time applications app_manifest.json, and main.c to use ISU0. If you are using a stand-alone SD Card board such as the [AdaFruit SD Card breakout board](https://www.adafruit.com/product/254) and the Seeed RDB then you can choose an appropriate ISU.

Note that you can increase the amount of debug information displayed from the High-Level and Real-Time Capable applications by uncommenting this line in CMakeLists.txt

```python
# add_compile_definitions(SHOW_DEBUG_INFO)
```

The High-Level application on startup defaults to writing zeros to SD Card blocks 0 and 1, this wipes the LittleFs initialization blocks from the SD Card, you can comment out the call to `FormatCard` in main to leave the card intact (formatting the card will force the initialization of LittleFs on an SD card).

Pressing 'Button A' on the Azure Sphere development board will trigger a call to `DoLittleFswork` - this function will initialize LittleFs, create a directory, create a file, write to the file, rewind the file pointer, read from the file, and display the contents of the file - the code also cleans up by deleting the file and the directory, comment out the clean up code to leave the SD Card contents intact.

### Desktop tools

There are two desktop tools:
* ReadRawSDCard is a utility to read a number of USB/SD Card blocks to a file on your PC
* LittleFsDesktop is a utility that mounts LittleFs from a desktop binary file, and extracts the contents to folders/files on the PC

**ReadRawSDCard**

The ReadRawSDCard utility requires two command line parameters:
1. Windows physical drive name (example: `\\.\PHYSICALDRIVE3`)
2. Number of 512 byte blocks to read

The utility creates an output file called `diskImage.bin`.

To obtain the list of physical drives on your PC open PowerShell and type the command `wmic diskdrive list brief`, this will produce output similar to the list below:

```dos
Caption                               DeviceID            Model                                 Partitions  Size
USB Reader USB Device                 \\.\PHYSICALDRIVE3  USB Reader USB Device                 1           16014620160
```

The Physical Drive name is the `DeviceID`, which in this case is `\\.\PHYSICALDRIVE3`

The number of blocks you need to read is based on the LittleFs drive layout you have defined in the Azure Sphere application, let's assume you have defined 4MB for LittleFs on an SD Card, this would require 8192 blocks (512 bytes per block). 

The command line for ReadRawSDCard would be:
```dos
ReadRawSDCard \\.\PHYSICALDRIVE3 8192
```

Note that this utility will prompt for Administrator rights.

**LittleFsDesktop**
The LittleFsDesktop utility is a C/Win32 program that uses LittleFs to mount a binary file (created from ReadRawSDCard or other utility that can read raw blocks from a USB/SD Card device) and extract the folders/files to the desktop. This can be useful for recovering telemetry/runtime data from a device.

The utility takes one command line parameter, this is the name of the LittleFs binary file to mount - the utility will attempt to mount LittleFs, and if successful will walk the LittleFs file system and extract any folders/files - the utility creates an `output` folder in the working directory of the utility, all folders/files extracted from the binary file will be written to the output folder, folder structure from LittleFs will be preserved.

If you want to see additional debug information while the LittleFsDesktop tool is running then you can uncomment the following line in `Source.c`.

```C
// #define SHOW_DEBUG_INFO
```

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

Codethink Labs M4 drivers are included via a submodule. The license for the drivers can be [found here](https://github.com/CodethinkLabs/mt3620-m4-drivers/blob/master/LICENSE.txt)