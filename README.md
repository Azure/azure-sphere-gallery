# Azure Sphere Gallery

This repository contains a collection of inspirational, unmaintained [Azure Sphere](https://www.microsoft.com/azure-sphere/) software samples and hardware designs ready to be reused.

For maintained samples and designs, see the official Azure Sphere Samples and Azure Sphere Hardware Designs repos:

- https://github.com/Azure/azure-sphere-samples
- https://github.com/Azure/azure-sphere-hardware-designs

## Using the samples and designs

See the [Azure Sphere Getting Started](https://www.microsoft.com/en-us/azure-sphere/get-started/) page for background on Azure Sphere.

Each sample/hardware design will include instructions in the readme for any system requirements, maintenance expectations, who to contact for support, and the amount of testing completed.

Each folder within this repository contains a README.md file that describes the samples or hardware designs therein. Follow the instructions in each individual readme.

## List of projects

| Folder name | Description |
| ----------- | ----------- |
| [CrashDumpsConfigure](CrashDumpsConfigure) | Script that can be used to get or set crash dump policy for your device groups in the Azure Sphere Security Service |
| [DeviceTenantFinder](DeviceTenantFinder) | Utility that makes it easy to find the Azure Sphere tenant name/Id within your tenants for a given Device Id|
| [HeapTracker](HeapTracker) | A memory allocation tracking library that can help diagnose memory leaks. |
| [SetIoTCentralPropsForDeviceGroup](SetIoTCentralPropsForDeviceGroup) | Utility that makes it easy to set an Azure IoT Central Device Twin property for all devices in an Azure Sphere Device Group. |
| [UdpDebugLog](UdpDebugLog) | Code that demonstrates how to override Log_Debug to broadcast on a UDP Socket, includes a Desktop Viewer application. |

## Contributing
This project contains a collection of Microsoft employee demos, hacks, hardware designs, and proof of concepts.  Microsoft authors of new contributions should fork the repo and create a pull request using a new folder, using the appropriate README template for [software](Templates/software-readme-template.md) or [hardware](Templates/hardware-readme-template.md) contributions, and following the checklist in the [pull request template](.github/pull_request_template.md).

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Licenses

For information about the licenses that apply to a particular sample or hardware design, see the License and README.md files in each subdirectory.
 
