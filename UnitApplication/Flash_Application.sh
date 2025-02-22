#!/bin/bash

echo "Flashing the board using Picoprobe and OpenOCD"

# Set the path to the OpenOCD installation directory - we are using the OpenOCD version from RPi repository https://github.com/raspberrypi/openocd
OPENOCD_PATH="/home/blankmcu/pico/openocd"

$OPENOCD_PATH/src/openocd \
    -s "/home/blankmcu/pico/openocd/tcl" \
    -f "$OPENOCD_PATH/tcl/interface/cmsis-dap.cfg" \
    -f "$OPENOCD_PATH/tcl/target/rp2350.cfg" \
    -c "adapter speed 5000" \
    -c "program output/moduri_bank_A.elf verify reset exit"

# Prompt the user to press any key to exit
echo "Press any key to exit the script."
read -n 1 -s  # Read a single keypress silently
echo "Exiting the script."

