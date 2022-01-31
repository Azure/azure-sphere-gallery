# Native Applibs

An x86 implementation of libapplibs. Allows building and running Azure Sphere apps natively on Linux

## Contents

| File/folder                        | Description                                             |
|------------------------------------|---------------------------------------------------------|
| `include`                          | The native applibs includes                             |
| `lib`                              | The native applibs libraries                            |
| `AzureSphereNativeToolchain.cmake` | The CMake toolchain file to use the native applibs      |
| `CMakePresets.json`                | The CMake configuration file to use the native applibs  |
| `nativeinstall.sh`                 | The install script to install native applibs components |
| `README.md`                        | The README file.                                        |
| `LICENSE.txt`                      | The license for the project.                            |

## Prerequisites

  - Ubuntu 20.04 (either native or through WSL)
  - Azure Sphere SDK
  - (Optional) Visual Studio Code and the Azure Sphere VS Code extension

## Setup

Requires [libwolfssl24](https://packages.ubuntu.com/focal/libwolfssl24)
  - Run command 'sudo apt-get install -y libwolfssl24' to install

## Running the code

Run the nativeinstall.sh file as sudo
Restart user session
  - You can check that there's now a folder called "/opt/azurespheresdk/NativeLibs"
  - You can also check that the environment variable LD_LIBRARY_PATH contains the path "/opt/azurespheresdk/NativeLibs/usr/lib"
Place the included CMakePresets.json in the root of the project
Generate the project the 'x86-Debug' configure preset, then build
(Optional) Modify the .vscode/launch.json to debug in VS Code
  - Open the Debug window, pull down launch configuration menu, select Add Configuration, and select C/C++: (gdb) Launch
  - Set program to "${workspaceFolder}/out/x86-Debug/<App Name>"
  - Save the file and run that configuration

To uninstall native applibs, run 'nativeinstall.sh -u' as sudo

## Project expectations

This code is not official, maintained, or production-ready.

## Expected support for the code

This code is not formally maintained, but we will make a best effort to respond to/address any issues you encounter.

## How to report an issue

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

For details on license, see LICENSE.txt in this directory.
