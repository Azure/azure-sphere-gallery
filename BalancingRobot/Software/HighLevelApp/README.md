# High Level App

The high level app runs on the MT3620 A7 core - the application sends telemetry to an Azure IoT Central application, read the [Azure_Sphere_Robot_Software_Setup_Guide](../../Documentation/Azure_Sphere_Robot_Software_Setup_Guide.pdf) which details how to:

* Setup an Azure IoT Central application (including Device Template, Telemetry, and Device Settings)
* Create and configure an Azure Sphere tenant
* Create Product and Device Groups within the Tenant

Note The High-Level and Real-Time applications can be deployed to the Robot from Visual Studio/Visual Studio Code if Development has been enabled for the Robot using `azsphere device enable-development`.

The high level application is responsible for interacting with your Azure IoT Central application (sending telemetry, dealing with Device Twin notifications), and also requesting telemetry from the real-time application.

The high level application needs to be configured for your Azure Sphere tenant and Azure IoT Central environment - this requires changes to your application manifest (app_manifest.json).

There are three sections of the app_manifest.json file that you need to modify, these are:

### CmdArgs 
The first CmdArgs array entry contains the ID scope of your Azure IoT Central application (the ID scope can be found under **Administration | Device Connection**)

The second CmdArgs array entry contains the App identifier (either 'AppA' or 'AppB') used to show the Robot software version on the front display.

### AllowedConnections

`AllowedConnections` contains the list of endpoints this application can talk to, follow the [instructions in the Azure IoT sample](https://github.com/Azure/azure-sphere-samples/blob/main/Samples/AzureIoT/IoTCentral.md#configure-the-sample-application-to-work-with-your-azure-sphere-tenant-and-devices) to configure the AllowedConnections list.

### DeviceAuthentication

`DeviceAuthentication` contains the tenant ID for the Azure Sphere tenant your Robot is claimed into, use `azsphere tenant list` to show the list of tenants and copy the guid for the appropriate tenant.

## Building two versions of the app for demo
To demo the OTA update capabilities of the robot, you need to build two versions of the app, known as A and B.
The high level app has a build-time flag to determine whether AppA or AppB is displayed.

To configure the A7 app build to select AppA or AppB simply modify the app_manifest.json file as above.

## Header/C files

The high level application uses several header/c pairs, these are:

| Header/C files | Description |
|-------------|-------------|
| [MutableStorageKVP](https://github.com/Azure/azure-sphere-gallery/tree/main/MutableStorageKVP) | Azure Sphere Gallery project to read/write key/value pairs into mutable storage |
| [UdpDebugLog](https://github.com/Azure/azure-sphere-gallery/tree/main/UdpDebugLoghttps://github.com/Azure/azure-sphere-gallery/tree/main/UdpDebugLog) | Azure Sphere Gallery project to broadcast debug data over UDP |
| GetDeviceHash | Creates a random 4 byte ID for the Robot (used in UdpDebugLog) |
| i2c_oled | Displays data on the robot SSD1306 (32x128px) display |
| intercore | handles encoding of intercore messages |
| parson  | JSON Parser - used by the Azure IoT Central Device Twin code |
| SSD1306_icons.h  | contains uint8_t arrays of icon images used on the robot display (wifi, IoT Central connection, battery, and 'AppA/B' |
| utils  | Contains functions for 'IsNetworkReady', delay(milliseconds), and generate guid |

