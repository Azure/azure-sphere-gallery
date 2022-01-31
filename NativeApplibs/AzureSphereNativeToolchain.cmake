set(CMAKE_SYSTEM_NAME Generic)

# Override normal Azure Sphere functions
function(azsphere_configure_tools)
endfunction()

function(azsphere_configure_api)
endfunction()

function(azsphere_target_add_image_package)
endfunction()

function(azsphere_target_hardware_definition target)
    set(options)
    set(oneValueArgs TARGET_DEFINITION)
    set(multiValueArgs TARGET_DIRECTORY)
    cmake_parse_arguments(PARSE_ARGV 1 ATHD "${options}" "${oneValueArgs}" "${multiValueArgs}")
    
    # Ensure all required arguments, and no unexpected arguments, were supplied.
    if(NOT (DEFINED ATHD_TARGET_DIRECTORY))
        # Check if the Target definition is in the SDK path
        set(sdk_hardware_json_path "${AZURE_SPHERE_SDK_PATH}/HardwareDefinitions/")
        if((DEFINED ATHD_TARGET_DEFINITION) AND (NOT (EXISTS "${sdk_hardware_json_path}/${ATHD_TARGET_DEFINITION}")))
            message(FATAL_ERROR "Couldn't find \'${ATHD_TARGET_DEFINITION}\' in the SDK. Provide a TARGET_DIRECTORY or use an existing board from the SDK.")
        endif()
    # TARGET_DEFINITION should always be provided when using hardware definitions
    elseif(NOT (DEFINED ATHD_TARGET_DEFINITION))
        message(FATAL_ERROR "azsphere_target_hardware_definition missing TARGET_DEFINITION argument")
    elseif(DEFINED ATHD_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "azsphere_target_hardware_definition received unexpected argument(s) ${ATHD_UNPARSED_ARGUMENTS}")
    endif()
    
    set(defn_file "${ATHD_TARGET_DEFINITION}")
    set(VAR_TARGET_DEF_FOUND FALSE)
    # Check that the user added paths exists
    if (DEFINED ATHD_TARGET_DIRECTORY)
        foreach (user_path ${ATHD_TARGET_DIRECTORY})
            set(defn_user_include_path "")
            if (IS_ABSOLUTE "${user_path}")
                set(defn_user_include_path "${user_path}")
            else()
                get_filename_component(defn_user_include_path ${user_path} ABSOLUTE BASE_DIR ${CMAKE_SOURCE_DIR})
                set(defn_user_include_path "${defn_user_include_path}")
            endif()
            if (NOT (EXISTS "${defn_user_include_path}"))
                message(FATAL_ERROR "azsphere_target_hardware_definition: TARGET_DIRECTORY ${defn_user_include_path} does not exist.")
            endif()
            if (NOT (IS_DIRECTORY "${defn_user_include_path}"))
                message(FATAL_ERROR "azsphere_target_hardware_definition: TARGET_DIRECTORY ${defn_user_include_path} is not a directory.")
            endif()
            if (NOT VAR_TARGET_DEF_FOUND)
                set(defn_file "${defn_user_include_path}/${ATHD_TARGET_DEFINITION}")
                if ((EXISTS "${defn_file}") AND (NOT(IS_DIRECTORY "${defn_file}")))
                    set(VAR_TARGET_DEF_FOUND TRUE)
                endif()
            endif()
            if(NOT (EXISTS "${defn_user_include_path}/inc"))
                message(FATAL_ERROR "azsphere_target_hardware_definition: TARGET_DIRECTORY ${defn_user_include_path} doesn't include an 'inc' directory. Make sure that you have generated the hardware definitions.")
            endif()
            target_include_directories(${target} SYSTEM PUBLIC "${defn_user_include_path}/inc")
            if(NOT (EXISTS "${defn_user_include_path}/inc/hw"))
                message(FATAL_ERROR "azsphere_target_hardware_definition: TARGET_DIRECTORY ${defn_user_include_path}/inc/ doesn't include a 'hw' directory. Make sure that you have generated the hardware definitions.")
            endif()
            list(APPEND APPLICATION_INCLUDE_PATHS_LIST "${defn_user_include_path}")
        endforeach()
    endif()
    
    # By default we add the SDK path as last in our list
    list(APPEND APPLICATION_INCLUDE_PATHS_LIST "${AZURE_SPHERE_SDK_PATH}/HardwareDefinitions/")
    target_include_directories(${target} SYSTEM PUBLIC "${AZURE_SPHERE_SDK_PATH}/HardwareDefinitions/inc")
    target_include_directories(${target} SYSTEM PUBLIC "${AZURE_SPHERE_SDK_PATH}/HardwareDefinitions/inc/hw")

    if (NOT VAR_TARGET_DEF_FOUND)
        set(defn_file "${AZURE_SPHERE_SDK_PATH}/HardwareDefinitions/${ATHD_TARGET_DEFINITION}")
    endif()
    
    # Ensure the hardware definition file exists and really is a file.
    # CMake doesn't have an IS_FILE operator, so approximate by failing if it is a directory.
    if (NOT (EXISTS "${defn_file}"))
        message(FATAL_ERROR "azsphere_target_hardware_definition: TARGET_DEFINITION file ${defn_file} does not exist.")
    endif()
    if (IS_DIRECTORY "${defn_file}")
        message(FATAL_ERROR "azsphere_target_hardware_definition: TARGET_DEFINITION file ${defn_file} is a directory.")
    endif()
    string(REGEX MATCH "\.[jJ][sS][oO][nN]$" is_json "${defn_file}")
    if ("${is_json}" STREQUAL "")
        message(FATAL_ERROR "azsphere_target_hardware_definition: TARGET_DEFINITION file ${defn_file} is not a json file.")
    endif()
    string (REPLACE ";" "," APPLICATION_INCLUDE_PATHS_STR "${APPLICATION_INCLUDE_PATHS_LIST}")
    get_filename_component(root_filename ${defn_file} NAME)
    # Supply JSON file to azsphere image-package pack-application.
    # This property is used in _azsphere_target_add_image_package_common.
    set_target_properties(${target} PROPERTIES AS_PKG_HWD_ARGS "-p;${APPLICATION_INCLUDE_PATHS_STR};-f;${root_filename}")
endfunction()

if (DEFINED AZURE_SPHERE_SDK_PATH)
    set(ENV{AzureSphereSdkPath} ${AZURE_SPHERE_SDK_PATH})
endif()
set(AZURE_SPHERE_SDK_PATH $ENV{AzureSphereSdkPath})
if (NOT (DEFINED AZURE_SPHERE_SDK_PATH))
    set(AZURE_SPHERE_SDK_PATH "/opt/azurespheresdk")
endif()

if (DEFINED NATIVEAPPLIBS_PATH)
    set(ENV{AzureSphereApplibsPath} ${NATIVEAPPLIBS_PATH})
endif()
set(NATIVEAPPLIBS_PATH $ENV{AzureSphereApplibsPath})
if (NOT (DEFINED NATIVEAPPLIBS_PATH))
	set(NATIVEAPPLIBS_PATH "/opt/azurespheresdk/NativeLibs")
endif()

set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".so")
find_library(LIBRARY_APPLIBS "applibs" PATHS "${NATIVEAPPLIBS_PATH}" PATH_SUFFIXES "usr/lib")
if (NOT LIBRARY_APPLIBS)
    message(FATAL_ERROR "libapplibs not found in ${NATIVEAPPLIBS_PATH}")
endif()

string(FIND ${LIBRARY_APPLIBS} "/" NATIVEAPPLIBS_PATH_END REVERSE)
string(SUBSTRING ${LIBRARY_APPLIBS} 0 ${NATIVEAPPLIBS_PATH_END} FOUND_NATIVEAPPLIBS_PATH)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-L ${FOUND_NATIVEAPPLIBS_PATH} -Wl,-rpath,${FOUND_NATIVEAPPLIBS_PATH}")

string(FIND ${FOUND_NATIVEAPPLIBS_PATH} "/" FOUND_NATIVEAPPLIBS_PATH_END REVERSE)
string(SUBSTRING ${FOUND_NATIVEAPPLIBS_PATH} 0 ${FOUND_NATIVEAPPLIBS_PATH_END} FOUND_NATIVEAPPLIBS_BASE_PATH)
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "${FOUND_NATIVEAPPLIBS_BASE_PATH}/include")
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "${FOUND_NATIVEAPPLIBS_BASE_PATH}/include")

# Disable linking during try_compile since our link options cause the generation to fail
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY CACHE INTERNAL "Disable linking for try_compile")

# Get hardware definition directory
if(DEFINED AZURE_SPHERE_TARGET_HARDWARE_DEFINITION_DIRECTORY)
    if (IS_ABSOLUTE "${AZURE_SPHERE_TARGET_HARDWARE_DEFINITION_DIRECTORY}")
        set(ENV{AzureSphereTargetHardwareDefinitionDirectory} ${AZURE_SPHERE_TARGET_HARDWARE_DEFINITION_DIRECTORY})
    else()
        get_filename_component(AZURE_SPHERE_TARGET_HARDWARE_DEFINITION_DIRECTORY_ABS ${AZURE_SPHERE_TARGET_HARDWARE_DEFINITION_DIRECTORY} ABSOLUTE BASE_DIR ${CMAKE_SOURCE_DIR})
        set(ENV{AzureSphereTargetHardwareDefinitionDirectory} ${AZURE_SPHERE_TARGET_HARDWARE_DEFINITION_DIRECTORY_ABS})
    endif()
endif()
set(AZURE_SPHERE_HW_DIRECTORY $ENV{AzureSphereTargetHardwareDefinitionDirectory})

# Append the hardware definition directory
if((NOT ("${AZURE_SPHERE_HW_DEFINITION}" STREQUAL "")) AND (IS_DIRECTORY "${AZURE_SPHERE_HW_DIRECTORY}/inc"))
    list(APPEND CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "${AZURE_SPHERE_HW_DIRECTORY}/inc")
    list(APPEND CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "${AZURE_SPHERE_HW_DIRECTORY}/inc")
endif()

set(COMPILE_DEBUG_FLAGS $<$<CONFIG:Debug>:-ggdb> $<$<CONFIG:Debug>:-O0>)
set(COMPILE_RELEASE_FLAGS $<$<CONFIG:Release>:-g1> $<$<CONFIG:Release>:-Os>)
set(COMPILE_C_FLAGS $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes> $<$<COMPILE_LANGUAGE:C>:-Wno-pointer-sign>)
add_compile_options(${COMPILE_C_FLAGS} ${COMPILE_DEBUG_FLAGS} ${COMPILE_RELEASE_FLAGS} -fPIC
                    -ffunction-sections -fdata-sections -fno-strict-aliasing
                    -fno-omit-frame-pointer -fno-exceptions -Wall
                    -Wswitch -Wempty-body -Wconversion -Wreturn-type -Wparentheses
                    -Wno-format -Wuninitialized -Wunreachable-code
                    -Wunused-function -Wunused-value -Wunused-variable
                    -Werror=implicit-function-declaration -fstack-protector-strong)
