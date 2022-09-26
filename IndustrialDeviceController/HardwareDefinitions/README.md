# Hardware definitions

Azure Sphere hardware is available from multiple vendors, and each vendor may expose features of the underlying chip in different ways. However, through hardware abstraction, the sample applications in the Azure Sphere samples repository on GitHub are hardware independent. They will run on any Azure Sphere-compatible hardware without changes to the code.

Hardware abstraction is achieved through the use of hardware definition files. The samples are written to run on an abstract "sample appliance". For each supported board or module there is a hardware definition file called sample_appliance.json.
This file maps the "peripherals" used by the sample appliance to the corresponding peripherals on the board or module. The contents of this file are used during the build procedure to update the app_manifest.json file and in compiling and packaging the sample.

To run any of the samples in the [Azure Sphere samples repository](https://github.com/Azure/azure-sphere-samples) on supported Azure Sphere hardware, you specify the directory that contains the sample_appliance.json file for the target hardware.

This README file describes how to [set the target hardware for a sample](#set-the-target-hardware-for-a-sample-application). For additional information, see [Manage hardware dependencies](https://learn.microsoft.com/azure-sphere/app-development/manage-hardware-dependencies) and [Hardware abstraction files](https://review.learn.microsoft.com/azure-sphere/hardware/hardware-abstraction) in the online documentation.

## Hardware

The samples support the following Azure Sphere hardware:

**Chips**

- [MediaTek MT3620](mt3620/)

**Modules**

- [AI-Link WF-M620-RSC1](ailink_wfm620rsc1/)
- [Avnet AES-MS-MT3620](avnet_aesms/)
- [USI USI-MT3620-BT-COMBO](usi_mt3620_bt_combo/)

**Development Kits**

- [MT3620 RDB](mt3620_rdb/)
- [Seeed MT3620 mini-dev board](seeed_mt3620_mdb/), which uses the AI-Link WF-M620-RSC1 module
- [Avnet MT3620 SK](avnet_mt3620_sk/), which uses the Avnet AES-MS-MT3620 module
- [USI MT3620 BT EVB](usi_mt3620_bt_evb/), which uses the USI USI-MT3620-BT-COMBO module

Each folder contains a JSON file and a header file that maps the board- or module-specific features to the underlying Azure Sphere chip.

The identifiers that the samples use for Azure Sphere features are abstracted into a file named sample_hardware.json, which maps them to peripherals in the board- and module-specific files.

## Set the target hardware for a sample application

To set or change the target hardware for a sample application, use CMake function `azsphere_target_hardware_definition` to specify the directory that contains the hardware definition file for your target hardware.

1. Open the CMakeLists.txt file, which is located in the sample source directory, and search for `azsphere_target_hardware_definition`.

1. Change the value of the parameter `TARGET_DIRECTORY` to the target hardware definition directory. For example, to run the sample on the AI-Link WF-M620-RSC1:

   change

   `azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "board_config.json")`

   to

   `azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/ailink_wfm620rsc1" TARGET_DEFINITION "board_config.json")`

