# Azure IoT sample with Store and Forward

The goal of this project is to show how to add store and forward capability to the [Azure Sphere AzureIoT sample](https://github.com/Azure/azure-sphere-samples/tree/main/Samples/AzureIoT). The storage implementation is based on the [simple file system gallery project and python disk host](https://github.com/Azure/azure-sphere-gallery/tree/main/SimpleFileSystem_RemoteDisk).

## Contents

| File/folder | Description |
|-------------|-------------|
| `src`       | modified/new source files to add to the AzureIoT sample project |
| `copy_required_files.cmd` | Windows script to copy files from the SimpleFileSystem_RemoteDisk project |
| `copy_required_files.sh` | Linux script to copy files from the SimpleFileSystem_RemoteDisk project |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://learn.microsoft.com/en-us/azure-sphere/install/overview) for more information).

Note that the Azure Sphere High Level application is configured for the 21.07 SDK release.

## How to use

You will need the following:
* Azure Sphere device claimed into a tenant
* A copy of the [Azure Sphere Github samples repository](https://github.com/Azure/azure-sphere-samples) (specifically the [Azure IoT sample](https://github.com/Azure/azure-sphere-samples/tree/main/Samples/AzureIoT))
* A copy of the [Azure Sphere Github Gallery repository](https://github.com/Azure/azure-sphere-gallery)

Copy the files from this project `src` to your copy of the root of the Azure Sphere `AzureIoT` sample project.

You will need to copy files from the Azure Sphere Gallery `SimpleFileSystem_RemoteDisk` project:
* On **Windows**, open a command prompt to this project folder and run `copy_required_files.cmd`
* On **Linux**, open a terminal and run `copy_required_files.sh`

The store and forward gallery project modifies several files:

`AzureIoT/CMakeLists.txt` - You will need modify the PC_HOST_IP definition within CMakeLists.txt to include the IP address of your PC (the PC there the Python remote disk host will run).

```cmd
add_compile_definitions(PC_HOST_IP="YOUR_PC_IP_ADDRESS")
```

`AzureIoT/app_manifest.json` - add the IP address of your PC to the `AllowedConnections` list in `app_manifest.json`

`AzureIoT/common/exitcodes.h` - a new exit code `Exit_Code_Init_FileSystem` has been added.

`AzureIoT/common/main.c` - The AzureIoT sample uses the `telemetryUploadEnabled` boolean to define whether telemetry will be uploaded or not, the sample has been extended to store telemetry in the simple file system when telemetryUploadEnabled == false.

The original AzureIoT sample uses a 5 second tick to generate telemetry, the tick has been modified to be 1 second. Every five seconds new telemetry will be generated - if there are no stored telemetry items and telemetryUploadEnabled is true the new telemetry will be uploaded, if there are existing stored telemetry items the new telemetry will be written to the simple file system (this preserves date/time order of uploaded telemetry). For the other four seconds stored telemetry will be uploaded if telemetryUploadEnabled is true.

The project uploads stored telemetry over time rather than uploading all stored data when telemetryUploadEnabled is true. This approach may not be suitable for your specific application, you should adapt the upload model to suit your needs.

Follow the Azure IoT sample instructions, you can choose the [IoT Hub](https://github.com/Azure/azure-sphere-samples/blob/main/Samples/AzureIoT/READMEStartWithIoTHub.md) or [IoT Hub with DPS](https://github.com/Azure/azure-sphere-samples/blob/main/Samples/AzureIoT/READMEAddDPS.md) instructions.

To enable remote storage you will need to run the [Python remote disk host application](../SimpleFileSystem_RemoteDisk/src/PyDiskHost/PyDiskHost.py).

You can toggle whether data is stored or uploaded by pressing Button 'A' on your Azure Sphere development board.

When the application starts telemetry upload is disabled, telemetry will be stored in the simple file system, you should see debug output similar to the following:

```cmd
INFO: Azure IoT Hub connection started.
INFO: DPS device registration successful
INFO: Azure IoT Hub connection complete.
Azure IoT connection status: IOTHUB_CLIENT_CONNECTION_OK
INFO: Azure IoT Hub client accepted request to report state '{"serialNumber":"TEMPMON-01234"}'.
INFO: Azure IoT Hub Device Twin reported state callback: status code 204.
1 telemetry item stored
2 telemetry items stored
3 telemetry items stored
4 telemetry items stored
5 telemetry items stored
```

When telemetry upload is enabled you should see debug output similar to the following:

```cmd
45 telemetry items stored
(45 telemetry items in storage) upload: 2021/09/27 - 13:22:10 - 50.40
Sending Azure IoT Hub telemetry: {"temperature":50.400001525878906}.
INFO: IoTHubClient accepted the telemetry event for delivery.
```

The date/time displayed in the Log_Debug message is the time that the telemetry item was stored (not the time the telemetry message was uploaded), this date/time will become the telemetry creation time in the IoT Hub message property, see [iothub-creation-time-utc](https://learn.microsoft.com/azure/iot-hub/iot-hub-devguide-messages-construct#application-properties-of-d2c-iot-hub-messages), this ensures that the timeline for uploading stored data is maintained.

The project is configured to store a maximum of 4,000 telemetry items before old data is overwritten, look at the  `InitializeFileSystem` function in `main.c` (the AzureIoT sample creates a new telemetry item every 5 seconds - with this configuration the app would store about 5 hours of data) - The Python disk host is configured to store 4MB of data.

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

