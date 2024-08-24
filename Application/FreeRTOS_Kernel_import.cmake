# ------------------------------------------------------------------------------
# This CMake script helps locate and include the FreeRTOS kernel for the RP2040 
# in an external project. It checks if the FREERTOS_KERNEL_PATH environment variable 
# is set or provided as a CMake argument, verifies the kernel's existence, and 
# then includes the necessary FreeRTOS port directory.
#
# Original source: <FREERTOS_KERNEL_PATH>/portable/ThirdParty/GCC/RP2040/FREERTOS_KERNEL_import.cmake
# Git repository: <https://github.com/FreeRTOS/FreeRTOS-Kernel>
# ------------------------------------------------------------------------------

if (NOT DEFINED ENV{FREERTOS_KERNEL_PATH})
    message(FATAL_ERROR "Environment variable FREERTOS_KERNEL_PATH is not defined - please define it with a path to the FreeRTOS kernel directory")
endif ()

# Set the FREERTOS_KERNEL_PATH variable if it's not already set, using the environment variable if available
if (NOT DEFINED FREERTOS_KERNEL_PATH)
    set(FREERTOS_KERNEL_PATH $ENV{FREERTOS_KERNEL_PATH})
    message(STATUS "Using FREERTOS_KERNEL_PATH from environment ('${FREERTOS_KERNEL_PATH}')")
endif ()

# Cache the FREERTOS_KERNEL_PATH variable to allow reuse between CMake runs
set(FREERTOS_KERNEL_PATH "${FREERTOS_KERNEL_PATH}" CACHE PATH "Path to the FreeRTOS Kernel" FORCE)

# Convert FREERTOS_KERNEL_PATH to an absolute path
get_filename_component(FREERTOS_KERNEL_PATH "${FREERTOS_KERNEL_PATH}" REALPATH)

# Define the path to the FreeRTOS port directory for RP2040
set(FREERTOS_KERNEL_RP2040_PATH "${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040")

# Stop if the FreeRTOS port directory does not exist
if (NOT EXISTS "${FREERTOS_KERNEL_RP2040_PATH}")
    message(FATAL_ERROR "Directory '${FREERTOS_KERNEL_RP2040_PATH}' not found. Make sure the FREERTOS_KERNEL_PATH points to the FreeRTOS kernel directory")
endif ()

# Include the FreeRTOS port directory for RP2040
message(STATUS "########## FreeRTOS RP2040 CMakeLists.txt - start ##########")
add_subdirectory(${FREERTOS_KERNEL_RP2040_PATH} FREERTOS_KERNEL)
message(STATUS "########## FreeRTOS RP2040 CMakeLists.txt - end ##########")
