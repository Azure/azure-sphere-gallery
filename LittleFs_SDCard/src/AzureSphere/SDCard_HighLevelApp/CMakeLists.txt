#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(SDCard_HighLevelApp C)

azsphere_configure_tools(TOOLS_REVISION "23.05")
azsphere_configure_api(TARGET_API_SET "16")

add_executable(${PROJECT_NAME} 
	main.c 
	SDCardViaRtCore.c
	eventloop_timer_utilities.c
	utils.c
	../../littlefs/lfs.c 
	../../littlefs/lfs_util.c)

# Enable/Disable runtime debug messages
# add_compile_definitions(SHOW_DEBUG_INFO)

include_directories(../intercore_messages ../../littlefs)

target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${AZURE_SPHERE_SDK_PATH}/HardwareDefinitions" TARGET_DEFINITION "mt3620_rdb.json")

azsphere_target_add_image_package(${PROJECT_NAME})
