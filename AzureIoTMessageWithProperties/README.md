# AzureIoTMessageWithProperties project

The goal of this project is to show how to add custom properties to a telemetry message being sent to Azure IoT Hub.

## Contents

| File/folder | Description |
|-------------|-------------|
| `src\HighLevelApp`       | Azure Sphere Sample App source files (based on the Azure IoT Github sample) |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://docs.microsoft.com/azure-sphere/install/overview) for more information).


## How to use

You will need the following:
* Azure Sphere device claimed into a tenant
* A copy of the [Azure Sphere Github samples repository](https://github.com/Azure/azure-sphere-samples) (specifically the [Azure IoT sample](https://github.com/Azure/azure-sphere-samples/tree/main/Samples/AzureIoT))

Follow the Azure IoT sample instructions, you can choose the [IoT Hub](https://github.com/Azure/azure-sphere-samples/blob/main/Samples/AzureIoT/READMEStartWithIoTHub.md) or [IoT Hub with DPS](https://github.com/Azure/azure-sphere-samples/blob/main/Samples/AzureIoT/READMEAddDPS.md) instructions. 

### Using the Azure IoT sample

You will need to modify the Azure IoT sample from the Azure Sphere samples Github repository.

Copy the files from the [HighLevelApp/common](./HighLevelApp/common) directory to your copy of the Azure IoT sample (into the common folder, the files will overwrite existing files in that folder) - these modified files add a new API in `azure_iot.h/.c` which is called `AzureIoT_SendTelemetryWithProperties` which is based on the `AzureIoT_SendTelemetry` function, the new function is called from `cloud.c` which sends two types of telemetry message:

- `Cloud_SendTelemetry` sends temperature data - this function now adds custom message properties (more on this below)
- `Cloud_SendThermometerMovedEvent` sends thermometerMoved telemetry - this function does not add custom properties to the message

Properties added to a message are a collection of Key/Value Pairs, here's an example of declaring telemetry properties (the structure MESSAGE_PROPERTY is defined in azure_iot.h, and used ind cloud.c):

```C
static MESSAGE_PROPERTY* telemetryMessageProperties[] = {
    &(MESSAGE_PROPERTY) { .key = "appid", .value = "hvac" },
    &(MESSAGE_PROPERTY) {.key = "format", .value = "json" },
    &(MESSAGE_PROPERTY) {.key = "type", .value = "telemetry" },
    &(MESSAGE_PROPERTY) {.key = "version", .value = "1" }
};
```

Here's what the call to send temperature telemetry including the custom properties looks like:

```C
    AzureIoT_Result aziotResult = AzureIoT_SendTelemetryWithProperties(serializedTelemetry, NULL, telemetryMessageProperties, NELEMS(telemetryMessageProperties));
```

### Viewing the Azure IoT Hub telemetry

There are two ways you can view the telemetry your device is sending to Azure IoT Hub.

1. Use the [Azure IoT Hub Explorer](https://github.com/Azure/azure-iot-explorer) - this is a GitHub project (currently in preview)
2. [Use the Azure CLI](https://docs.microsoft.com/en-us/cli/azure/iot/hub?view=azure-cli-latest#az_iot_hub_monitor_events)

With the updated Azure IoT sample running you should see telemetry events for Temperature and Thermometer Moved

Here's the updated temperature telemetry which now includes the properties for appid, format, type, and version
```JSON
{
  "body": {
    "temperature": 58.950008392333984
  },
  "enqueuedTime": "Tue Aug 03 2021 13:57:57 GMT+0100 (British Summer Time)",
  "properties": {
    "appid": "hvac",
    "format": "json",
    "type": "telemetry",
    "version": "1"
  }
}
```

And here's the thermometerMoved telemetry, note that this does not contain custom properties.
```JSON
{
  "body": {
    "thermometerMoved": true
  },
  "enqueuedTime": "Tue Aug 03 2021 13:57:48 GMT+0100 (British Summer Time)"
}

```

## Routing IoT Hub Messages based on telemetry content

Azure IoT Hub provides the ability to route messages based on the message contents - you might have several classes of device sending telemetry to the same Azure IoT Hub - each device class could have their telemetry send to different long-term storage, or routed to a device specific Event Hub.

More information on routing of Azure IoT Hub messages can be found in in the [IoT Hub message routing query syntax documentation](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-routing-query-syntax)

There's also a Microsoft Build 2021 session [Build Secured IoT Solutions for Azure Sphere with IoT Hub](https://www.youtube.com/watch?v=UTVPjZGZblo) that demonstrates message routing through IoT Hub.

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

See [LICENSE.txt](./LICENCE.txt)