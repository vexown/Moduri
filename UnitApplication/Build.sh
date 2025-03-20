#!/bin/bash

# ****************************************************************************
# Build.sh
#
# Description:
# This script automates the process of building a project using CMake.
# It performs the following steps:
# 1. Checks for and installs required system packages if missing
# 2. Ensures project dependencies (FreeRTOS, pico-sdk, picotool) are initialized as git submodules
# 3. Checks if the 'build' directory exists and creates it if not.
# 4. Configures the build directory by running CMake.
# 5. Builds the project using CMake.
# 6. Copies output files to the output directory.
# 7. Prompts the user to press any key to exit the script.
#
# Usage:
# Run this script from the root directory of the project:
#   ./Build.sh                          # Build the default application
#   ./Build.sh --clean                  # Clean and rebuild
#   ./Build.sh --example=list           # List available examples
#   ./Build.sh --example=ExampleName    # Build a specific example
#   ./Build.sh --clean --example=ExampleName  # Clean and build an example
#
# ****************************************************************************

# Exit immediately if any command fails
set -e

# If a command in a pipeline fails, the whole pipeline fails
set -o pipefail

echo "########## Build.sh - start ##########"

# Check for required system packages
echo "Checking for required system packages..."

# Check if we're on a system with apt
if command -v apt-get &> /dev/null; then
    MISSING_PACKAGES=""
    
    # List of required packages
    REQUIRED_PACKAGES=(
        "cmake"
        "gcc-arm-none-eabi"
        "libnewlib-arm-none-eabi" 
        "build-essential" 
        "g++" 
        "libstdc++-arm-none-eabi-newlib"
        "libusb-1.0-0-dev"
    )
    
    # Check each required package
    for pkg in "${REQUIRED_PACKAGES[@]}"; do
        if ! dpkg-query -W -f='${Status}' $pkg 2>/dev/null | grep -q "ok installed"; then
            echo "Package $pkg is not installed."
            MISSING_PACKAGES="$MISSING_PACKAGES $pkg"
        fi
    done
    
    # Install missing packages if any
    if [ ! -z "$MISSING_PACKAGES" ]; then
        echo "Installing missing packages:$MISSING_PACKAGES"
        sudo apt-get update
        sudo apt-get install -y $MISSING_PACKAGES
        echo "Required packages installed."
    else
        echo "All required packages are already installed."
    fi
else
    echo "Warning: This script cannot check for packages on non-Debian based systems."
    echo "Please ensure you have these packages installed:"
    echo "- cmake"
    echo "- gcc-arm-none-eabi"
    echo "- libnewlib-arm-none-eabi"
    echo "- build-essential"
    echo "- g++"
    echo "- libstdc++-arm-none-eabi-newlib"
    echo "- libusb-1.0-0-dev"
fi

# Check if git submodules are initialized
echo "Checking git submodules..."
# Create Dependencies directory if it doesn't exist
DEPS_DIR="$(pwd)/Dependencies"
if [ ! -d "$DEPS_DIR" ]; then
    mkdir -p "$DEPS_DIR"
    echo "Created Dependencies directory"
fi

# Initialize all git submodules if not already done
if [ ! -d "$DEPS_DIR/FreeRTOS-Kernel/.git" ] || [ ! -d "$DEPS_DIR/esp-idf/.git" ]; then
    echo "Initializing git submodules..."
    git submodule update --init --recursive "$DEPS_DIR/FreeRTOS-Kernel"
    git submodule update --init --recursive "$DEPS_DIR/esp-idf"
fi

# Verify FreeRTOS-Kernel is at the correct commit
FREERTOS_DIR="$DEPS_DIR/FreeRTOS-Kernel"
FREERTOS_COMMIT="4f7299d6ea746b27a9dd19e87af568e34bd65b15"
ACTUAL_COMMIT=$(cd "$FREERTOS_DIR" && git rev-parse HEAD)
if [ "$ACTUAL_COMMIT" != "$FREERTOS_COMMIT" ]; then
    echo "FreeRTOS-Kernel is not at the expected commit. Updating..."
    (cd "$FREERTOS_DIR" && git fetch && git checkout "$FREERTOS_COMMIT")
fi

# Verify ESP-IDF is at the correct commit
ESP_IDF_DIR="$DEPS_DIR/esp-idf"
ESP_IDF_TAG="v5.2.5"
ACTUAL_TAG=$(cd "$ESP_IDF_DIR" && git describe --tags --exact-match 2>/dev/null || echo "unknown")
if [ "$ACTUAL_TAG" != "$ESP_IDF_TAG" ]; then
    echo "ESP-IDF is not at the expected version. Updating..."
    (cd "$ESP_IDF_DIR" && git fetch && git checkout "$ESP_IDF_TAG")
fi

# Install the ESP32 toolchain
echo "Installing ESP32 toolchain..."
cd "$ESP_IDF_DIR"
./install.sh esp32
. ./export.sh
cd ../../Application
idf.py set-target esp32
#TODO - find out if this is needed: idf.py menuconfig 
idf.py build

#TODO - move this to Flash.sh
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor

# Export environment variables for CMake
export FREERTOS_KERNEL_PATH="$FREERTOS_DIR"
export ESP_IDF_PATH="$ESP_IDF_DIR"

echo "Using FreeRTOS-Kernel at: $FREERTOS_KERNEL_PATH"
echo "Using ESP-IDF at: $ESP_IDF_PATH"

# Parse command line arguments
CLEAN_BUILD=0
EXAMPLE_NAME=""

for arg in "$@"
do
    # Check for --clean flag
    if [[ "$arg" == "--clean" ]]; then
        CLEAN_BUILD=1
    fi
    
    # Check for --example flag with value
    if [[ "$arg" == --example=* ]]; then
        EXAMPLE_NAME="${arg#*=}"
    fi
done

# Handle clean build if requested
if [ $CLEAN_BUILD -eq 1 ]; then
    echo "Performing clean build..."
    if [ -d "build" ]; then
        rm -rf build
        echo "Removed existing build directory"
    fi
fi

# Create build directory if it does not exist
if [ ! -d "build" ]; then
    mkdir build
    echo "Created build directory"
else
    echo "Build directory already exists"
fi

# Configure the build directory
echo "Configuring the build directory..."
cd build

# Set CMake arguments based on whether an example is selected
#TODO: CMAKE_ARGS=".."

# Run CMake to configure the build system
# This command generates the necessary build files in the 'build' directory.
# These files include:
# - Makefiles or project files for the chosen build system (e.g., Make, Ninja, Visual Studio).
# - Configuration files that define how the project should be built.
# - Dependency files that track which files need to be recompiled when changes are made.
#TODO: cmake $CMAKE_ARGS

# Build the project using the generated build files
# This command compiles the project based on the configuration done by the previous cmake command.
# It uses the generated Makefiles or project files to compile the source code into executables or libraries.
echo "Building..."
#TODO: cmake --build .

echo "Build complete."

echo "Copying output files..."

#TODO - Copy output files to the output directory

echo "Done"

# Prompt the user to press any key to exit
echo "Press any key to exit the script."

# Handle user input
read -n 1 -s  # Read a single keypress silently
echo "Exiting the script."


