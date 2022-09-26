# Multi-Device Provisioning

These example scripts show how multiple Azure Sphere devices can be provisioned automatically and simultaneously from a single PC.
The provisioning script:
- runs in a loop until Ctrl-C forces exit
- detects when each Azure Sphere device is attached. When a new device is attached:
  - runs a series of actions on that device
  - after the actions finish, blinks the USB serial activity LED slowly (1Hz) if successful or fast (2.5Hz) if there was an error, allowing the user to observe the result visually
  - logs the results to console
- does this for many devices in parallel (we've tried up to 10)

## Contents

| File/folder | Description |
|-------------|-------------|
| provision.py  | the main Python script, used by the scripts below  |
| provision_deleteapps.py  | script to delete applications  |
| provision_wifi.py  | script to add wifi  |
| LICENSE.txt | License |
| README.md | This readme file |

## Prerequisites

- The scripts run on Windows, and depend on the [Azure Sphere SDK](https://learn.microsoft.com/azure-sphere/install/install-sdk) and [Azure Sphere Command Line Interface (CLIv2)](https://learn.microsoft.com/azure-sphere/reference/azsphere-cli).
- [`python`](https://www.python.org/downloads/) (3.x)

## How to use

The project contains a core Python script (provision.py), and a couple of sample scripts (provision_deleteapps.py, and provision_wifi.py).

Here's an example of running the provision_wifi.py script.

```makefile
C:\MultiDeviceProvisioning>python provision_wifi.py mySSID NetworkKey
Configuring Wi-Fi for devices ( mySSID : NetworkKey )  Press Ctrl-C to exit.
Processing new device (with 5s wait) at connection path 1213413
Added WiFi network to device at 1213413
                Device at 1213413 completed successfully
Exiting but there are still 1 attached devices
```

Note that the script will monitor device plug in and removal, and will provision any new devices that are found, so you can use this script to apply a series of actions to many devices.

If the devices include a USB-serial activity LED (like the Reference Development Board), then the script causes this LED to flash fast (2.5Hz) if there was a failure or slow (1Hz) on success, enabling the user to (a) see when the operation is complete, and (b) determine devices that have failed provisioning.

## Key concepts

The sample scripts are executing commands supported by the Azure Sphere CLIv2 and parsing the results from the CLI command.

Let's look at part of the provision_wifi.py script. The script takes two command line parameters, these are the network SSID and Network Key.

In the snippet below the script executes the `azsphere device wifi list` command to obtain the current Wi-Fi configuration for a device, and then checks to see whether the user provided SSID is in the stdout results. If the SSID is not found, then the command `azsphere device wifi add -s <ssid> -p <network key>` is executed.

```python
# add wifi if missing
result = provision.azspherecommand(["device", "wifi", "list"], device)
if result.returncode != 0:
    return False # not ok

if bytes(wifi_ssid, 'utf-8') not in result.stdout:
    result = provision.azspherecommand(["device", "wifi", "add", "-s", wifi_ssid, "-p", wifi_network_key], device)

```
With this information it should be relatively simple to modify one of the sample scripts, or create your own scripts.

## Next steps

- For more information on Azure Sphere command line interface commands, see [Azure Sphere Command Line Interface (CLIv2)](https://learn.microsoft.com/azure-sphere/reference/azsphere-cli)

## Sample expectations

- The scripts are designed for developer or test usage only. For manufacturing production devices, please refer to the official [Manufacturing Tools](https://learn.microsoft.com/azure-sphere/hardware/factory-floor-tasks).

### Expected support for the code

There is no official support guarantee for this code, but we will make a best effort to respond to/address any issues you encounter.

### How to report an issue

If you run into an issue with this script, please [open a GitHub issue against this repo](https://github.com/Azure/azure-sphere-gallery/issues).

## Licenses

See [LICENSE.txt](./LICENSE.txt)