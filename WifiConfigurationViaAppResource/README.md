# Wifi configuration via application resource project

The goal of this project is to show how to configure Wi-Fi settings for a device using an embedded application resource file (JSON).

## Contents

| File/folder | Description |
|-------------|-------------|
| `src`       | Azure Sphere High Level application source code |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |
| `ThirdPartyNotices.txt`   | The license for parson (JSON parser). |

## Prerequisites & Setup

- An Azure Sphere-based device with development features enabled (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://learn.microsoft.com/en-us/azure-sphere/install/overview) for more information).

Note that the Azure Sphere High Level application is configured for the 21.01 SDK release.

## How to use
The application uses an embedded application resource file (WiFiConfig.json), the file contains the SSID and network key used to configure the device.

```json
{
    "ssid": "mySSID",
    "networkKey": "myNetworkKey"
}
```

To read the JSON the application uses the [Storage_OpenFileInImagePackage](https://learn.microsoft.com/en-us/azure-sphere/reference/applibs-reference/applibs-storage/function-storage-openfileinimagepackage) API, and then  [lseek](https://man7.org/linux/man-pages/man2/lseek.2.html), and [read](https://linux.die.net/man/2/read) APIs to read the JSON file - [parson](https://github.com/kgabis/parson) is used as a JSON parser to read the SSID and Network Key.

The sample JSON contains 'mySSID' and 'myNetworkKey', replace these with the SSID and network key that you want to provision on your device.

The application enables the WiFiConfig capability in app_manifest.json.

```json
"WifiConfig": true
```

Note that the project uses `WifiConfig_SetSecurityType` to set the security type to `WifiConfig_Security_Wpa2_Psk` - the API can also take `WifiConfig_Security_Open` and `WifiConfig_Security_Wpa2_EAP_TLS`. The sample will need to be modified to support these other security types.

## Project expectations

* The code has been developed to show how to configure Wi-Fi settings using an embedded JSON application resource - It is not official, maintained, or production-ready code.

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

This project builds on open source components, these are listed in [ThirdPartyNotices.txt](./ThirdPartyNotices.txt)