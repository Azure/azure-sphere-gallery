# Get device os version

The goal of this project is to show how to get the OS version of an MT3620 device.

## Contents

| File/folder  | Description                                                                                 |
| ------------ | ------------------------------------------------------------------------------------------- |
| `src\CSharp` | Contains the C# project and source files that demonstrate how to get the device OS version. |
| `src\Python` | Contains the Python file that demonstrates how to get the device OS version.                |
| `README.md`  | This README file.                                                                           |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- For the Python script, [install and/or upgrade  pip](https://pip.pypa.io/en/stable/installing/) and install [`azuresphere-device-api`](https://pypi.org/project/azuresphere-device-api/)
- For the C# project, install [`Microsoft.Azure.Sphere.DeviceAPI`](https://www.nuget.org/packages/Microsoft.Azure.Sphere.DeviceAPI).


## Project expectations

* The code has been developed to show how to get the OS version of an MT3620 device, based on [the C#/Python package project](https://github.com/Azure/azure-sphere-samples/tree/main/Manufacturing) - It is not official, maintained, or production-ready code.

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
