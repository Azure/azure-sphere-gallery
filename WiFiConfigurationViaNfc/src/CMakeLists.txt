#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(WiFiProvisioningViaNfc_HighLevelApp C)

azsphere_configure_tools(TOOLS_REVISION "23.05")
azsphere_configure_api(TARGET_API_SET "16")

include_directories(${CMAKE_SOURCE_DIR} M24SR)

add_executable(${PROJECT_NAME} 
	main.c 
	utils.c
	eventloop_timer_utilities.c
	M24SR/M24SR.c
)

# uncomment the following line to get verbose debug output
# add_compile_definitions(ENABLE_VERBOSE_DEBUG_OUTPUT)

target_link_libraries(${PROJECT_NAME} applibs gcc_s c)

# Referencing the HardwareDefinitions directly from the SDK, so to not carry them over
azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${AZURE_SPHERE_SDK_PATH}/HardwareDefinitions" TARGET_DEFINITION "avnet_mt3620_sk.json")

azsphere_target_add_image_package(${PROJECT_NAME})
