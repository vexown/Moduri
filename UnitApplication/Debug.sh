#!/bin/bash

# ****************************************************************************
# Debug.sh
#
# Description:
# This script automates the process of setting up an OpenOCD debug session
# for an RP2040 microcontroller. It performs the following steps:
# 1. Opens a new Windows Terminal (WT) window and runs the StartOpenOCD.sh script,
#    which starts the OpenOCD server for the target device.
# 2. Waits for 1 second to ensure that OpenOCD is fully initialized.
# 3. Launches gdb-multiarch with the specified ELF file and executes GDB commands.
# 4. The GDB commands perform the following actions:
#    - Connect to the OpenOCD server running on localhost at port 3333.
#    - Reset the MCU and halt execution immediately after the reset.
#    - Load the program into the target device's memory.
#    - Set a breakpoint at the main function.
#    - Start running the program.
#
# Usage:
# Run this script to start debugging your embedded application on the RP2040
# microcontroller. Ensure that the StartOpenOCD.sh script is correctly set up
# to initialize the OpenOCD session before running this script.
#
# Role of OpenOCD:
# OpenOCD (Open On-Chip Debugger) primarily acts as a bridge between
# the host computer and the target MCU (Microcontroller Unit). It provides:
# 1. Debug adaptor support: Interfaces with various hardware debug adaptors.
# 2. Target support: Understands different MCU architectures and their debug interfaces.
# 3. Protocol translation: Converts GDB commands into the appropriate debugging protocol
#    (e.g., JTAG, SWD) that the target MCU understands.
# 4. Flash programming: Can often be used to program the MCU's flash memory.
#
# Role of gdb-multiarch:
# GDB (GNU Debugger), specifically gdb-multiarch, is the actual debugger where you:
# 1. Interact with your code: Examine variables, memory, and registers.
# 2. Control execution: Start, stop, step through code, and set breakpoints.
# 3. Analyze program behavior: Track program flow and detect issues.
# 4. Support multiple architectures: gdb-multiarch can debug different CPU architectures.
#
# Interaction between OpenOCD and GDB:
# 1. OpenOCD starts a GDB server, listening for connections from GDB clients.
# 2. GDB connects to this server, usually over a TCP/IP port.
# 3. GDB sends commands to OpenOCD, which translates and forwards them to the target MCU.
# 4. OpenOCD receives responses from the MCU and sends them back to GDB.
#
# This separation allows GDB to remain architecture-independent while OpenOCD
# handles the hardware-specific details of communicating with the target MCU.
#
# Commonly Used GDB Commands for RP2040 Debugging
#
# 1. target remote [host:port]          # Connect to a remote GDB server (e.g., OpenOCD)
# 2. monitor reset halt                 # Reset the MCU and halt execution immediately after the reset
# 3. load                               # Load the program into the target device's memory
# 4. break/b [function_name]            # Set a breakpoint at the specified function (e.g., break main)
# 5. continue                           # Continue execution until the next breakpoint or program end
# 6. step                               # Step into the next line of code (go into function calls)
# 7. next                               # Step over the next line of code (execute without going into functions)
# 8. finish                             # Run until the current function returns
# 9. print [variable_name]              # Print the current value of a variable
# 10. info registers                    # Display the current values of all CPU registers
# 11. x/[format] [address]              # Examine memory at the specified address (e.g., x/10x 0x20000000)
# 12. backtrace                         # Show the call stack (function calls leading to the current point)
# 13. delete [breakpoint_number]        # Delete the specified breakpoint
# 14. info breakpoints                  # List all current breakpoints
# 15. quit                              # Exit GDB

# ****************************************************************************

# Open a new Windows Terminal window and run the OpenOCD startup script
wt.exe --title "OpenOCD" bash -c "./StartOpenOCD.sh && exec bash"

# Wait for 1 second to ensure OpenOCD has initialized
sleep 1

# Launch GDB for core0
wt.exe --title "Core0 GDB" bash -c "./StartGDB_Core0.sh && exec bash"

# Launch GDB for core1
wt.exe --title "Core1 GDB" bash -c "./StartGDB_Core1.sh && exec bash"
