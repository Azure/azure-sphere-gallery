# WiFiConfigurationViaNFC project

The goal of this project is to show how to configure Wi-Fi settings for an Azure Sphere device using an NFC Tag written from a mobile phone (or other tag writer device).

## Contents

| File/folder | Description |
|-------------|-------------|
| `src`       | Azure Sphere High Level application source code |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://docs.microsoft.com/azure-sphere/install/overview) for more information).

## How to use
The project has been built for the [Avnet Azure Sphere Starter Kit](https://www.avnet.com/wps/portal/us/products/avnet-boards/avnet-board-families/ms-azure-sphere/) and [Mikroe Tag Click](https://www.mikroe.com/nfc-tag-click) board (based around the [ST M24SR NFC/RFID tag IC](https://www.st.com/en/nfc/m24sr64-y.html)) - the project can also work with other Azure Sphere boards, the Mikroe Click board will need to be wired up appropriately (refer to the [Mikro Tag Click User Manual](https://download.mikroe.com/documents/add-on-boards/click/nfc-tag/nfc-tag-click-manual-v100.pdf) for the wiring diagram).

The Azure Sphere application uses I2C/ISU2 to talk to the ST M24SR, and uses the click INT/GPIO2 as an interrupt for NFC tap events.

When an NDEF (NFC Data Exchange Format) Record is received by the application it's checked to ensure it's a WiFi NDEF record that contains SSID, Network Key, and authentication type - the SSID, Network Key and authentication type are passed to a callback function in main to add the device to the network.

You will need an application that is capable of writing NFC WiFi records - the Azure Sphere application has been tested using an Android phone and NFC Tools from the Android App Store.

The application waits for an NFC tap/write event by watching GPIO2/INT - once a tap event has occurred the application reads the NDEF record and determines whether this is a WiFi record - The app then extracts the SSID, Network Key, and authentication type - if all of these are present/valid the application then adds the WiFi configuration to the Azure Sphere device.

### Resources
* [Avnet Azure Sphere Starter Kit](https://www.avnet.com/wps/portal/us/products/avnet-boards/avnet-board-families/ms-azure-sphere/)
* [Mikroe NFC Tag click board](https://www.mikroe.com/nfc-tag-click)
* [ST M24SR Technical Specification](https://www.st.com/resource/en/datasheet/m24sr64-y.pdf) (I2C protocol definition)
* [ST M24SR Programmers Reference App Note](https://www.st.com/resource/en/application_note/dm00102751-m24sr-programmers-application-note-based-on-m24srdiscovery-firmware-stmicroelectronics.pdf)
* [WiFi Alliance](https://www.wi-fi.org/) - the **Wi-Fi Simple Configuration Technical Specification** contains the WiFi NDEF Record definition.

## Project expectations

* This code is not official, maintained, or production-ready.

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
