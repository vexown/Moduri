#!/bin/bash

# Launch GDB for core1 with the specified ELF file and execute GDB commands from gdb_commands_core1.txt
gdb-multiarch output/moduri.elf -x gdb_commands_core1.txt
