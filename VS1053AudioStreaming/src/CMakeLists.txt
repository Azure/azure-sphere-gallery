#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

# SELECT DEVELOPER BOARD ##########################################################################################
#
set(SEEED_STUDIO_RDB TRUE "Seeed Studio Azure Sphere MT3620 Development Kit (aka Reference Design Board or rdb)")
#
###################################################################################################################

cmake_minimum_required(VERSION 3.10)

project(VS1053AudioStream_HighLevelApp C)

azsphere_configure_tools(TOOLS_REVISION "23.05")
azsphere_configure_api(TARGET_API_SET "16")

# ENABLE NPR INTERNET RADIO STREAM ##########################################################################################
#
# add_compile_definitions(ENABLE_RADIO_STREAMING)
#
###################################################################################################################

include_directories(${CMAKE_SOURCE_DIR} VS1053)

add_executable(${PROJECT_NAME} 
	main.c 
	VS1053/vs1053.c
)

target_link_libraries(${PROJECT_NAME} applibs gcc_s c)

if (SEEED_STUDIO_RDB) 
	message(verbose " NOTE: Build is configured for Seeed and Adafruit VS1053")
	azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${CMAKE_SOURCE_DIR}/HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "audioplayback_hw.json")
endif(SEEED_STUDIO_RDB) 

azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES speech.mp3)
