#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(BalancingRobot_HighLevel C)

azsphere_configure_tools(TOOLS_REVISION "23.05")
azsphere_configure_api(TARGET_API_SET "16")

# to enable UdpLog, uncomment the next line.
# add_compile_definitions(USE_SOCKET_LOG)

add_executable (${PROJECT_NAME} 
	main.c 
	parson.c 
	eventloop_timer_utilities.c
	i2c_oled.c
	utils.c
	MutableStorageKVP/cJSON.c
	intercore.c
	MutableStorageKVP/MutableStorageKVP.c
	UdpDebugLog/udplog.c
	GetDeviceHash.c)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC ${AZURE_SPHERE_API_SET_DIR}/usr/include/azureiot)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_HOME_DIRECTORY}/MutableStorageKVP)

TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
target_link_libraries (${PROJECT_NAME} m azureiot applibs pthread gcc_s c curl m)
include_directories(../inc)

azsphere_target_add_image_package(${PROJECT_NAME})
