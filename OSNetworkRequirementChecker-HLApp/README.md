# OSNetworkRequirementChecker-HLApp

This diagnostic app enables you to test some aspects of the [Azure Sphere OS Networking Requirements](https://docs.microsoft.com/azure-sphere/network/ports-protocols-domains) using an Azure Sphere high-level application. This makes it easier to determine if a given network environment does not meet Azure Sphere OS's networking requirements, which can cause issues such as lack of time synchronization, inability to receive OS or Application updates, and lack of short-lived device certificate which means that an application's Azure IoT connections may not work properly. Currently, this application tests two specific aspects of OS networking requirements: that DNS resolution works for required endpoints, and that NTP time resolution succeeds for the standard NTP endpoints used by the OS.

This application performs two device (MT3620) networking diagnostic tests. The first one tests the DNS resolver functionality against known prod end-points; the second one tries time-syncing with dedicated NTP servers around the world. After running both ones, the app generate a summary report to the device console, which contains diagnostic information about whether each test failed or succeeded.

## Contents
| File/folder | Description |
|-------------|-------------|
| `src`       | application source code |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites

1. Internet connection through Wi-Fi or Ethernet.

## Setup

### Prepare the app
1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 20.10 or above. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk) as needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Connect your Azure Sphere device to the same local network as the DNS service.
1. Enable application development, if you have not already done so:

   `azsphere device enable-development`

### Network configuration

By default, this app runs over a Wi-Fi connection to the internet. To use Ethernet instead, make the following changes:

1. Configure Azure Sphere as described in [Connect Azure Sphere to Ethernet](https://docs.microsoft.com/azure-sphere/network/connect-ethernet).
1. Add the following line to the Capabilities section of the app_manifest.json file:

   `"NetworkConfig" : true`
1. In dns-helper.c, ensure that the global constant `NetworkInterface` is set to "eth0". In source file src/dns-helper.c, search for the following line:

     `char NetworkInterface[] = "wlan0"`;

   and change it to:

     `char NetworkInterface[] = "eth0"`;
1. In main.c, add a call to `Networking_SetInterfaceState` before any other networking calls:

   ```c
    int err = Networking_SetInterfaceState("eth0", true);
    if (err == -1) {
        Log_Debug("Error setting interface state %d\n",errno);
        return -1;
    }
   ```

## Running the code

### Build and run the app
To build and run this app, follow the instructions in [Build a high-level application](https://docs.microsoft.com/azure-sphere/install/qs-blink-application).


### Testing the app
When you run the application, towards the very end, it should displays a summary section in this format:

      Index:  0
      Name:   prod end-point name
      IPv4:   ###.###.###.###
      Alias:  alias

      ...... 

      Custom NTP Time Server Sync list:
      Index: 0, Name: ###.###.###.###, UTC time before sync: t1, after sync: t2

      ......

**Note**: issues connecting to 40.81.188.85 are expected when using a commercial ISP in the U.S.. In case this happens, it does not represent a problem as far as at least one NTP server can be reached and replies with the correct time-sync. There will be no specific error shown, but the failure can bee seen from the Device Output console, where the time-sync will come from a secondary NTP server rather than from the primary one, i.e.:

      INFO: Primary Server: 40.81.188.85
      INFO: Fallback Server NTP Option: 1
      EVENT: Successfully time synced to server prod.time.sphere.azure.net

## Next steps

### Project expectations
This code is not official, maintained, or production-ready code. For maintained guidance on the Azure Sphere OS networking requirements please refer to the [doc page](https://docs.microsoft.com/azure-sphere/network/ports-protocols-domains).

### Expected support for the code
The author will try to reply to issues in future versions.

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