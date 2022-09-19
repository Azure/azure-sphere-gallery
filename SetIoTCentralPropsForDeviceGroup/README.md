# SetIoTCentralPropsForDeviceGroup

SetIoTCentralPropsForDeviceGroup is a utility that makes it easy to set an Azure IoT Central Device Twin property for all devices in an Azure Sphere Device Group.

## Contents

| File/folder | Description |
|-------------|-------------|
| `src\SetIoTCentralPropsForDeviceGroup`       | SetIoTCentralPropsForDeviceGroup utility |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

You should have at least one [Azure Sphere tenant](https://learn.microsoft.com/en-us/azure-sphere/deployment/create-tenant), and one or more devices claimed into your tenant.

You should clone the Azure IoT sample from the [Azure Sphere Samples](https://github.com/Azure/azure-sphere-samples) Github Repository.

Follow the instructions to [setup an Azure IoT Central application](https://github.com/Azure/azure-sphere-samples/blob/main/Samples/AzureIoT/IoTCentral.md)

## How to use

SetIoTCentralPropsForDeviceGroup is a console application, the application takes four command line arguments:

| Command | Description |
|-------------|-------------|
| `Azure IoT Central App URL`       | example: https://myapp.azureiotcentral.com |
| `Azure IoT Central API Token` | [Create an API token in the IoT Central App](https://learn.microsoft.com/en-us/azure/iot-central/core/overview-iot-central-tour#administration) |
| `Azure Sphere Device group guid`   | The guid of the Azure Sphere Device Group to update |
| `JSON File containing settings to apply`       | example: `{"thermometerTelemetryUploadEnable": true}` |

**Note that the JSON property needs to be read/write in the Azure IoT Central Application.**

To obtain the Azure Sphere Device Group ID you can:
* Determine the list of Azure Sphere tenants you have access to `azsphere tenant list`
* Select the tenant `azsphere tenant select -i {guid of tenant}`
* Determine the list of products within the tenant `azsphere product list`
* Determine the list of device groups within a product `azsphere product device-group list --productname <product name>`
* You can move a device into a device group using this command `azsphere device update --devicegroupid <guid> -i <device id>`

### Checklist - Make sure you have:
1. A device that's claimed into your Azure Sphere tenant
2. You have created a Product, and Device Groups (default device groups are ok)
3. Your device has been selected into one of the device groups
4. You have cloned the Azure IoT Sample from Github
5. You have created the Azure IoT Central application
6. You have run ShowIoTCentralConfig to obtain the IoT Central Endpoint addresses, and have updated the Azure IoT App_Manifset.json

Make sure you have the four required command line arguments:
* Azure IoT Central app URL
* Azure IoT Central API Token
* Azure Sphere Device Group Guid
* JSON Setting to apply

You can now run the SetIoTCentralPropsForDeviceGroup utility:

A sample command line could look like this:

`SetIoTCentralPropsForDeviceGroup https://myapp.azureiotcentral.com "IoT Central Access Token" 168A1115-568D-4717-A445-CFC4BB1BB8C7 {\"StatusLED\":true}`

## Potential issues

While working with the community we've documented a couple of potential issues that users may encounter using this sample

### Issue encountered: Application incorrectly parses JSON argument

You may see this issue if you use the Windows PowerShell application to run the sample.  Try running the sample using the Windows Command Prompt "cmd" application.

### Issue encountered: The sample runs without errors, however the twin property is not updated in the IoTCentral application

Make sure that your device twin properties are defined as an "Interface" in your IoTCentral application.  You can add the standard "Azure Sphere Device Template" to see a working example.

## Project expectations

* This is a utility for developers; it is not official, maintained, or production-ready code.

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
