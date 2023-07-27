#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

project (DRAMClickboard C)

azsphere_configure_tools(TOOLS_REVISION "23.05")
azsphere_configure_api(TARGET_API_SET "16")

# External Library Add
add_library(DRAM_Click STATIC dram.c)

# Create executable
add_executable (${PROJECT_NAME} main.c)
target_link_libraries (${PROJECT_NAME} DRAM_Click applibs pthread gcc_s c)

# TARGET_HARDWARE and TARGET_DEFINITION relate to the hardware definition targeted by this sample.
# When using this sample with other hardware, replace TARGET_HARDWARE with the name of that hardware.
# For example, to target the Avnet MT3620 Starter Kit, use the value "avnet_mt3620_sk".
# See https://aka.ms/azsphere-samples-hardwaredefinitions for further details on this feature.
set(TARGET_HARDWARE "avnet_mt3620_sk")
set(TARGET_DEFINITION "sample_appliance.json")

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../HardwareDefinitions/${TARGET_HARDWARE}" TARGET_DEFINITION "${TARGET_DEFINITION}")

azsphere_target_add_image_package(${PROJECT_NAME})
