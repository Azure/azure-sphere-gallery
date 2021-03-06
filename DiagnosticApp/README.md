# Sample: Diagnostic Application

**Note:** In Progress. Currently supporting DNS Resolver and Custom NTP Test.

This application performs two device (MT3620) networking diagnostic tests. The first one tests the DNS resolver functionality against known prod end-points; the second one tries time-syncing with dedicated NTP servers around the world. After running both ones, the app generate a summary report to the device console, which contains diagnostic information about whether each test failed or succeeded.

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [Networking](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-networking/networking-overview) | Manages network connectivity |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Device Output window during debugging |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

## Contents
| File/folder | Description |
|-------------|-------------|
| `src`       | Sample application |
| `README.md` | This README file. |
| `LICENSE.txt`   | The license for the project. |

## Prerequisites

1. Internet connection through Wi-Fi or Ethernet.

## Prepare the sample

1. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 20.10 or above. At the command prompt, run **azsphere show-version** to check. Install [the Azure Sphere SDK](https://docs.microsoft.com/azure-sphere/install/install-sdk) as needed.
1. Connect your Azure Sphere device to your computer by USB.
1. Connect your Azure Sphere device to the same local network as the DNS service.
1. Enable application development, if you have not already done so:

   `azsphere device enable-development`

### Network configuration

By default, this sample runs over a Wi-Fi connection to the internet. To use Ethernet instead, make the following changes:

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

## Build and run the sample

To build and run this sample, follow the instructions in [Build a high-level application](https://docs.microsoft.com/en-us/azure-sphere/install/qs-blink-application?tabs=windows%2Ccliv2beta&pivots=vs-code).


## Testing the sample

When you run the application, towards the very end, it should displays a summary section in this format:

        Index:  0
        Name:   prod end-point name
        IPv4:   ###.###.###.###
        Alias:  alias
       
       ...... 

        Custom NTP Time Server Sync list:
        Index: 0, Name: ###.###.###.###, UTC time before sync: t1, after sync: t2

      ......

## Expected support for the code
The app is an ongoing project. More features will added in the future, and will try to reply to issues once per quarter.

## License

For details on license, see LICENSE.txt in this directory.