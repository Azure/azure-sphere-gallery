# Wifi configuration via Uart project

The goal of this project is to show how to configure Wi-Fi settings for a device using one of the Azure Sphere UARTs and a desktop Terminal application.

## Contents

| File/folder | Description |
|-------------|-------------|
| `src`       | Azure Sphere High Level application source code |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features enabled (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://docs.microsoft.com/en-us/azure-sphere/install/overview) for more information).
- A USB/Serial adapter to connect the Azure Sphere board to your development PC ([here's an example device](https://www.amazon.com/gp/product/B07BBPX8B8/ref=ppx_yo_dt_b_asin_title_o00_s00?ie=UTF8&psc=1))
- A terminal application that enables Serial connections (PuTTY or similar).

Note that the Azure Sphere High Level application is configured for the 21.01 SDK release.

## How to use
The application is configured to use ISU0, UART Tx, and Rx pins - the MT3620 pinout diagram can be found [here](https://docs.microsoft.com/en-us/azure-sphere/hardware/mt3620-user-guide). The UART is configured for 115200 baud.

The high level application will display a menu in your development PC terminal application - the menu contains: device reboot, listing of the existing network configuration, and the ability to add a new network configuration (see below).

```plaintext
Azure Sphere UART Wi-Fi Configuration application starting...

1. Reboot
2. Get stored Wi-Fi networks
3. Add Wi-Fi

Option>
```

If you connect the PC serial terminal after the Azure Sphere device has booted then simply press 'Enter' to re-display the menu.

The application enables two capabilities in app_manifest.json - these enable the Wi-Fi APIs and the ability to reboot the device.

```json
"WifiConfig": true,
"PowerControls": [ "ForceReboot" ]
```

Note that the project uses `WifiConfig_SetSecurityType` to set the security type to `WifiConfig_Security_Wpa2_Psk` - the API can also take `WifiConfig_Security_Open` and `WifiConfig_Security_Wpa2_EAP_TLS`. The sample will need to be modified to support these other security types.

## Project expectations

* The code has been developed to show how to configure Wi-Fi setting using a UART/Menu - It is not official, maintained, or production-ready code.

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