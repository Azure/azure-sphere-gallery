# Azure Sphere Squirrel
The goal of this project is to show that it is possible to run a higher-level scripting language atop the MT3620 and to highlight the business-grade productivity it can bring in the form of:
- A more succinct syntax (e.g async HTTPS in <5 lines, simplified async multi-tasking etc).
- Rapid iteration through instant deploy and test.
- Easier code reuse, through more powerful abstractions.
- Powerful unit test through the use of a higher-level framework, that can run on or off chip for CI/CD.

## Contents
| File/folder | Description |
|-------------|-------------|
| `HLCore`       | Contains the source code and project configuration for the High-Level Core. |
| `README.md`    | This README file. |
| `LICENSE.txt`  | The license for the project. |

## Prerequisites
VS Code / Visual Studio, an Azure account and [Avnet Azure Sphere MT3620 Starter Kit 2.0](https://www.avnet.com/wps/portal/us/products/new-product-introductions/npi/azure-sphere-mt3620-starter-kit-2-0).

## Setup
1. Ensure to initialise and pull Git submodules.
2. Install the [Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk) on PC.
3. Place the device into development mode by executing `azsphere device enable-development` from the command line.

## High level application
The high-level application is responsible for running the Squirrel-Lang VM and Compiler, exposing HLCore APIs to Squirrel source code and executing example Squirrel-Lang applications. The [HLCore README](./HLCore/README.md) provides more information.

## Project Expectations
- This project is a demonstrator and has not been tested for use in a production environment.
- This project does not (yet) expose all HLCore APIs to the Squirrel VM.
- Line information within Squirrel runtime error messages is not correct.
  - Line information requires 8 bytes of storage per line. This is fine for small-scale projects, but really adds-up for larger scale ones, and so the injection of line information into the compiled closure has been commented out.
- Code is provided as-is with support provided on a best-effort basis.

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
For details on license, see LICENSE.txt in this directory.