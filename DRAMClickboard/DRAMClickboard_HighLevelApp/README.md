# DRAM Click board high-level app

This sample demonstrates how to use the DRAM Click board with [Azure Sphere SPI interface](https://learn.microsoft.com/azure-sphere/app-development/spi) in a high-level application.

The sample goes through a sequence of write adn read commands to each address on the DRAM Click board connected to an MT3620 development board through the SPI ISU on one of the MikroBUS socekts. The data is written and read from the chip with no delays in between calls. To see a slower output, there are delay statements in the code that can be uncommented.

## Contents

| File/folder           | Description |
|-----------------------|-------------|
| `app_manifest.json`   | Application manifest file, which describes the resources. |
| `CMakeLists.txt`      | CMake configuration file, which Contains the project information and is required for all builds. |
| `CMakePresets.json`   | CMake presets file, which contains the information to configure the CMake project. |
| `launch.vs.json`      | JSON file that tells Visual Studio how to deploy and debug the application. |
| `LICENSE.txt`         | The license for this sample application. |
| `dram.c`             | Source code for DRAM Click board library |
| `dram.h` | Header file for DRAM Click board library |
| `main.c`              | Main C source code file. |
| `README.md`           | This README file. |
| `.vscode`             | Folder containing the JSON files that configure Visual Studio Code for deploying and debugging the application. |
| `HardwareDefinitions` | Folder containing the hardware definition files for various Azure Sphere boards. |

The sample uses the following Azure Sphere libraries.

| Library   | Purpose |
|-----------|---------|
| [gpio](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Accesses button A and LED 1 on the device. |
| [log](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) |  Displays messages in the **Device Output** window during debugging. |
| [spi](https://learn.microsoft.com/azure-sphere/reference/applibs-reference/applibs-spi/spi-overview) | Manages the Serial Peripheral Interfaces (SPIs). |



## Prerequisites

This sample requires the following hardware:

- An [Azure Sphere development board](https://aka.ms/azurespheredevkits) that supports the [Sample Appliance](../../../HardwareDefinitions) hardware requirements.

   **Note:** By default, the sample targets the [Reference Development Board](https://learn.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design) design, which is implemented by the Seeed Studios MT3620 Development Board. To build the sample for different Azure Sphere hardware, change the value of the TARGET_HARDWARE variable in the `CMakeLists.txt` file. For detailed instructions, see the [Hardware Definitions README](../../../HardwareDefinitions/README.md) file.

- [Mikro DRAM Click board](https://www.mikroe.com/dram-click)

## Setup

1. Set up your Azure Sphere device and development environment as described in the [Azure Sphere documentation](https://learn.microsoft.com/azure-sphere/install/overview).
1. Even if you've performed this set up previously, ensure you have Azure Sphere SDK version 23.05 or above. At the command prompt, run **azsphere show-version** to check. Upgrade the Azure Sphere SDK for [Windows](https://learn.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://learn.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Enable application development, if you have not already done so, by entering the **azsphere device enable-development** command at the command prompt.
1. Clone the [Azure Sphere samples](https://github.com/Azure/azure-sphere-samples) repository and find the *DRAMClickboard_HighLevelApp* sample in the *DRAMClickboard* folder.

### Hardware Defintions Setup
1. Change the CMakeLists file to reflect the hardware that you will be using with the app. Currently, only the avnet_mt3620_sk and avnet_mt3620_sk_rev2 have hardware definition files for this sample application
1. Add the set of SPI ISU and GPIO pins that correspond with the chosen MikroBUS socket to the app manifest document to enable the peripherals to be used in the High-Level app.

### Set up DRAM Click board connections

Plug the click board into one of the 2 MikroBUS sockets on the MT3620.

The MT3620 Rev 1 board supports both socket 1 and socket 2 for the DRAM Click board. Be sure to change the app manifest and DRAM initalization function call to match the socket chosen to plug the DRAM chip into.

The MT3620 Rev 2 board only supports socket 1. Be sure to add the corresponding peripherals to the app manifest and pass them into the DRAM initialization function.

Also, for the Rev 1 board, you must set the SPI Mode to SPI mode 2. For the Rev 2 board, the SPI Mode must be set to SPI Mode 0. You can change this setting in the dram.c file in the dram init function.

## Project Expectations

When you run the application, it first initializes the SPI interface, does a software reset for the DRAM chip and does a communication check. All these operations are launched by the DRAM initialization function. 
After it is confirmed that the MT3620 can successfully communicate with the DRAM chip, a series of writes and reads will occur. The first write/read pairing is the message "MikroE". The second pairing is a write and a fast read of the message is "DRAM Click board". Uncommenting the debug statements will slow down the output and allow you to view what is written and read from the chip.
If the initialization fails, verify that the device is in the correct socket and that the SPI Mode for the revision model has been changed accordingly.

### Toggle Wrap Boundary

There is function that toggles the wrap boundary of data transfers between Linear Burst mode and Wrap32. According to the chip's documentation, the DRAM Click board starts off in Linear Burst mode, which allows for an unlimited number of data bytes to be communicated between the chips. Wrap32 on the other hand limits the transaction size to 1KB.
**There is no practical difference between these modes** for the MT3620 Rev 1 and Rev 2 boards for several reasons. 1) I have coded the write and read functions to partition the data into valid sized chunks, so there can be up to 8MB transmitted in a single communication command from the user standpoint. 2) The speed of both modes are bottlenecked by the MT3620 SPI bus speed limit which is 40MHz. Almost all operations occur at this speed no matter the wrap boundary mode.

### Quad SPI Mode

Currently, there is no support for Quad SPI mode

## Next steps

- For an overview of Azure Sphere, see [What is Azure Sphere](https://learn.microsoft.com/azure-sphere/product-overview/what-is-azure-sphere).
- To learn more about Azure Sphere application development, see [Overview of Azure Sphere applications](https://learn.microsoft.com/azure-sphere/app-development/applications-overview).

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

For information about the licenses that apply to this library, see [LICENSE.txt](./LICENSE.txt)