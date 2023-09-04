#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)
project (MQTT_HighLevelApp C)

# MQTT BROKER CONFIG ##############################################################################################
#
add_compile_definitions(ALTAIR_MQTT_HOST="test.mosquitto.org")
add_compile_definitions(ALTAIR_MQTT_SECURE_PORT="8884")
add_compile_definitions(MQTT_CA_CERTIFICATE="Certs/mosquitto.org.crt")
add_compile_definitions(MQTT_CLIENT_PRIVATE_KEY="Certs/client.key")
add_compile_definitions(MQTT_CLIENT_CERTIFICATE="Certs/client.crt")
#
add_compile_definitions(MQTT_USE_WOLFSSL)
#
###################################################################################################################

azsphere_configure_tools(TOOLS_REVISION "23.05")
azsphere_configure_api(TARGET_API_SET "16")

add_subdirectory("AzureSphereDevX" out)

add_library(mqttc STATIC
    MQTT-C/src/mqtt_pal.c
    MQTT-C/src/mqtt.c
)
include_directories(${CMAKE_SOURCE_DIR} MQTT-C/include)

# Create executable
add_executable (${PROJECT_NAME} main.c comms_manager.c)
target_link_libraries (${PROJECT_NAME} applibs pthread gcc_s c wolfssl tlsutils azure_sphere_devx mqttc)
target_include_directories(${PROJECT_NAME} PUBLIC AzureSphereDevX/include )

set_source_files_properties(MQTT-C/src/mqtt.c PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties(MQTT-C/include/mqtt.h PROPERTIES COMPILE_FLAGS -Wno-sign-conversion)
set_source_files_properties(MQTT-C/src/mqtt_pal.c PROPERTIES COMPILE_FLAGS -Wno-conversion)


azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES "Certs/mosquitto.org.crt" "Certs/client.crt" "Certs/client.key")