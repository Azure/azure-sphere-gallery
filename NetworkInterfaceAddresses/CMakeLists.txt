#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(PrintNetworkInterfaceAddresses_HighLevelApp C)

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} applibs gcc_s c)

azsphere_target_add_image_package(${PROJECT_NAME})
