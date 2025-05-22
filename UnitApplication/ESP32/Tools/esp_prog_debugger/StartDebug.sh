#!/bin/bash

# ****************************************************************************
# StartDebug.sh
#
# Description:
# This script sets up the environment for debugging an ESP32 application using OpenOCD and GDB.
# It performs the following steps:
# 1. Sets up the ESP-IDF environment
# 2. Launches OpenOCD in a new terminal window
# 3. Waits for OpenOCD to initialize
# 4. Launches GDB (via idf.py gdb) in another new terminal window
#
# Usage:
#   ./StartDebug.sh
#
# Hardware Connections:
#   Below is the standard and most common JTAG interface for the original ESP32 series of chips and modules (ESP32-WROOM-32 and ESP32-WROVER):
#   ---------------------------
#   ESP-PROG -> ESP32
#   --------------------------
#   TMS -> GPIO14 (Test Mode Select)
#   TDI -> GPIO12 (Test Data In)
#   TCK -> GPIO13 (Test Clock)
#   TDO -> GPIO15 (Test Data Out)
#   GND -> GND (Ground)
#   VCC -> 3.3V (Power Supply)
#   --------------------------
#   In case of newer ESP32 chips (ESP32-S2, ESP32-S3, ESP32-C3, etc.), the JTAG pinout may vary. Please refer to the specific datasheet or technical 
#   reference manual for the correct pin mapping.
#
#   IMPORTANT: Although you can theoretically power each board separately, I have found
#   that the ESP32 board must be powered by the ESP-PROG board directly to achieve a successful
#   connection.
#
# OpenOCD Configuration:
#   Espressif's documentation explains which .cfg files to use for different ESP32 modules.
#   See https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/jtag-debugging/tips-and-quirks.html#jtag-debugging-tip-openocd-configure-target
#
#   For ESP32-WROOM-32, the configuration file is: board/esp32-wrover-kit-3.3v.cfg
#   Located in my case at: /home/blankmcu/.espressif/tools/openocd-esp32/v0.12.0-esp32-20241016/openocd-esp32/share/openocd/scripts/board/esp
#
# ****************************************************************************

DEPS_DIR="../../../Dependencies"
ESP_IDF_DIR="$DEPS_DIR/esp-idf"

cd "$ESP_IDF_DIR"
. ./export.sh
cd ../../ESP32/Application

echo "Starting the ESP32 Debugger..."

# Launch OpenOCD in a new terminal window
# The 'exec bash' at the end keeps the terminal open after openocd exits,
# allowing you to see any output or errors.
gnome-terminal --title="OpenOCD ESP32" -- bash -c "openocd -f board/esp32-wrover-kit-3.3v.cfg -c 'adapter speed 10000' -c 'ftdi tdo_sample_edge falling'; exec bash" &
# Wait a couple of seconds for OpenOCD to initialize.
# GDB needs OpenOCD to be running and listening before it can connect.
sleep 2

# Launch GDB (via idf.py gdb) in another new terminal window
# This command must be run from the ESP-IDF project directory.
# The script already navigates to ESP32/Application, which is assumed to be the project directory.
gnome-terminal --title="GDB ESP32" -- bash -c "idf.py gdb; exec bash" &

