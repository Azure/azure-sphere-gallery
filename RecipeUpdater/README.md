# IoT Hub Recipe Pusher Demonstration

This sample application demonstrates pushing a recipe file from Azure IoT Hub to a DHCP client connected to a Azure Sphere device. The recipe file is sent to the client using an HTTP POST request. This Azure Sphere application is accompanied by a web application in [web/](./web/). From the web application, a target file can be uploaded to Blob Storage and deployed out to Sphere devices.

This application follows two Azure Sphere samples: [Private Network Services](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/PrivateNetworkServices) and [Azure IoT](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/AzureIoT).

**IMPORTANT:** Before compiling and running this application, some configuration is required to ensure the device connects to the proper IoT hub and local endpoint.

## Contents


| File/folder | Description |
|-------------|-------------|
| `src`       | Source code of the Sphere. |
| `web`       | Source code of the web application. |
| `certs` | HTTPS certificates for cURL | 
| `HardwareDefinitions` | Subdirectories of the hardware definition files. |
| `script` | Build scripts for validation |
| `CMakeLists.txt` | The CMake build list for the Sphere application. |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Preresiquites

This application requires the following:

* Visual Studio Code with the Azure Sphere extension
* An Azure Sphere device with an ethernet port. In this application, an [Avnet Guardian 100](https://www.avnet.com/shop/us/products/avnet-engineering-services/aes-ms-mt3620-guard-100-3074457345641694219) is used.
* A client (oven, etc.) capable of receiving updates via a POST HTTP request.
* Azure Sphere SDK 21.07 or higher.
* An Azure subscription with:
   * Blob Storage
   * IoT Hub
   * Web Apps (see: web/ for details)

## Configuration
To configure the program, you'll have to make changes to app_manifest.json. 

1. From the Azure IoT Hub, find the hostname (`xxxxx.azure-devices.net`). This will be passed into the `CmdArg` as `--Hostname`. This domain must also be added in `AllowedConnections`.
2. From Azure Blob Storage, find the hostname (`xxxxx.blob.core.windows.net`). This must be added in the `AllowedConnections` as well.
3. Update the endpoint URL. The client (oven, etc.) will have an endpoint which will accept a POST request. This URL is set in the `--Endpoint` argument. Note that the IP must be set to 172.16.0.2.
4. If a certificate is used when getting the file from the cloud (see: certs/ for more information), the `--CertPath` argument must be set. If no certificate is used, this argument can be omitted.
5. The device tenant must be set in the `DeviceAuthentication` key. This can be retrieved from the CLI using `azsphere tenant show-selected`.

## Setup
1. Setup the Azure Sphere device according to their instructions, connect the device via USB, and ensure development is enabled with `azsphere device enable-development`.
2. Ensure the Azure Sphere is connected to a wireless network with internet.
3. Connect a client (oven, test device, etc) to the ethernet port on the Azure device.
   
   * This will eventually be assigned 172.16.0.2 by the DHCP server on the device.

## Running the code
Download Visual Studio Code with the Azure Sphere extension. Use `azsphere_connect.sh` to connect to a Sphere device, then press F5 (or Run > Start Debugging) to debug.

### Status LEDs

The LEDs on the Guardian module are on when:

1. DHCP server is running
2. Update is currently in progress
3. Update failed

## Key concepts
This application follows two Azure Sphere samples: [Private Network Services](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/PrivateNetworkServices) and [Azure IoT](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/AzureIoT).

The Sphere application is informed when the device twin is updated in IoT Hub. When the twin is updated, it uses a cURL multi handle to open a request to Blob Storage _and_ to the ethernet client endpoint. It simultaneously downloads the file and uploads it to the client. If this fails to connect to either cURL handle, it will print an error into the output and the device twin is updated with a `failed` attribute. It will continue to retry unless the 

<!-- Discuss the aspects of the project that might be particularly useful to someone looking at it:

- Significant design decisions
- Gotchas and tricky bits
- Opportunities for customization -->

## Project expectations

This application should not be used directly in a production environment. Please consider the following before using this code:

* The connection between the Sphere device and the client (oven) is not secure. HTTPS can be enabled by adding the SSL certificate into certs/ and modifying the cURL client to use that certificate, similar to how HTTPS is used between the Sphere device and Blob Storage.
* No file integrity (e.g. a checksum) is employed but should be used in a production environment.
* The download time window is unused, although it is passed in the update request.
* A watchdog timer should be enabled in a production environment.
* The code has not been run through a complete security analysis.

### Expected support for the code

There is no official support guarantee for this code, but we will make a best effort to respond to/address any issues you encounter.

### How to report an issue

If you run into an issue with this library, please open a GitHub issue within this repo.


## Contributing

<!--- Include the following text verbatim--->

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

<!---Make sure you've added the [MIT license](https://docs.opensource.microsoft.com/content/releasing/license.html) to the project folder.--->

For details on license, see LICENSE.txt in this directory.
