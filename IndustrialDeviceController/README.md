# IndustrialDeviceController project

This project contains the hardware and software files needed to build an Azure Sphere based solution for data ingestion. It targets to provide a set of tools, service and software to enable collecting real time telemetry data from various CE devices through MODBUS protocol in a secure, low latency and reliable way. All the telemetry data been collected will be sent to IoT Hub, so these telemetry data can be easily consumed by other applications like monitoring system.

## Contents

| File/folder | Description |
|-------------|-------------|
| `HardwareDefinitions`       | Hardware definition files for Azure Sphere hardware |
| `Software`       | High-Level application |
| `README.md` | This README file |
| `LICENSE.txt`   | The license for the project |
| `ThirdPartyNotices.txt`   | The notice file for Third-party OSS used by the project |

## Instructions

To build out the IndustrialDeviceController, please refer to the README file in the [Software](Software) directory.

## Project expectations

The project is a proof of concept. It is not official, maintained, or production-ready code.

### Expected support for the code
This code is unsupported.

### How to report an issue
If you run into an issue with this project, please open a GitHub issue against this repo.

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

This project builds on open source components, these are listed in [ThirdPartyNotices.txt](./ThirdPartyNotices.txt)