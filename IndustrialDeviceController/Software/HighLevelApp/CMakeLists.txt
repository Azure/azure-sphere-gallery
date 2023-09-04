# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.8)

project(${PRODUCT} C)

message(STATUS ${PRODUCT})

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(POSTFIX "b")
endif()

# Update firmware version to include HW type and env
if(EXISTS "${CMAKE_SOURCE_DIR}/app_manifest.json")
    message(STATUS "update manifest.json")
    file(READ "${CMAKE_SOURCE_DIR}/app_manifest.json" AZURE_SPHERE_APP_MANIFEST_CONTENTS)
    string(REGEX REPLACE "(mt3620-idc)-([0-9\.]*)b?" "${PRODUCT}-\\2${POSTFIX}" AZURE_SPHERE_APP_MANIFEST_CONTENTS "${AZURE_SPHERE_APP_MANIFEST_CONTENTS}")
    file(WRITE "${CMAKE_SOURCE_DIR}/app_manifest.json" ${AZURE_SPHERE_APP_MANIFEST_CONTENTS})
endif()

include_directories(include)
include_directories(external)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -DLOG_LEVEL=6 -DLEAK_DETAIL")

aux_source_directory(init INIT_SRCS)
aux_source_directory(iot IOT_SRCS)
aux_source_directory(drivers DRIVERS_SRCS)
aux_source_directory(drivers/modbus MODBUS_DRIVERS_SRCS)
aux_source_directory(libutils LIBUTILS_SRCS)

azsphere_configure_tools(TOOLS_REVISION "23.05")

azsphere_configure_api(TARGET_API_SET "16")

option(ENABLE_SERIAL_LOG "Enable Serial Log" OFF)
if(ENABLE_SERIAL_LOG)
  message(STATUS "Enable Serial Log")
  add_definitions(-DENABLE_SERIAL_LOG=1)
endif()

# Create executable
add_executable(${PROJECT_NAME} ${INIT_SRCS} ${IOT_SRCS} ${DRIVERS_SRCS} ${MODBUS_DRIVERS_SRCS} ${LIBUTILS_SRCS})

target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c azureiot frozen safeclib)

target_compile_options(${PROJECT_NAME} PRIVATE -Wno-sign-conversion -Wno-conversion -Wall -Werror)

if (${PRODUCT} STREQUAL "mt3620-idc")
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "../../HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "board_config.json")
endif()

azsphere_target_add_image_package(${PROJECT_NAME})

add_subdirectory(external/safeclib)
add_subdirectory(external/frozen)