## Azure Sphere - Parse Device Logs  Packages and Tools

This folder contains tools that can assist with the parsing of device log in to human readable format.

| Folder         | Description              |
|---------------------|--------------------------|
| parsedevicelog           | Contains tools that can assist with the parsing device log. |

## parse-devicelog.py

The parse-devicelog.py file contains the logic for parsing the device log binary file into human readable format. 

## params
-d path to OS manifest directory 
-f devicelog bin filename path 

e.g c:> parse-devicelog.py -d 22.07 -f AzureSphere_DeviceLog_113.bin

## error-code.py

parse-devicelog.py file imports error-code info from the commonerror_yml folder, which coverts the error code into common error name 

e.g  Parsed device log bin file =>   [0.088000] Info appman_permissions(30): Initializing permissions for peripheral type (type: 1)
     Parsed device log bin file with error code  =>     [0.088000] Info appman_permissions(30): Initializing permissions for peripheral type (type: {'code': 1, 'name': 'LinuxError::1', 'message': 'LinuxError error code: 1', '_is_untranslated': False})
 

