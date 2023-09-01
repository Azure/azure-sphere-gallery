#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(LittleFs_HighLevelApp C)

azsphere_configure_tools(TOOLS_REVISION "23.05")
azsphere_configure_api(TARGET_API_SET "16")

add_executable(${PROJECT_NAME} 
	main.c 
	curlFunctions.c
	remoteDiskIO.c
	SimpleFs/sfs.c)

target_link_libraries(${PROJECT_NAME} applibs gcc_s c curl)
include_directories(SimpleFs)

# Enable/Disable Curl memory tracking
# add_compile_definitions(ENABLE_CURL_MEMORY_TRACE)

# // TODO: define the PC HOST IP Address
add_compile_definitions(PC_HOST_IP="192.168.0.107")

# Target hardware for the sample.
set(TARGET_HARDWARE "mt3620_rdb")
set(TARGET_DEFINITION "sample_appliance.json")

# Referencing the HardwareDefinitions directly from the SDK, so to not carry them over
azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${AZURE_SPHERE_SDK_PATH}/HardwareDefinitions" TARGET_DEFINITION "mt3620.json")

azsphere_target_add_image_package(${PROJECT_NAME})
