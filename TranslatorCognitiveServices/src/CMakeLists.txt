#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(TranslatorCognitiveServices_HighLevelApp C)

add_executable(${PROJECT_NAME} 
	main.c 
	parson.c
	translator.c
)

target_link_libraries(${PROJECT_NAME} applibs gcc_s c curl)

# Referencing the HardwareDefinitions directly from the SDK, so to not carry them over
azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${AZURE_SPHERE_SDK_PATH}/HardwareDefinitions" TARGET_DEFINITION "mt3620_rdb.json")

azsphere_target_add_image_package(${PROJECT_NAME})
