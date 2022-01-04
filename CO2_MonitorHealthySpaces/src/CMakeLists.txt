#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)

project("co2monitor" C)

include(azsphere_board.txt)

if (EXISTS "${CMAKE_SOURCE_DIR}/Tools/cmake/azsphere_config.cmake")

    include(Tools/cmake/azsphere_config.cmake)
    auto_generate_azsphere_config()

else()

    # For information on setting tools revision and target api set see
    # https://docs.microsoft.com/en-us/azure-sphere/app-development/using-cmake-functions

    azsphere_configure_tools(TOOLS_REVISION "21.07")
    azsphere_configure_api(TARGET_API_SET "10")

endif()

if (SCD4x)
    set(scd4x
        "AzureSphereDrivers/embedded-i2c-scd4x/scd4x_i2c.c"
        "AzureSphereDrivers/embedded-i2c-scd4x/sensirion_common.c"
        "AzureSphereDrivers/embedded-i2c-scd4x/sensirion_i2c.c"
        "AzureSphereDrivers/embedded-i2c-scd4x/sensirion_i2c_hal.c"
    )
    source_group("scd4x" FILES ${scd4x})
endif(SCD4x)

# Create executable
add_executable (${PROJECT_NAME}
    "main.c"
    "Onboard/azure_status.c"
    "Onboard/onboard_sensors.c"
    "co2_sensor.c"
    ${scd4x}
)

target_link_libraries (${PROJECT_NAME} applibs pthread gcc_s c azure_sphere_devx)
target_include_directories(${PROJECT_NAME} PUBLIC AzureSphereDevX/include)
target_include_directories(${PROJECT_NAME} PUBLIC Onboard)

add_subdirectory("AzureSphereDevX" out)

set_source_files_properties("AzureSphereDrivers/embedded-i2c-scd4x/sensirion_common.c" PROPERTIES COMPILE_FLAGS -Wno-conversion)
set_source_files_properties("AzureSphereDrivers/embedded-i2c-scd4x/sensirion_i2c.c" PROPERTIES COMPILE_FLAGS -Wno-conversion)

if(SCD30)
    add_definitions( -DSCD30=TRUE )
    target_link_libraries (${PROJECT_NAME} scd30_lib)
    add_subdirectory("AzureSphereDrivers/EmbeddedScd30" scd30)
endif(SCD30)

if (SCD4x)
    add_definitions( -DSCD4x=TRUE )
    target_include_directories(${PROJECT_NAME} PUBLIC AzureSphereDrivers/embedded-i2c-scd4x scd4x)
endif(SCD4x)

if(AVNET OR AVNET_REV_2)
    add_subdirectory("AzureSphereDrivers/AVNET/HL" drivers)
    target_link_libraries(${PROJECT_NAME} avnet_drivers)
    target_include_directories(${PROJECT_NAME} PUBLIC AzureSphereDrivers/AVNET/HL AzureSphereDrivers/AVNET/HL/Common)
endif(AVNET OR AVNET_REV_2)

set(BOARD_COUNTER 0)

if(AVNET)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_AVNET=TRUE )
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "co2_monitor.json")
    message(STATUS "Azure Sphere board selected: AVNET REV 1")
endif(AVNET)

if(AVNET_REV_2)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_AVNET=TRUE )
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/avnet_mt3620_sk_rev2" TARGET_DEFINITION "co2_monitor.json")
    message(STATUS "Azure Sphere board selected: AVNET REV 2")
endif(AVNET_REV_2)

if(BOARD_COUNTER EQUAL 0)
    message(FATAL_ERROR "No Azure Sphere boards selected. Ensure one board set")
endif()

if(BOARD_COUNTER GREATER 1)
    message(FATAL_ERROR "Multiple (${BOARD_COUNTER}) Azure Sphere boards selected. Ensure only one board set")
endif()

azsphere_target_add_image_package(${PROJECT_NAME})