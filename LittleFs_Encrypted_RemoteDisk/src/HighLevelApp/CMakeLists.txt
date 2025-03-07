#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.20)

project(LittleFs_HighLevelApp C)

add_executable(${PROJECT_NAME} 
	main.c 
	crypt.c
	curlFunctions.c
	encrypted_storage.c
	remoteDiskIO.c
	littlefs/lfs.c 
	littlefs/lfs_util.c)

target_include_directories(${PROJECT_NAME} PRIVATE wolfssl)
target_link_libraries(${PROJECT_NAME} applibs gcc_s c curl wolfssl)

# Enable/Disable Curl memory tracking
# add_compile_definitions(ENABLE_CURL_MEMORY_TRACE)

# Enabled debug logging for encrypted storage
# add_compile_definitions(ENCRYPTED_STORAGE_DEBUG)

# // TODO: define the PC HOST IP Address
add_compile_definitions(PC_HOST_IP="")

# Target hardware for the sample.
set(TARGET_HARDWARE "mt3620_rdb")
set(TARGET_DEFINITION "sample_appliance.json")

# Referencing the HardwareDefinitions directly from the SDK, so to not carry them over
azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${AZURE_SPHERE_SDK_PATH}/HardwareDefinitions" TARGET_DEFINITION "mt3620.json")

azsphere_target_add_image_package(${PROJECT_NAME})
