# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

import provision
import sys
import time

def configure_wifi(device):

    result = provision.azspherecommand(["device", "wifi", "show-status"], device)
    if result.returncode != 0:
        return False # not ok

    # add wifi if missing
    result = provision.azspherecommand(["device", "wifi", "list"], device)
    if result.returncode != 0:
        return False # not ok
    
    if bytes(wifi_ssid, 'utf-8') not in result.stdout:
        result = provision.azspherecommand(["device", "wifi", "add", "-s", wifi_ssid, "-p", wifi_network_key], device)
        if result.returncode != 0:
            return False # not ok
        print("Added WiFi network to device at " + device["DeviceConnectionPath"])
    else:
        print("No need to add WiFi network to device at " + device["DeviceConnectionPath"])
    return True

if len(sys.argv) != 3:
    print("Please provide the SSID and Network Key")
    sys.exit(-1)

wifi_ssid=sys.argv[1]
wifi_network_key=sys.argv[2]

print("Configuring Wi-Fi for any attached Azure Sphere devices in parallel (",wifi_ssid, ':', wifi_network_key, "). Press Ctrl-C to exit.")

ret = provision.perdevice(configure_wifi)

sys.exit(ret)