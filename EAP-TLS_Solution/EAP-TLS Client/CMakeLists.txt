#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)
project(EAP_TLS_Client C)

azsphere_configure_tools(TOOLS_REVISION "23.05")
azsphere_configure_api(TARGET_API_SET "16")


# Create executable
add_executable(${PROJECT_NAME} main.c eventloop_timer_utilities.c parson.c "lib/_environment_config.c" "lib/eap_tls_lib.c" "lib/web_api_client.c" "tests/tests.c")
target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c curl tlsutils)

# Add MakeImage post-build command
azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${AZURE_SPHERE_SDK_PATH}/HardwareDefinitions" TARGET_DEFINITION "mt3620.json")


# RADIUS network root & client certificates (optionally provisioned)
#***************************************************************************************
# NOTE: the below are "dummy" certificates, private passwords and identities
# all generated with a dev/test CA, and SHOULD NOT BE USED IN PRODUCTION ENVIRONMENTS!!
#***************************************************************************************
azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES  "certs/eap-tls-webapi.pem" "certs/az-CA.pem" "certs/iotuser_public.pem" "certs/iotuser_private.pem" "certs/extuser_public.pem" "certs/extuser_private.pem")