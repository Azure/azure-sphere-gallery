#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(IntercoreComms_RTApp_MT3620_BareMetal C)

azsphere_configure_tools(TOOLS_REVISION "21.07")

# Enable/Disable runtime debug messages
# add_compile_definitions(SHOW_DEBUG_INFO)

include_directories(../intercore_messages)

add_executable(${PROJECT_NAME} main.c SD.c Socket.c lib/VectorTable.c lib/SPIMaster.c lib/UART.c lib/Print.c lib/GPT.c lib/MBox.c)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

azsphere_target_add_image_package(${PROJECT_NAME})

