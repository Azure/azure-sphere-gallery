# OSNetworkRequirementChecker-PC

This PC command line utility enables you to test some aspects of the [Azure Sphere OS Networking Requirements](https://docs.microsoft.com/azure-sphere/network/ports-protocols-domains) using a PC attached to the network that you wish to attach an Azure Sphere device to. This makes it easier to determine if a given network environment does not meet Azure Sphere OS's networking requirements, which can cause issues such as lack of time synchronization, inability to receive OS or Application updates, and lack of short-lived device certificate which means that an application's Azure IoT connections may not work properly. 

This application tests the following:

1. For the standard NTP endpoints used by the OS, it verifies if a correct time-sync can be retrieved from them, using the same port numbers as the Azure Sphere OS uses.

2. For the list of other endpoints that the OS communicates with, it verifies that the hostnames can be resolved via DNS to IP addresses, and verifies that a TCP connection can be established to those endpoints using the correct port number.

During execution, the utility displays whether each check fails or succeeds.

## Contents
| File/folder | Description |
|-------------|-------------|
| `main.cpp` | The console application source code. |
| `README.md` | This README file. |
| `LICENSE.txt` | The license for the project. |

## Prerequisites

A PC running Windows or Linux to run the utility on, which must be connected to the same network on which the Azure Sphere devices are (or will be) connected.

## Setup

Copy the all the project files in a working directory of your choice, from now on referred as `<WORKDIR>`.

## Running the code

### Build and run the app
The command line utility has a single source file which supports compiling on Windows (i.e. Visual Studio or Visual Studio Code) and Linux (i.e. Visual Studio Code or g++):

#### On Windows (Visual Studio)
Open the `OSNetworkRequirementChecker-PC.sln` from Visual Studio and build it. The executable should be generated in the following directory:
```
<WORKDIR>\x64\Debug\OSNetworkRequirementChecker-PC.exe
```

#### On Linux (g++)
Open a terminal window, navigate to your `<WORKDIR>` and run the following command:

```
~/<WORKDIR> $ g++ -o OSNetworkRequirementChecker-PC main.cpp
```
Once compiled, the only required executable `OSNetworkRequirementChecker-PC` will be located in the same `<WORKDIR>` directory.

### Testing the app
To run the application, just the single executable file produced from the previous steps is required:

#### On Windows
```
.\<WORKDIR>\x64\Debug>OSNetworkRequirementChecker-PC.exe
```
#### On Linux
```
sudo ./OSNetworkRequirementChecker-PC
```
When running the application, it'll display the checks being executed and their related results, as they are performed.
A successful output will look the following (errors will be displayed inline in case they occur):
```     
        Azure Sphere network-checkup utility.

        Querying required NTP servers...
        - time from time.windows.com<168.61.215.74> --> Wed Aug 18 13:52:03 2021
        - time from time.sphere.azure.net<168.61.215.74> --> Wed Aug 18 13:52:03 2021
        - time from prod.time.sphere.azure.net<168.61.215.74> --> Wed Aug 18 13:52:03 2021
        - time from 168.61.215.74<168.61.215.74> --> Wed Aug 18 13:52:03 2021
        - time from 20.43.94.199<20.43.94.199> --> Wed Aug 18 13:52:03 2021
        - time from 20.189.79.72<20.189.79.72> --> Wed Aug 18 13:52:03 2021
        - time from 40.81.94.65<40.81.94.65> --> Wed Aug 18 13:52:04 2021
        - time from 40.81.188.85<40.81.188.85> --> recvfrom() failed with error 10060 
        - time from 40.119.6.228<40.119.6.228> --> Wed Aug 18 13:52:14 2021
        - time from 40.119.148.38<40.119.148.38> --> Wed Aug 18 13:52:14 2021
        - time from 20.101.57.9<20.101.57.9> --> Wed Aug 18 13:52:14 2021
        - time from 51.137.137.111<51.137.137.111> --> Wed Aug 18 13:52:15 2021
        - time from 51.145.123.29<51.145.123.29> --> Wed Aug 18 13:52:15 2021
        - time from 52.148.114.188<52.148.114.188> --> Wed Aug 18 13:52:15 2021
        - time from 52.231.114.183<52.231.114.183> --> Wed Aug 18 13:52:15 2021

        Querying required endpoints...

        Device provisioning and communication with IoT Hub:
        - Resolving global.azure-devices-provisioning.net... success --> connecting to 40.64.134.6:8883... success!
        - Resolving global.azure-devices-provisioning.net... success --> connecting to 40.64.134.6:443... success!

        Internet connection checks, certificate file downloads, and similar tasks:
        - Resolving www.msftconnecttest.com... success --> connecting to 13.107.4.52:80... success!
        - Resolving prod.update.sphere.azure.net... success --> connecting to 72.21.81.200:80... success!

        Communication with web services and Azure Sphere Security service:
        - Resolving anse.azurewatson.microsoft.com... success --> connecting to 51.143.121.203:443... success!
        - Resolving eastus-prod-azuresphere.azure-devices.net... success --> connecting to 20.49.110.135:443... success!
        - Resolving prod.core.sphere.azure.net... success --> connecting to 20.66.4.193:443... success!
        - Resolving prod.device.core.sphere.azure.net... success --> connecting to 20.66.4.195:443... success!
        - Resolving prod.deviceauth.sphere.azure.net... success --> connecting to 20.62.133.98:443... success!
        - Resolving prod.dinsights.core.sphere.azure.net... success --> connecting to 20.66.4.195:443... success!
        - Resolving prod.releases.sphere.azure.net... success --> connecting to 13.107.246.69:443... success!
        - Resolving prodmsimg.blob.core.windows.net... success --> connecting to 52.239.154.132:443... success!
        - Resolving prodmsimg-secondary.blob.core.windows.net... success --> connecting to 52.239.162.36:443... success!
        - Resolving prodptimg.blob.core.windows.net... success --> connecting to 52.240.48.36:443... success!
        - Resolving prodptimg-secondary.blob.core.windows.net... success --> connecting to 52.239.104.36:443... success!
        - Resolving sphereblobeus.azurewatson.microsoft.com... success --> connecting to 13.107.213.69:443... success!
        - Resolving sphereblobweus.azurewatson.microsoft.com... success --> connecting to 13.107.213.69:443... success!
        - Resolving sphere.sb.dl.delivery.mp.microsoft.com... success --> connecting to 152.195.19.97:443... success!
```

**Note**: like in the output above, issues connecting to 40.81.188.85 are expected when using a commercial ISP in the U.S..
In case this happens, it does not represent a problem as far as at least one NTP server can be reached and replies with the correct time-sync.

## Next steps

### Project expectations

For the latest information regarding the network connectivity requirements for Azure Sphere-based devices, please refer to the official [Azure Sphere OS networking requirements](https://docs.microsoft.com/en-us/azure-sphere/network/ports-protocols-domains) documentation.

### Expected support for the code

There is no official support guarantee for this code, but we will make a best effort to respond and address any issues you may encounter.

### How to report an issue

If you run into an issue with this script, please open a GitHub issue against this repo.

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

For information about the licenses that apply to this script, see [LICENSE.txt](./LICENCE.txt)