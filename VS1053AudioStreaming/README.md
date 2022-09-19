# Audio Streaming using VS1053 Audio Codec

The goal of this project is to show how to interface with a VS1053 Audio Codec connected using SPI, and streaming NPR.

## Contents

| File/folder | Description |
|-------------|-------------|
| `src\`       | Azure Sphere Sample App source code |
| `src\HardwareDefinitions` | Hardware definition files for the Seeed RDB and Avnet Starter Kit |
| `src\VS1053`       | Source for VS1053 hardware |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://learn.microsoft.com/en-us/azure-sphere/install/overview) for more information).

Note that the Azure Sphere High Level application is configured for the 21.04 SDK release.

## How to use

The project supports the Seeed RDB with an [Adafruit VS1053](https://www.adafruit.com/product/1381) board.

Here's the pinout used in the project the Seeed RDB pinout diagram is [here](https://learn.microsoft.com/en-us/azure-sphere/hardware/mt3620-user-guide))

| Adafruit VS1053 Pin | Seeed MT3620 RDB Pin | Hardware Definition Mapping |
|-------------|-------------|-------------|
| VCC | Header 3, Pin 3 | NA |
| GND | Header 3, Pin 2 | NA |
| DREQ | Header 2, Pin 6 | VS1053_DREQ |
| MISO | Header 4, Pin 5 | VS1053_SPI ISU1 |
| MOSI | Header 4, Pin 11 | VS1053_SPI ISU1 |
| SCLK | Header 4, Pin 7 | VS1053_SPI ISU1 |
| RST | Header 2, Pin 14 | VS1053_RST |
| CS | Header 1, Pin 3 | VS1053_CS |
| xDCS | Header 2, Pin 12 | VS1053_DCS |
| AGND | Audio Jack GND | NA |
| ROUT | Audio Jack Right | NA |
| LOUT | Audio Jack Left | NA |

The VS1053 project code exposes four functions:

* **VS1053_Init** to initialize the hardware
* **VS1053_Cleanup** to cleanup SPI and GPIO resources
* **VS1053_SetVolume** to set the volume level (0 is off, 30 is max)
* **VS1053_PlayByte** to play audio data

The project is configured to play an embedded resource audio file, and also supports internet radio streaming. To enable the internet radio stream uncomment the **add_compile_definitions** line in the following block in the CMakeLists.txt file.

```cmake
# ENABLE NPR INTERNET RADIO STREAM ##########################################################################################
#
# add_compile_definitions(ENABLE_RADIO_STREAMING)
#
###################################################################################################################

```

## Project expectations

* The code is not official, maintained, or production-ready.

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
