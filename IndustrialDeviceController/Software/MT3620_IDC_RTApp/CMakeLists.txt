#  Copyright (c) Codethink Ltd. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.11)
project(MT3620_IDC_RTApp C)
azsphere_configure_tools(TOOLS_REVISION "20.10")

# Create executable
add_executable(${PROJECT_NAME} main.c Socket.c lib/VectorTable.c lib/GPIO.c lib/UART.c lib/Print.c lib/GPT.c lib/MBox.c)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

# Add MakeImage post-build command
azsphere_target_add_image_package(${PROJECT_NAME})