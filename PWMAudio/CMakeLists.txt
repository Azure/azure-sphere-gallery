#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)

project (PWMAudioRT C)

azsphere_configure_tools(TOOLS_REVISION "23.05")

# Create executable
add_executable(${PROJECT_NAME} main.c lib/VectorTable.c lib/GPT.c lib/GPIO.c)
target_link_libraries (${PROJECT_NAME})
set_target_properties (${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

azsphere_target_add_image_package(${PROJECT_NAME})