#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

project (HLCore CXX)

azsphere_configure_tools(TOOLS_REVISION "23.05")
azsphere_configure_api(TARGET_API_SET "16")
set(DISABLE_DYNAMIC true)
set(SQ_DISABLE_INSTALLER true)
set(SQ_DISABLE_HEADER_INSTALLER true)
set(AZURE_SPHERE_HL true)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DAZURE_SPHERE_HL -DE_SQUIRREL")
add_subdirectory("squirrel")

# Create executable
add_executable (${PROJECT_NAME} "main.cpp" "squirrel_cpp_helper.cpp" "logs.cpp" "curl_logs.cpp" "eventloop_timer_utilities.cpp" "event_queue.cpp" "json.cpp" "pretty_print.cpp" "http.cpp" "http_request.cpp" "gpio.cpp")
target_link_libraries (${PROJECT_NAME} applibs pthread gcc_s c squirrel_static sqstdlib_static curl tlsutils wolfssl)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${AZURE_SPHERE_SDK_PATH}/HardwareDefinitions" TARGET_DEFINITION "avnet_mt3620_sk_rev2.json")
azsphere_target_add_image_package(${PROJECT_NAME} DEBUG_LIB "libmalloc" RESOURCE_FILES "certs/CA.cer" "main.nut" "test.nut")