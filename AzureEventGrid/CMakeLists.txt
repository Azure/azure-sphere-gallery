#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)
project (AzureEventGrid_HighLevelApp C)

#
add_compile_definitions(MQTT_USE_WOLFSSL)
#
###################################################################################################################

add_library(mqttc STATIC
    MQTT-C/src/mqtt_pal.c
    MQTT-C/src/mqtt.c
    )
include_directories(${CMAKE_SOURCE_DIR} MQTT-C/include)

# Create executable
add_executable (${PROJECT_NAME} main.c mqtt_connection.c eventloop_timer_utilities.c options.c)
target_link_libraries (${PROJECT_NAME} applibs pthread gcc_s c wolfssl tlsutils mqttc)

set_source_files_properties(MQTT-C/src/mqtt.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(MQTT-C/include/mqtt.h PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
set_source_files_properties(MQTT-C/src/mqtt_pal.c PROPERTIES COMPILE_FLAGS -Wno-conversion)

# Target hardware for the sample.
set(TARGET_HARDWARE "mt3620_rdb")
set(TARGET_DEFINITION "sample_appliance.json")

# Referencing the HardwareDefinitions directly from the SDK, so to not carry them over
azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${AZURE_SPHERE_SDK_PATH}/HardwareDefinitions" TARGET_DEFINITION "mt3620.json")

find_program(POWERSHELL powershell.exe)

if (POWERSHELL)
    # Run validate_manifest script during build
    add_custom_target(ValidateManifest ALL
        COMMAND ${POWERSHELL} -ExecutionPolicy Bypass -NoProfile -NonInteractive
        -File ${CMAKE_SOURCE_DIR}/script/validate_manifest.ps1
        -Manifest ${CMAKE_SOURCE_DIR}/app_manifest.json 
        DEPENDS ${CMAKE_SOURCE_DIR}/app_manifest.json)
else()
    # Warn users without PowerShell to update their manifest
    add_custom_target(ValidateManifest ALL
        COMMAND echo "Please ensure that you have updated app_manifest.json as described in README.md."
        DEPENDS ${CMAKE_SOURCE_DIR}/app_manifest.json)
endif()

add_dependencies(ValidateManifest ${PROJECT_NAME})

azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES "Certs/DigiCertGlobalRootG2CA.pem")