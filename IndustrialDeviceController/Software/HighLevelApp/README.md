# High Level App

The high level app runs on the MT3620 board to collect the telemetry through MODBUS protocol and ingest into IoTHub:
* Configure and interact with Azure Sphere peripherals, such as GPIO, UART and network interface.
* Communicate with cloud-based services
* Parse provision message and load the required driver
* Communicate with industrial device through Modbus protocol and collect telemetry
* Send out the telemetry message in json format to cloud-based services

## Prerequisites

Azure account, [Azure Sphere MT3620 Development Kit](https://www.seeedstudio.com/Azure-Sphere-MT3620-Development-Kit-US-Version-p-3052.html) and TTL to RS485 converter board.

## Hardware Setup

Refer to [MT3620 reference development board (RDB) user guide](https://docs.microsoft.com/en-us/azure-sphere/hardware/mt3620-user-guide).

## Software Setup

Install the [Azure Sphere SDK](https://docs.microsoft.com/en-us/azure-sphere/install/install-sdk?pivots=visual-studio) on PC.
Install and use [Azure IoT explorer](https://docs.microsoft.com/en-us/azure/iot-pnp/howto-use-iot-explorer) to communicate with mt3620 board through IoT hub.
Set up the [IoT Hub Device Provisioning Service](https://docs.microsoft.com/en-us/azure/iot-dps/quick-setup-auto-provision) with the Azure portal.
Record the hostname of IoT Hub and the ID Scope of Device Provisioning Service.

## Build

Open the file app_manifest.json, add the hostname of IoT Hub in AllowedConnections section.
Open the file app_manifest.json, replace the DeviceAuthentication with the UUID of the Azure Sphere tenant to use for device authentication.
Open the file iot/azure_iot_utilities.c, update the scopeId to be the one of your Device Provisioning Service.

#### On Windows (Visual Studio)
Open the cmake file src/CMakeLists.txt from Visual Studio and build. The executable should be generated in the following directory:
IndustrialDeviceController/Software/HighLevelApp/out/

## Contents

| File/folder | Description |
|-------------|-------------|
| `drivers`       | Device module defines the interface between main task and modbus driver |
| `external`       | Contains third-party source code frozen and safeclib |
| `include`       | header files |
| `init`       | Contains the main entry entrance of application, initialization, watchdog and device abstraction layer |
| `iot`       | Handles the message and device twin from IoT Hub and sends messages to IoT Hub  |
| `libutils`       | Contains util functions |
| `app_manifest.json` | The application manifest file |
| `CmakeLists.txt` | The CMake file contains a set of directives and instructions describing source files and targets |
| `CmakeSettings.json` | The CMake setting file |
| `README.md` | This README file |

## IoT Message Definition
IoT message include device to cloud (D2C) and cloud to device (C2D) message, all the message is in json format. For most fields, we use human readable string instead of enumeration to make sure the json file is self-explained, not depend on any external code or DB. Message type been identified by the IoT Message property field "message_type". The reason to use a separate property field is to simplify IoT hub message routing.

D2C message type include:
- telemetry

C2D message include:
- control


### D2C telemetry message
D2C telemetry message is to report device data points periodically. Along with each telemetry message, there is an "error" field to indicate error status for this polling round.

The message format is
```
message_type == "telemetry"
```
```json
{
    "timestamp":"yyyy-mm-dd hh:mm:ss.sss",
    "name":"MWH01_SCHNEIDER_LINK150_GENERATOR",
    "location" : {
        "site":"MWH01",
        "colo" : "colo1",
        "panel" : "rack1"
    },
    "model":"MWH01_POWERMETER_LINK150_V1",
    "points" : [
      ["DP_NAME_1", "DP_VALUE_1"],
      ["DP_NAME_2", "null"],
      ["DP_NAME_3", "DP_VALUE_3"],
      ["error": 0]
    ]
}
```

For points values, the field sequence is [key, value], when failed to read one data point, value will be "?"


### C2D control message
C2D control message is to send control command to device. This include push device provision data request to reset device, request to dump device state, get/set value of certain data points, etc.
The command set can be extended, so the format takes a general format. The "command" field is mandatory "data" field is optional and format is command dependent.
```
message_type == "control"
```
```json
{
    "timestamp":<send time>,
    "command":<command string>,
    "data":<command data>
}
```

current supported command includes
- reset, trigger application to reset, no "data" field.
```json
{
    "timestamp":<send time>,
    "command":"reset",
}
```

- debug, enable remote debug, all info and above log will be sent to iot hub with "message_type" equal to diag.
```json
{
    "timestamp":<send time>,
    "command":"debug",
    "data":true/false
}
```

- provision, push device provision data
```json
{
    "timestamp":<send time>,
    "command":"provision",
    "data":<provision data>
}
```

### Device provision message
Device provision message is a pair of C2D/D2C message for sphere initial provision. As sphere SW is common for all connected devices and it has no local storage to persist settings. When sphere unit comes on line, it needs to ask cloud service regarding its configuration, this is called provision process. The provision can be request/response when device need provision data immediately. Or it can be unsolicited message from cloud when need to push a new configuration to sphere. Empty provision data is also allowed to prevent Sphere keep send provision request when provision data not ready in cloud yet or simply we want to disable one Sphere unit temporarily.

The request/response provision procedure is

- First sphere send a provision request via d2c control message
- Cloud service look for the data for this sphere according to its id.
- Cloud send data to sphere via c2d control message
- After sphere received provision data, it will create agent instances for connected devices and start to poll and report data.
- If sphere failed to receive provision data or fail to parse data, it will ask cloud service again.

The provision request message will use below format
```
message_type == "control"
```
```json
{
    "timestamp":<send time>,
    "command":provision,
    "data":<device id?>
}
```

The provision response message will use below format
```
message_type == "control"
```
```json
   [
      {
        "name": "SAT09_LINK150_GENERATOR",
        "protocol": "MODBUS_TCP",
        "report_interval_ms": 10000,
        "connection": {
          "server_id":40,
          "port": 5020,
          "ip": "10.105.24.226"
        },
        "location": {
          "site": "MWH01",
          "colo": "COLO1",
          "panel": "GENERATOR01"
        },
        "model": "DEIF-AGC-233",
        "schema": [
          [
            "discrete_01",
            100001,
            "bit",
            0,
            1,
            0
          ],
          [
            "holding_bit14",
            400004,
            "bit",
            14
          ],
        ]
      },
      {
        "name": "SAT09_AHU",
        "protocol": "MODBUS_TCP",
        "report_interval_ms": 5000,
        "connection": {
          "server_id":41,
          "port": 5021,
          "ip": "10.105.24.226"
        },
        "location": {
          "site": "MWH01",
          "colo": "COLO1",
          "panel": "AHU01"
        },
        "model": "Schneider Electric-PM-8244",
        "schema": [
          [
            "holding_f01",
            400005,
            "float_be",
            10
          ],
          [
            "holding_f02",
            400007,
            "float_be",
            10
          ]
        ]
      }
    ]
```

For schema definiton, the field sequence is [key, number, type, bit, multiplier, offset], first three are mandatory.

Note, some of the field is protocol specific, like schema, connection

connection field is protocol specific:

For modbus_tcp:
```json
     "connection" : {
         "server_id": 40,
         "port":5020,
         "ip":10.105.24.34"
     }
```

For modbus_rtu:
```json
     "connection" : {
         "server_id": 40,
         "uart_config": "115200:0:8:0:1:0"
     }
```

For pxc36:
```json
     "connection" : {
         "uart_config": "115200:0:8:0:1:0"
     }
```

The schema field is also protocol specific. For pxc36, since all the data points already been predefined in PXC which may have vendor specific name, so the schema is a mapping table between vendor specific data point name and Microsoft standard data point name. Each schema entry is a key mapping
```json
    {
        "key": "RAW_FAN",
        "oem_key": "STD_FAN"
    }
```

For modbus, device may have a points definition table, but the table is not on device. There is no such method to retrieve all the data points from devices. So sphere need to know that which data point is using which register, address and what the data type of this data point, so it can query the certain registers to retrieve the data points. The schema is a modbus point definition table, each entry is a point definition
"key" is the standard data point name to be used in telemetry report. "number" is six digits number and the first digit to identify register type, (rest part -1) identify address. "type" indicates the data type, how to use the register to encode data value. "multiplier" is for device that does not support float, multiple register value with the multiplier to get the real value.

 register number | Data Addresses | Table Name
 ----------------|----------------|-------------
 1 - 65536       |  0000 - FFFF   |     Discrete Output Coils
 10001 - 165536  |  0000 - FFFF   |     Discrete Input Contacts
 30001 - 365536  |  0000 - FFFF   |     Analog Input Registers
 40001 - 465536  |  0000 - FFFF   |     Analog Output Holding Registers

```json
    []
    "holding_f02",
    400007,
    "float_be",
    10
    }
```
For some modbus data point, the entry may include a "bit" field, which means device is trying to use of Nth bit of uint16 register to represent a binary value.

## IoT Device Twin Definition
Device twin will be used to sync state between iot hub and iot device. Current sphere unit will report following in device twin
```
      "bootReason": "Unknown",
      "firmwareVersion": "0.1.0",
      "debug": true,
      "stats": {
        "metricCount": 2,
        "metric0": {
          "tag": "failed_to_send",
          "sample": 102,
          "value": "1.0/1.0/1.0"
        },
        "metric1": {
          "tag": "poll_fail",
          "sample": 3282,
          "value": "1.0/1.0/1.0"
        }
      },
      "provision": {
        "device0": {
          "name": "BN14-EPM-A-PDU-A2-1",
          "model": "PDU-Vertiv",
          "report_ms": 60000,
          "location": {
            "site": "BN14",
            "colo": "R1",
            "panel": "P1"
          }
        },
        "deviceCount": 1
      }
```
bootReason - indicate why application start, could be crash, watchdog, killed, OTA or normal. (To be supported)
firmwareVersion - indicate current firmware version running on the Sphere unit
debug - indicate if remote debug been enabled on this device, reset on application restart
provision - indicate ce devices been provisioned to this sphere unit and the connection status of those devices
stats - some statistic data for those ce device, defined by application, like poll_duration, failed attempt, memory usage etc.