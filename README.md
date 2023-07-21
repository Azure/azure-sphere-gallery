# Azure Sphere Gallery

This repository contains a collection of inspirational, unmaintained [Azure Sphere](https://www.microsoft.com/azure-sphere/) software samples and hardware designs ready to be reused.

For maintained samples and designs, see the official Azure Sphere Samples, Azure Sphere Tools, and Azure Sphere Hardware Designs repos:

- https://github.com/Azure/azure-sphere-samples
- https://github.com/Azure/azure-sphere-tools
- https://github.com/Azure/azure-sphere-hardware-designs

## Using the samples and designs

See the [Azure Sphere Getting Started](https://www.microsoft.com/en-us/azure-sphere/get-started) page for background on Azure Sphere.

Each sample/hardware design will include instructions in the readme for any system requirements, maintenance expectations, who to contact for support, and the amount of testing completed.

Each folder within this repository contains a README.md file that describes the samples or hardware designs therein. Follow the instructions in each individual readme.

## List of projects

| Folder name | Description |
| ----------- | ----------- |
| [AzureFunctionApp_AzureSpherePublicAPI](AzureFunctionApp_AzureSpherePublicAPI) | This project shows how to create a Python Azure Function app that uses Managed Service Identity to call the Azure Sphere Public API |
| [AzureIoT_StoreAndForward](AzureIoT_StoreAndForward) | This project shows how to add telemetry store and forward capability to the AzureIoT sample |
| [AzureSphereSquirrel](AzureSphereSquirrel) | This project shows how to run a high-level scripting langauge (Squirrel Lang) on MT3620 for powerful high-level API use, code hot-reload and unit testing. |
| [BalancingRobot](BalancingRobot) | This project contains the hardware, enclosure, and software files needed to build an Azure Sphere and Azure RTOS self-balancing robot. |
| [CO2 Monitor healthy spaces](CO2_MonitorHealthySpaces) | This project integrates a CO2 monitor with IoT Central. It covers security, driver development, IoT Plug and Play, deferred updates, and watchdogs. |
| [CrashDumpsConfigure](CrashDumpsConfigure) | Script that can be used to get or set crash dump policy for your device groups in the Azure Sphere Security Service |
| [AzureIoTMessageWithProperties](AzureIoTMessageWithProperties) | A project that shows how to add custom properties to an Azure IoT Hub telemetry message, which can then be used for message routing |
| [AzureSphereTenantDeviceTwinSync](AzureSphereTenantDeviceTwinSync) | A project that shows how to update IoT Hub device twin with OS Version, Product/Device Group and more based on telemetry triggers using Azure IoT Hub/EventGrid and a custom ASP.NET Core REST API  |
| [DeviceTenantFinder](DeviceTenantFinder) | Utility that makes it easy to find the Azure Sphere tenant name/Id within your tenants for a given Device Id |
| [DNSServiceDiscoveryUnicast](DNSServiceDiscoveryUnicast) | Application showing unicast DNS service discovery to allow discovery of and connection to services outside of the local network |
| [DRAMClickboard](DRAMClickboard) | Application showcases the capabilites of interacting with a Mikro DRAM Click board on Azure Sphere |
| [EAP-TLS_Solution](EAP-TLS_Solution) | A library & demo solution implementation for Azure Sphere-based devices connecting to Extensible Authentication Protocol – Transport Layer Security (EAP-TLS) networks. |
| [EncryptedStorage](EncryptedStorage) | A project demonstrating simple use of the wolfCrypt authenticated encryption APIs |
| [GetDeviceOSVersion](GetDeviceOSVersion)| A project that retrieves the OS versions for all Azure Sphere based devices.|
| [Grove_16x2_RGB_LCD](Grove_16x2_RGB_LCD) | A project that shows how to integrate a Seeed Grove LCD/RGB 16x2 display |
| [HeapTracker](HeapTracker) | A memory allocation tracking library that can help diagnose memory leaks. |
| [LittleFs_Encrypted_RemoteDisk](LittleFs_Encrypted_RemoteDisk) | A project that shows how to add encrypted, authenticated storage using [Littlefs](https://github.com/littlefs-project/littlefs) to an Azure Sphere project (with remote storage) |
| [LittleFs_RemoteDisk](LittleFs_RemoteDisk) | A project that shows how to add [Littlefs](https://github.com/littlefs-project/littlefs) to an Azure Sphere project, uses Curl to talk to remote storage |
| [LittleFs_SDCard](LittleFs_SDCard) | A project that combines [Littlefs](https://github.com/littlefs-project/littlefs) with SD Card support, and PC utilities to read the SD Card and extract files/folders.|
| [MQTT-C_Client](MQTT-C_Client) | A project that shows how to add the  [MQTT-C](https://github.com/LiamBindle/MQTT-C.git) library to an Azure Sphere project. There is also a host Python app provided for testing purposes. |
| [MultiDeviceProvisioning](MultiDeviceProvisioning) | Shows how multiple Azure Sphere devices can be provisioned automatically and simultaneously from a single PC. |
| [MutableStorageKVP](MutableStorageKVP) | Provides a set of functions that expose Key/Value pair functions (write, read, delete) over Azure Sphere Mutable Storage. |
| [NetworkInterfaceAddresses](NetworkInterfaceAddresses) | A minimal Azure Sphere app that prints the MAC and IP address of the given network interface. |
| [OSNetworkRequirementChecker-HLApp](OSNetworkRequirementChecker-HLApp) | A sample app that performs DNS resolver and custom NTP test for diagnosing networking connectivity problems. |
| [OSNetworkRequirementChecker-PC](OSNetworkRequirementChecker-PC) | A PC command line utility for diagnosing networking connectivity issues. |
| [OpenSourceProjectsSupportingExternalPeripherals](OpenSourceProjectsSupportingExternalPeripherals) | This is a list of open-source projects supporting external hardware on the Azure Sphere MT3620 platform |
| [ParseDeviceLog](https://github.com/Azure/azure-sphere-tools/) | Tools that can parse device logs in to human readable format. **This content has moved to the Azure Sphere Tools repository.** |
| [PWMAudio](PWMAudio) | Generate tones using PWM on the real-time cores |
| [RS485Driver](RS485Driver) | An RS-485 real-time driver with HL-Core interfacing API. |
| [ServiceAPIDeviceCodeAuth](ServiceAPIDeviceCodeAuth) | Code snippet to access public rest api using device code flow from a web app. |
| [SetIoTCentralPropsForDeviceGroup](SetIoTCentralPropsForDeviceGroup) | Utility that makes it easy to set an Azure IoT Central Device Twin property for all devices in an Azure Sphere Device Group. |
| [SetTimeFromLocation](SetTimeFromLocation) | Project that shows how to use Reverse IP lookup to get location information, then obtain time for location, and set device time. |
| [SimpleFileSystem_RemoteDisk](SimpleFileSystem_RemoteDisk) | Project that shows how to add a simple FIFO/circular buffer file system to an Azure Sphere project, uses Curl to talk to remote storage|
| [ToggleClassicCLI](ToggleClassicCLI) | Scripts for Windows and Linux that allow you to test the removal (and restore) of the Azure Sphere Classic CLI. |
| [TranslatorCognitiveServices](TranslatorCognitiveServices) | Project that shows how to call Microsoft Translator Cognitive Services APIs from Azure Sphere |
| [UdpDebugLog](UdpDebugLog) | Code that demonstrates how to override Log_Debug to broadcast on a UDP Socket, includes a Desktop Viewer application. |
| [VS1053AudioStreaming](VS1053AudioStreaming) | Project that shows how to play audio through a VS1053 codec board |
| [WebHookPublicAPIServicePrincipal](WebHookPublicAPIServicePrincipal) | Project that shows how to use the Azure Sphere Public API with Azure Active Directory Service Principal authentication |
| [WifiConfigurationViaAppResource](WifiConfigurationViaAppResource) | Project that shows how to configure device Wi-Fi settings using an embedded JSON resource file |
| [WifiConfigurationViaNfc](WifiConfigurationViaNfc) | Project that shows how to configure device Wi-Fi settings using NFC |
| [WifiConfigurationViaUart](WifiConfigurationViaUart) | Project that shows how to configure device Wi-Fi settings using Uart |
| [IndustrialDeviceController](IndustrialDeviceController) | Project that provides a set of tools, service and software to enable collecting real time telemetry data from various CE devices through MODBUS protocol in a secure, low latency and reliable way.|
| [Rust](Rust) | Rust language support for Azure Sphere |

## Contributing
This project contains a collection of Microsoft employee demos, hacks, hardware designs, and proof of concepts.  Microsoft authors of new contributions should fork the repo and create a pull request using a new folder, using the appropriate README template for [software](Templates/software-readme-template.md) or [hardware](Templates/hardware-readme-template.md) contributions, and following the checklist in the [pull request template](.github/pull_request_template.md).

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Licenses

For information about the licenses that apply to a particular sample or hardware design, see the License and README.md files in each subdirectory.

