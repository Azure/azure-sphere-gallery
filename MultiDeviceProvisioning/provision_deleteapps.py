# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

import provision
import sys

def deleteapps(device):
    result=provision.azspherecommand(["device", "sideload", "delete"], device)
    return result.returncode == 0
   
print("Deleting apps from any attached Azure Sphere devices in parallel.  Press Ctrl-C to exit.")
ret = provision.perdevice(deleteapps)
sys.exit(ret)