#!/bin/bash

echo "Flashing the board using Picoprobe and OpenOCD"

openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "program output/moduri.elf verify reset exit" 

# Prompt the user to press any key to exit
echo "Press any key to exit the script."

# Handle user input
read -n 1 -s  # Read a single keypress silently
echo "Exiting the script."