#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)

project (RecipePusher C)

azsphere_configure_tools(TOOLS_REVISION "21.07")
azsphere_configure_api(TARGET_API_SET "10")

# Create executable
add_executable (${PROJECT_NAME} src/main.c)

add_subdirectory(src)
add_subdirectory(src/common)

target_include_directories(${PROJECT_NAME} PUBLIC
                           ${CMAKE_SOURCE_DIR}/common
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azureiot
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azure_prov_client
                           ${AZURE_SPHERE_API_SET_DIR}/usr/include/azure_c_shared_utility)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror)
target_compile_definitions(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
target_link_libraries(${PROJECT_NAME} m azureiot applibs gcc_s c curl)

set(TARGET_HARDWARE "mt3620")
set(TARGET_DEFINITION "mt3620.json")

find_path(
    TARGET_DIRECTORY
    NAMES ${TARGET_DEFINITION}
    PATHS "HardwareDefinitions/${TARGET_HARDWARE}" "HardwareDefinitions/${TARGET_HARDWARE}"
    NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "${TARGET_DIRECTORY}" TARGET_DEFINITION "${TARGET_DEFINITION}")

find_program(POWERSHELL powershell.exe)

if (POWERSHELL)
    # Run validate_manifest script during build
    add_custom_target(ValidateManifest ALL
        COMMAND ${POWERSHELL} -ExecutionPolicy Bypass -NoProfile -NonInteractive
        -File ${CMAKE_SOURCE_DIR}/script/validate_manifest.ps1
        -Manifest ${CMAKE_SOURCE_DIR}/app_manifest.json 
        -Helper ${MANIFEST_HELPER}
        DEPENDS ${CMAKE_SOURCE_DIR}/app_manifest.json)
else()
    # Warn users without PowerShell to update their manifest
    add_custom_target(ValidateManifest ALL
        COMMAND echo "Please ensure that you have updated app_manifest.json as appropriate."
        DEPENDS ${CMAKE_SOURCE_DIR}/app_manifest.json)
endif()

add_dependencies(ValidateManifest ${PROJECT_NAME})

# azsphere_target_add_image_package(${PROJECT_NAME})
azsphere_target_add_image_package(${PROJECT_NAME} RESOURCE_FILES "certs/RootCert.pem")
