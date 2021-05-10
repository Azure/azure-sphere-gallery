# DevicePropertiesWebHook project

The goal of this project is to show how to update device twin properties for a device connected to Azure IoT Hub to reflect Azure Sphere Security Service (AS3) data about that device, such as its Product, Device Group, and OS Feed type.

The project also shows how to transmit device lifecycle events from the Azure Sphere device to Azure IoT, such as when the app is restarted or when the app receives notification that the device software is up-to-date.

## Contents

| File/folder | Description |
|-------------|-------------|
| `src\HighLevelApp`       | Azure Sphere Sample App source files (based on the Azure IoT Github sample) |
| `src\REST_API`       | ASP.NET Core REST API project |
| `src\DesktopSimulator`       | C# .NET Core desktop application for simulating device telemetry |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://docs.microsoft.com/en-us/azure-sphere/install/overview) for more information).


## How to use

You will need the following:
* Azure Sphere device claimed into a tenant
* A copy of the [Azure Sphere Github samples repository](https://github.com/Azure/azure-sphere-samples) (specifically the [Azure IoT sample](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/AzureIoT))

Follow the Azure IoT sample instructions, you can choose the [IoT Hub](https://github.com/Azure/azure-sphere-samples/blob/master/Samples/AzureIoT/READMEStartWithIoTHub.md) or [IoT Hub with DPS](https://github.com/Azure/azure-sphere-samples/blob/master/Samples/AzureIoT/READMEAddDPS.md) instructions. 

* An [Azure Key Vault](https://azure.microsoft.com/en-gb/services/key-vault/) - this will hold three secrets (Azure IoT Hub connection string, your Azure Sphere tenant Id [guid], and an APIKey used by the ASP.NET Core app service to validate a web session for initializing Active Directory Authentication)

Here's how to obtain the three secrets you will need for Key Vault:

**Azure IoT Hub Connection String**: Open your Iot Hub in the Azure Portal, navigate to **Shared Access Policies** | **iothubowner** - copy the **Primary connection string**

**API Key**: In Visual Studio 2019 use the **Tools** | **Create Guid** option, and specifically the Registry Format, this will create a GUID in this format: {00000000-0000-0000-0000-000000000000} - use a text editor to remove the Braces/Brackets/Mustaches, so you end up with just the text and dashes. Note: You can also use Visual Studio Code with the **Insert Guid** extension to generate the guid, or [an online guid generator](http://guid.one/).

**Azure Sphere Tenant Id** Use the Azure Sphere command line interface (CLI) and run `azsphere tenant show-selected` (you can also use the `azsphere tenant list` and `azsphere tenant select` commands to select a different tenant).

Navigate to your Key Vault in the Azure Portal, you will need to create three secrets, named as follows:

| Key Vault Secret Name | Value |
|----------|----------|
| APIKey | The GUID you generated in Visual Studio/Code (sans mustaches) |
| IoTHubConnectionString | The iothubowner primary connection string |
| tenantId | Your Azure Sphere tenant Id (guid) |

Add the three secrets to Key Vault.

**Get the Azure Key Vault URI** - While you are in the Key Vault view in the Azure Portal click on **Overview**, top right of the overview page contains the Key Vault URI - **Copy the Key Vault URI** you will need this for the ASP.NET Core REST API project.

### Build and Deploy the ASP.NET Core REST API project.

Open the [REST_API project](./REST_API/DevicePropertiesWebHook.sln), open **Utils.cs** and paste your Key Vault URI into the empty string called **KeyVaultUrl**.

This is the only change you need to make to the REST_API project.

**Build and Deploy the REST_API project to your Azure Resource Group**

Once the REST_API project is deployed to Azure copy the project endpoint URL to the clipboard (this will be displayed in the build output window, and also in your browser once the project is deployed).

Note that once the project is deployed you will see a web page open that states that the 'page cannot be found' - this is expected, there isn't a defined REST API on the base URL for the project.

You can test that the REST_API project is running by opening your web browser and navigating to the project URL/root - this should display a message showing that the **API Service is running**.

The API Service exposes two REST APIs, these are:

**root** if you use your browser to navigate to the REST API project url/root this will display a message showing that the API service is running, and the authentication state of the service.

**webhook** the /webhook endpoint supports GET and POST - GET is used to initialize the authentication process, and POST is the webhook endpoint that IoT Hub/Event Grid will post data.

Note that you need to authorize the REST API project to have access to your key vault before the service will work (the service reads the key vault secrets when the /webhook endpoint is accessed).

**Authorize your REST API service with Key Vault**
When your REST API service is built and deployed to your Azure Portal Resource Group a 'Service Principal' is created, this can be used to authorize access to your key vault. 

Follow [this guide](https://docs.microsoft.com/en-us/azure/key-vault/general/assign-access-policy-portal) for granting permissions to your secrets, note that the REST API service should be granted list and read permissions. The name of your Principal is going to be the name of your REST API service in the Azure Portal.

**Ensure that the REST API Service is configured for **Always on** (a restart will lose the authentication data)

In the Azure Portal go to your REST API service resource, select **Configuration** | **General Settings** - then locate **Always On**, enable this (if not set), and save the changes (this will restart the service).

**Authenticating against the REST API service**
The REST API service uses the [Device Code Flow](https://github.com/AzureAD/microsoft-authentication-library-for-dotnet/wiki/Device-Code-Flow) authentication model - you initiate authentication using your browser.

Using your web browser navigate to the REST API service URL/webhook and add the APIKey that's stored in Key Vault - the browser URL might look something like:

```
https://yourAPIServiceURL/webhook?userapikey=00000000-0000-0000-0000-000000000000
```

The browser will display an authentication URL and a code to be used in the authentication process - login using credentials that you typically use for the Azure Sphere CLI (note that you need Contributor or Administrator access for authentication to complete successfully).

### Enable Event Grid message forwarding and filtering
Enabling telemetry forwarding to a WebHook is enabled in the Azure Portal/IoT Hub page by clicking on **Events**

You will need your REST API web hook URL, this will look something like:

```
https://yourAPIServiceURL/webhook
```

Note that the APIKey is only needed to kick off the authentication process, and isn't needed for the WebHook post API.

Use this [Internet of Things show](https://channel9.msdn.com/Shows/Internet-of-Things-Show/IoT-Devices-and-Event-Grid?term=event%20grid&lang-en=true) on MSDN Channel 9 as a guide for setting up IoT Hub/Event Grid with message forwarding and filtering to a WebHook (in the video the WebHook is an Azure Logic App you will point the Event Grid WebHook URL to the REST API).

Note that Event Grid supports five Event Types (Device Created, Device Deleted, Device Connected, Device Disconnected, and Device Telemetry) - You will only need the Device Telemetry event type.

You can allow all IoT Hub messages to flow through Event Grid to your REST API, you can also filter so that only telemetry messages that contain 'NoUpdateAvailable' or 'AppRestart' flow through to the WebHook endpoint.

Once you've configured the Event Grid WebHook integration you can now edit the filtering rule.

In the IoT Hub page left hand menu select **Message routing** - You should see a **RouteToEventGrid** route, with the Routing Query showing as 'true' (meaning that all messages are sent to the WebHook).

Before editing the Routing Query for Event Grid you need to add a new Route:

On the **Routes** page click **Add** - give the new Route a name, something like **all-messages**, select the **events** Endpoint, and the Data Source as **Device Telemetry Messages**. Click **Save**

You can now edit the Event Grid Route. Click on the **RouteToEventGrid** route. You should see that the Routing query is **true**, this means that all messages are sent to the WebHook.

Modify the Routing Query to be:
```
$body.NoUpdateAvailable OR body.AppRestart
```
This will only forward telemetry messages that contain the **NoUpdateAvailable** or **AppRestart** key.

A list of supported expressions and conditions can be found [here](https://github.com/MicrosoftDocs/azure-docs/blob/master/articles/iot-hub/iot-hub-devguide-query-language.md#expressions-and-conditions).

### Using the Desktop Simulator

The Desktop Simulator project is a .NET Core 3.1 application that mimics the behavior of a device by sending **Temperature**, **NoUpdateAvailable** and **AppRestart** telemetry to your IoT Hub.

The simulator needs the connection details of a device that's in your Azure Sphere tenant.

You can obtain the Device IDs of devices in your Azure Sphere tenant at the Azure Sphere CLI using the following command.

```
azsphere device list
```

This will list the Device IDs in your currently selected Azure Sphere tenant.

Copy one of the Device IDs to the clipboard, and then navigate to your IoT Hub in the Azure Portal.

Select **IoT devices** and then select **New** - Enter the Device ID you copied from the device list for your Azure Sphere tenant and select **Save**

You should now see your device ID listed in the IoT Hub devices list - **Select your device** then copy the **Primary Connection String** to the clipboard.

Open the [Device Simulator](./DesktopSimulator/deviceSim.sln) project - open **Program.cs**

Paste the connection string into the empty **connStr** (line 18).

You can now build and run the application - the application will send 'AppRestart' messages every 10 seconds, and 'NoUpdateAvailable' every 60 seconds - these messages will trigger the REST API Service to lookup details of the device and update the Device Twin JSON in IoT Hub.

### Using the Azure IoT sample

You will need to modify the Azure IoT sample from the Azure Sphere samples Github repository.

Copy the files from the [HighLevelApp/Common](./HighLevelApp/Common) directory to your copy of the Azure IoT sample (into the Common folder, the files will overwrite existing files in that folder) - these modified files add support for System Event Notifications - The updated application will send 'NoUpdateAvailable' telemetry to Azure IoT Hub if there aren't any pending updates (the check happens once every 24 hours, or when a device is restarted).

You will need to update the **app_manifest.json** for the project to add the following capability.

```
   "SystemEventNotifications": true
```

**Enabling JSON data flow through Event Grid**

Ensure that your telemetry messages are tagged appropriately for content-type and content-encoding.
Data sent through Azure Event Grid needs to be tagged as 'application/json' and 'utf8', if you don't set the encoding appropriately your telemetry will be sent to EventGrid and the webhook endpoint as a Base64 encoded string.

Modify the `Azure IoT/common/azure_iot.c` source file, AzureIoT_SendTelemetry function to add the Encoding and Content Type properties.

The updated code should look like this:

```cpp
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromString(jsonMessage);
    IoTHubMessage_SetContentEncodingSystemProperty(messageHandle,"utf-8");
    IoTHubMessage_SetContentTypeSystemProperty(messageHandle,"application/json");
```

The AzureIoT sample should now be ready to run on your development board.

### Expected outcome from running the Device Simulator or Updated Azure IoT sample
The device simulator and modified Azure IoT sample send 'NoUpdateAvailable' and 'AppRestart' telemetry messages to your Azure IoT Hub, these messages are then sent to your REST API service which uses the authenticated user to call [Azure Sphere Public APIs](https://docs.microsoft.com/en-us/rest/api/azure-sphere/) to obtain information about the device that's sending the telemetry - this information includes:
- Device OS version
- Device Product
- Device Group (including whether the Device Group is Retail Eval)
- Whether App Updates are enabled

This information is added to the devices 'Desired' Device Twin state, and therefore is replicated on the device - The device can then store this information in mutable storage and potentially display this information to an engineer or user.

Note that the Device Twin information can also be read by a desktop application using the appropriate Azure SDK functions.

## Project expectations

* This code is not official, maintained, or production-ready code.

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