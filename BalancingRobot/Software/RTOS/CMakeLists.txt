#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.11)
#set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
project (BalancingRobot_RealTime C ASM)

azsphere_configure_tools(TOOLS_REVISION "23.05")

SET(CMAKE_ASM_FLAGS "-mcpu=cortex-m4")

add_compile_definitions(OSAI_AZURE_RTOS)

add_link_options(-specs=nano.specs -specs=nosys.specs)

configure_file( ${CMAKE_HOME_DIRECTORY}/mt3620_m4_software/MT3620_M4_Sample_Code/BareMetal/MT3620_RTApp_BareMetal_HelloWorld/linker.ld
                ${CMAKE_HOME_DIRECTORY}/linker.ld
                COPYONLY)

# inter-core messages
include_directories("../inc")

# Create executable
add_executable (${PROJECT_NAME}
    rtos_app/FanOut.c
    rtos_app/i2c.c
    rtos_app/mt3620-intercore.c
    rtos_app/mt3620-uart-poll.c
    rtos_app/PID.c
    rtos_app/PID_v1.c
    rtos_app/rtcoremain.c
    rtos_app/rtos_app.c
    rtos_app/utils.c
    rtos_app/VL53L1X.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_adc.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_dma.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_eint.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_gpio.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_gpioif.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_gpt.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_i2c.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_i2s.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_pwm.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_spim.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_uart.c
    mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/src/os_hal_wdt.c
    ./rtos_app/rtos_app.c 
    ./rtos_app/rtcoremain.c
    ./rtos_app/tx_initialize_low_level.S)

target_include_directories(${PROJECT_NAME} PUBLIC mt3620_m4_software/MT3620_M4_Sample_Code/OS_HAL/inc)
target_include_directories(${PROJECT_NAME} PUBLIC
                            mt3620_m4_software/MT3620_M4_Driver/MHAL/inc
                            mt3620_m4_software/MT3620_M4_Driver/HDL/inc
                            mt3620_m4_software/MT3620_M4_BSP/CMSIS/include
                            mt3620_m4_software/MT3620_M4_BSP/mt3620/inc
                            mt3620_m4_software/MT3620_M4_BSP/printf)

add_subdirectory(mt3620_m4_software/MT3620_M4_Driver)

set(THREADX_ARCH "cortex_m4")
set(THREADX_TOOLCHAIN "gnu")
add_subdirectory(threadx)

target_link_libraries(${PROJECT_NAME} MT3620_M4_Driver azrtos::threadx m)


set_target_properties (${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_HOME_DIRECTORY}/linker.ld)

azsphere_target_add_image_package(${PROJECT_NAME})

