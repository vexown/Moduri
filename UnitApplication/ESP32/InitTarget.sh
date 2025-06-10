#!/bin/bash

# ****************************************************************************
# InitTarget.sh
#
# Description:
# This script sets the target SoC for the project. It needs to be run only once.
#
# Usage:
#   ./InitTarget.sh
#
# ****************************************************************************
DEPS_DIR="../Dependencies"
ESP_IDF_DIR="$DEPS_DIR/esp-idf"

cd "$ESP_IDF_DIR"
. ./export.sh
cd ../../ESP32/Application

echo "---------------------------------------------------"
echo "Available targets:"
echo "---------------------------------------------------"
idf.py set-target --help
echo "---------------------------------------------------"

# Prompt user to select target
echo "---------------------------------------------------"
echo "Please enter the target you want to set. (Select from the list above, e.g., esp32, esp32s2, esp32s3, etc.)"
echo "---------------------------------------------------"
read -p "Target: " target

echo "---------------------------------------------------"
echo "Setting the target to $target..."
echo "---------------------------------------------------"
idf.py set-target "$target"

echo "---------------------------------------------------"
echo "Operation completed"
echo "---------------------------------------------------"

# Prompt the user to press any key to exit
echo "Press any key to exit the script."

# Handle user input
read -n 1 -s  # Read a single keypress silently
echo "Exiting the script."