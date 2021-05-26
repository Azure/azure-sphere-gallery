# Grove 16x2 RGB LCD Display

The goal of this project is to show how to interface with a [Grove 16x2 RGB LCD display](https://wiki.seeedstudio.com/Grove-LCD_RGB_Backlight/) using I2C.

## Contents

| File/folder | Description |
|-------------|-------------|
| `src\HighLevelApp`       | Azure Sphere Sample App source code |
| `src\16x2_driver`       | Source for the Grove 16x2 RGB LCD display |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://docs.microsoft.com/en-us/azure-sphere/install/overview) for more information).

Note that the Azure Sphere High Level application is configured for the 21.04 SDK release.

## How to use

The 'display driver' for the Grove 16x2 RGB LCD display is based on the [Python Seeed LCD RGB Wiki sample code](https://wiki.seeedstudio.com/Grove-LCD_RGB_Backlight/).

The display driver exposes three functions, usage is demonstrated below:

```cpp
 RGBLCD_Init(MT3620_RDB_HEADER4_ISU2_I2C);    // returns false on failure
 RGBLCD_SetColor(0, 255, 0);  // Set background Color Green
 RGBLCD_SetText("Hello world\nSecond line!"); // two lines using '\n'
```
Note that the Grove 16x2 RGB LCD display is a 5V device, you will need to use a [Level Converter](https://learn.sparkfun.com/tutorials/bi-directional-logic-level-converter-hookup-guide/all) to drive the display at 5V rather than 3.3V.

## Project expectations

* The code has been developed to show how to integrate a Seeed I2C LCD RGB 16x2 display to an Azure Sphere board -  It is not official, maintained, or production-ready code.

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
