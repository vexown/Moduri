#!/bin/bash

# ****************************************************************************
# Build.sh
#
# Description:
# This script automates the process of building a project using CMake.
# It performs the following steps:
# 1. Checks if the 'build' directory exists and creates it if not.
# 2. Configures the build directory by running CMake.
# 3. Builds the project using CMake.
# 4. Prompts the user to press 'X' or 'x' to exit the script.
#
# Usage:
# Run this script from the root directory of the project. It assumes that
# CMake is installed and available in the system PATH.
#
# ****************************************************************************

# Exit immediately if any command fails
set -e

# If a command in a pipeline fails, the whole pipeline fails
set -o pipefail

echo "########## Build.sh - start ##########"
echo "Building all components with CMake"

# Parse command line arguments
CLEAN_BUILD=0
for arg in "$@"
do
    case $arg in
        --clean)
        CLEAN_BUILD=1
        shift # Remove --clean from processing
        ;;
    esac
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

# Run CMake to configure the build system
# This command generates the necessary build files in the 'build' directory.
# These files include:
# - Makefiles or project files for the chosen build system (e.g., Make, Ninja, Visual Studio).
# - Configuration files that define how the project should be built.
# - Dependency files that track which files need to be recompiled when changes are made.
cmake ..

# Build the project using the generated build files
# This command compiles the project based on the configuration done by the previous cmake command.
# It uses the generated Makefiles or project files to compile the source code into executables or libraries.
echo "Building..."
cmake --build .

echo "Build complete."

echo "Copying output files..."

cp -v ./SWC/moduri_bank_A.elf     ../output
cp -v ./SWC/moduri_bank_A.elf.map ../output
cp -v ./SWC/moduri_bank_A.uf2     ../output
cp -v ./SWC/moduri_bank_A.bin     ../output

cp -v ./SWC/moduri_bank_B.elf     ../output
cp -v ./SWC/moduri_bank_B.elf.map ../output
cp -v ./SWC/moduri_bank_B.uf2     ../output
cp -v ./SWC/moduri_bank_B.bin     ../output

cp -v ./Bootloader/bootloader.elf ../output
cp -v ./Bootloader/bootloader.elf.map ../output
cp -v ./Bootloader/bootloader.uf2 ../output
cp -v ./Bootloader/bootloader.bin ../output

cp -v ./Bootloader/metadata.bin ../output

echo "Done"

# Prompt the user to press any key to exit
echo "Press any key to exit the script."

# Handle user input
read -n 1 -s  # Read a single keypress silently
echo "Exiting the script."


