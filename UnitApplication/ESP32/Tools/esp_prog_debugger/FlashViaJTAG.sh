#!/bin/bash

# ****************************************************************************
# FlashViaJTAG.sh
#
# Description:
# This script flashes an ESP32 application binary using OpenOCD and a JTAG debugger.
# It performs the following steps:
# 1. Sets up the ESP-IDF environment
# 2. Uses OpenOCD to program the specified binary file to the ESP32's flash memory.
#
# Usage:
#   ./FlashViaJTAG.sh
#
# Ensure that the binary file (e.g., build/moduri.bin) exists and the path is correct.
# The JTAG debugger must be connected to the ESP32.
#
# Hardware Connections:
#   Refer to the JTAG connection details in StartDebug.sh or the ESP32 documentation.
#   The script assumes a standard JTAG setup compatible with OpenOCD.
#
#   IMPORTANT: Although you can theoretically power each board separately, I have found
#   that the ESP32 board must be powered by the ESP-PROG board directly to achieve a successful
#   connection.
#
# OpenOCD Configuration:
#   The script uses 'board/esp32-wrover-kit-3.3v.cfg' by default.
#   Adjust the OpenOCD configuration file (-f option) if you are using a different board or JTAG adapter.
#   Espressif's documentation explains which .cfg files to use for different ESP32 modules.
#   See https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/jtag-debugging/tips-and-quirks.html#jtag-debugging-tip-openocd-configure-target
#
# ****************************************************************************


DEPS_DIR="../../../Dependencies"
ESP_IDF_DIR="$DEPS_DIR/esp-idf"

cd "$ESP_IDF_DIR"
. ./export.sh
cd ../../ESP32/Application

echo "Starting the ESP32 Debugger..."

openocd -f board/esp32-wrover-kit-3.3v.cfg -c "program_esp build/moduri.bin 0x10000 verify exit"


