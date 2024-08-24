# ****************************************************************************
# CMakeLists.txt
#
# Description:
# This is the top-level CMake configuration file for the Blinky project.
# It sets up the project and includes necessary CMake modules for building
# the project. The configuration includes:
# 1. Minimum required CMake version.
# 2. Project definition with supported languages (C, C++, ASM).
# 3. Standard settings for C and C++.
# 4. Inclusion of the Pico SDK configuration and initialization.
# 5. Inclusion of the FreeRTOS kernel configuration.
# 6. Addition of subdirectories for project components.
# 7. ...any future top level build system configuration
#
# Usage:
# Place this file at the root of the project directory. Ensure that
# pico_sdk_import.cmake and FreeRTOS_Kernel_import.cmake are located
# in the same directory or adjust the paths accordingly.
#
# ****************************************************************************
cmake_minimum_required(VERSION 3.13)

# Print messages for script tracking and debugging
message("########## TOP CMakeLists.txt - start ##########")

# Specify parameters for pico-sdk
set(PICO_BOARD pico_w)
set(PICO_PLATFORM rp2040)
set(PICO_COMPILER pico_arm_cortex_m0plus_gcc)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)

######################### Pre build imports ###############################

# Include the Pico SDK configuration script
# This script sets up the Raspberry Pi Pico SDK, platform, toolchain, and some CMake utility functions.
message("########## pico_sdk_import.cmake - start ##########")
include(pico_sdk_import.cmake)

# Include the FreeRTOS configuration script
# This script sets up variables and configuration for the FreeRTOS kernel.
message("########## FreeRTOS_Kernel_import.cmake - start ##########")
include(FreeRTOS_Kernel_import.cmake)

############################################################################

# Set project name and supported languages
project(Blinky C CXX ASM)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
# This macro sets up the Pico SDK and prepares it for use.
message("########## pico_sdk_init macro - start ##########")
pico_sdk_init()

# Add a subdirectory for the software components
add_subdirectory(SWC)

message("########## TOP CMakeLists.txt - end ##########")