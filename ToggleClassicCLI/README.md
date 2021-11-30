# Azure Sphere Classic CLI Scripts (disable/enable Azure Sphere Classic CLI)

This project provides scripts for Windows and Linux that allow you to test the removal (and restore) of the Azure Sphere Classic CLI. Classic CLI has been superceded by CLIv2 and was marked as deprecated from February 2021, and will be removed from the Azure Sphere SDK, see [Azure Sphere CLI migration guidance](https://docs.microsoft.com/en-gb/azure-sphere/reference/classic-cli-migration?tabs=cliv2beta) for more information.

## Contents

| File/folder | Description |
|-------------|-------------|
| `Windows` | Windows Powershell script to disable/enable Azure Sphere Classic CLI |
| `Linux` | Linux Bash script to disable/enable Azure Sphere Classic CLI |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites & Setup

- An Azure Sphere-based device with development features (see [Get started with Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/get-started/) for more information).
- Setup a development environment for Azure Sphere (see [Quickstarts to set up your Azure Sphere device](https://docs.microsoft.com/en-us/azure-sphere/install/overview) for more information).

## How to use

### Windows
Copy the ToggleClassicCLI PowerShell script from the `Windows` folder to your development PC. 

Note: The ToggleClassicCLI PowerShell script will need to run with elevated permissions (and will prompt if you are not running with the appropriate permissions).

The script checks whether `C:\Program Files (x86)\Microsoft Azure Sphere SDK\Tools` exists (Classic CLI enabled), if the `Tools` folder exists the folder is renamed and hidden and the `Azure Sphere Classic Developer Command Prompt (Deprecated)` is removed. If the hidden folder exists then this is renamed back to `Tools`, the hidden attribute is removed, and the Azure Sphere Classic Developer Command Prompt (Deprecated) is restored.

Note: If you decide to uninstall the Azure Sphere SDK after disabling the Azure Sphere Classic CLI you should re-run the ToggleClassicCLI PowerShell script before running the uninstaller, this will ensure that all SDK files are removed from your development PC.


### Linux

Copy the ToggleClassicCLI bash script from the `Linux` folder to your development PC, note that you will need to enable execute permissions on the script either through the shell, or through the terminal (`chmod 755 ToggleClassicCLI.sh`). 

Note: The ToggleClassicCLI bash script will need to run with elevated permissions (and will prompt if you are not running with the appropriate permissions).

The script checks whether `/opt/azurespheresdk/Tools` exists (Classic CLI enabled), if the `Tools` folder exists the folder is renamed and hidden (the `azsphere_v1` link will not work). 

If the hidden folder exists then this is renamed back to `Tools` - The script will also prompt to ask whether you want Azure Sphere CLIv2 or Classic CLI enabled as the default CLI.


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

