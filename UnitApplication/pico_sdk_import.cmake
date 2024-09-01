# ------------------------------------------------------------------------------
# This CMake script helps locate and include the Raspberry Pi Pico SDK for use 
# in an external project. It checks if the PICO_SDK_PATH environment variable 
# is set or provided as a CMake argument, verifies the SDK's existence, and 
# then includes the necessary SDK initialization file. 
#
# Original source: <PICO_SDK_PATH>/external/pico_sdk_import.cmake
# Git repository: https://github.com/raspberrypi/pico-sdk
# ------------------------------------------------------------------------------

if (NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "Environmental variable PICO_SDK_PATH is not defined - please define it with a path to the pico-sdk directory")
endif ()
 
# Set the PICO_SDK_PATH variable if it's not already set, using the environment variable if available
if (NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    message("Using PICO_SDK_PATH from environment ('${PICO_SDK_PATH}')")
endif ()

# Cache the PICO_SDK_PATH variable to allow reuse between CMake runs
set(PICO_SDK_PATH "${PICO_SDK_PATH}" CACHE PATH "Path to the Raspberry Pi Pico SDK" FORCE)

# Convert PICO_SDK_PATH to an absolute path
get_filename_component(PICO_SDK_PATH "${PICO_SDK_PATH}" REALPATH)

# Set the path to the pico_sdk_init.cmake file in the SDK directory
set(PICO_SDK_INIT_CMAKE_FILE ${PICO_SDK_PATH}/pico_sdk_init.cmake)

# Stop if the pico_sdk_init.cmake file is not found in the SDK directory
if (NOT EXISTS ${PICO_SDK_INIT_CMAKE_FILE})
    message(FATAL_ERROR "'${PICO_SDK_PATH}' does not contain pico_sdk_init.cmake file. Make sure the PICO_SDK_PATH points to the pico-sdk directory")
endif ()

# Include (load and execute) the SDK initialization file
message("########## pico_sdk_init.cmake - start ##########")
include(${PICO_SDK_INIT_CMAKE_FILE})
message("########## pico_sdk_init.cmake - end ##########")




