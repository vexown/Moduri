#!/bin/bash

# ****************************************************************************
# Flash.sh
#
# Description:
# This script automates the process of flashing a built project to an ESP32 device.
# It performs the following steps:
# 1. Sets up the ESP-IDF environment
# 2. Connects to the ESP32 device on /dev/ttyUSB0
# 3. Flashes the built firmware to the device
# 4. Prompts the user to press any key to exit the script
#
# Usage:
#   ./Flash.sh
#
# ****************************************************************************
DEPS_DIR="../Dependencies"
ESP_IDF_DIR="$DEPS_DIR/esp-idf"

cd "$ESP_IDF_DIR"
. ./export.sh
cd ../../ESP32/Application

idf.py -p /dev/ttyUSB0 flash

echo "Done"

# Prompt the user to press any key to exit
echo "Press any key to exit the script."

# Handle user input
read -n 1 -s  # Read a single keypress silently
echo "Exiting the script."


