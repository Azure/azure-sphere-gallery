# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

import time
import urllib.request
import json
from threading import Thread
import socket
import sys
import subprocess
import shutil

def done(device, ok):
    if ok is None:
        print("WARNING: per device action returned 'None' - assuming success")
        ok = True

    if device['removed']:
        return

    device['done'] = True
    if ok:
        delay=0.5
        print("\t\tDevice at " + device['DeviceConnectionPath'] + " completed successfully")
    else:
        delay=0.2
        print("ERROR: Device at " + device['DeviceConnectionPath'] + " completed with error")

    while True:
        starttime = time.time()
        while time.time() - starttime < delay:
            if device['removed']:
                return
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
            sock.sendto(bytes("123456789012345678901234567890123456789012345678901234567890", "utf-8"), (device['IpAddress'], 12345))
            time.sleep(0.05)
        time.sleep(delay)
        if device['removed']:
            return

def doaction(per_device_action, device):
    time.sleep(5)
    done(device, per_device_action(device))

def perdevice(per_device_action):
    listattachedurl="http://localhost:48938/api/service/devices"
    deviceList = []

    while True:
        for device in deviceList:
            device['seen'] = False

        curList = []
        try:
            la = urllib.request.urlopen(listattachedurl)
            if(la.getcode()==200):
                data = la.read()
                curList = json.loads(data)
            else:
                print("Error receiving data", la.getcode())
        except:
            print("Error talking to slip service")
            continue

        for curListItem in curList:
            found=False
            for deviceListItem in deviceList:
                if deviceListItem['DeviceConnectionPath'] == curListItem['DeviceConnectionPath']:
#                    print("existing device " + curListItem['DeviceConnectionPath'])
                    deviceListItem['seen'] = True
                    found=True
            if not found:
                print("Processing new device (with 5s wait) at connection path " + curListItem['DeviceConnectionPath'])
                item = {
                    'DeviceConnectionPath' : curListItem['DeviceConnectionPath'],
                    'IpAddress' : curListItem['IpAddress'],
                    'seen': True,
                    'done': False,
                    'removed': False
                }
                deviceList.append(item)
                t=Thread(target=doaction, args=[per_device_action, item])
                t.daemon=True
                t.start()

        for deviceListItem in deviceList:
            if not deviceListItem['seen']:
#                print("removed device " + deviceListItem['DeviceConnectionPath'])
                deviceListItem['removed'] = True
                if not deviceListItem['done']:
                    print("ERROR: removed device at " + deviceListItem['DeviceConnectionPath'] + " before processing complete!")
                else:
                    print("Device at " + deviceListItem['DeviceConnectionPath'] + " removed")
                deviceList.remove(deviceListItem)

        try:
            time.sleep(0.2)
        except:
            if len(deviceList) == 0:
                return 0
            else:
                print("Exiting but there are still " + str(len(deviceList)) + " attached devices")
                return 1

def azspherecommand(args, device = None):
    if args is None:
        print("azspherecommand requires a string arg list and an optional device")
        return None

    path=shutil.which('azsphere')

    if (path is None):
        print("install the Azure Sphere SDK from: https://learn.microsoft.com/azure-sphere/install/overview")
        return None

    azspherepath=path
    args = [azspherepath] + args
    if device is not None:
        args = args + ["--device", device['DeviceConnectionPath']]

    result = subprocess.run(args, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
    if result.returncode != 0:
        if device is not None:
            print("Azsphere command " + ' '.join(args) + " on device at " + device['DeviceConnectionPath'] + " failed with output:")
        else:
            print("Azsphere command " + ' '.join(args) + " failed with output:")
        print(result.stdout)
    return result