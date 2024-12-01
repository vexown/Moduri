#!/bin/bash

# Script for Analyzing ELF and BIN Files for Embedded Systems
#
# This script analyzes an ELF file and its associated binary (BIN) file, generating various reports and outputs
# to aid in the inspection of memory usage, symbols, and memory sections. It segregates symbols based on memory 
# ranges for RAM and FLASH, generates disassembly, a hex dump with adjusted flash addresses, and provides a summary 
# of the binary and ELF file properties.
#
# Usage:
#   ./analyze_binary.sh (in the same directory as your .elf and .bin binaries)
#
# Dependencies:
#   - ARM toolchain (arm-none-eabi)
#   - `xxd` for hex dumping

# Raspberry Pi Pico W Memory Map:
# (Source - Address Map chapter in https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf)
#
# - ROM: 0x00000000 
#        This is ACTUAL ROM - Read Only Memory - The ROM contents are fixed at the time the silicon is manufactured
#        It's not some relict bullshit of calling regular writeable flash/EEPROM as ROM, as manufacturers and people often do.
#        It contains the initial startup routine, flash boot seq, flash programming routines, USB mass storage dev etc.
# - RAM: 264KB SRAM at 0x20000000 - 0x20042000
#        Divided into 6 banks for vastly improved mem bandwith for multiple masters. For SW - it can be treated as single 264kB mem region.
#        Can be used for anything, processor code, data buffers... there is no restrictions. Accessed via AHB-Lite arbiter.
# - Program Flash (XIP): 2MB XIP at 0x10000000 - 0x10200000
#        External Flash, accessed via QSPI interface using execute-in-place (XIP) hardware. Meaning it can be addressed and accessed by the 
#        system as it were internal memory. Once correctly configured by RP2040's bootrom and flash second stage, the software can treat 
#        flash as a large read-only memory (the memory in theory is writeable but in this context of XIP, it is read-only for the SW)
# - APB Peripheral Registers: Start at 0x40000000
#        APB Peripherals include: SYSCFG, CLOCKS, PLL, UART, SPI, I2C, ADC, PWM, TIMER, WATCHDOG etc.
# - AHB-Lite Peripheral Registers: Start at 0x50000000
#        AHB-Lite Peripherals: only DMA. RP2040 DMA (Direct Memory Access) controller has separate read and write connections, enabling 
#        efficient bulk data transfers without processor intervention.
#        For DMA examples see https://github.com/raspberrypi/pico-examples/tree/master/dma 
# - IOPORT Registers: Start at 0xD0000000
#        Used to access SIO - The Single-cycle IO block (SIO) contains several peripherals that require low-latency, deterministic access 
#        from the processors. All IOPORT reads and writes (and therefore all SIO accesses) take place in exactly one cycle, unlike the main AHB-Lite
#        system bus, where the Cortex-M0+ requires two cycles for a load or store, and may have to wait longer due to contention from other system 
#        bus masters. This is vital for interfaces such as GPIO, which have tight timing requirements.
#        SIO peripherals include: CPUID, GPIO Control, HW Spinlocks, Inter-processor FIFOs, Integer Divider, Interpolator.
# - Cortex-M0+ core Internal Registers: Start at 0xE0000000
#        Info from RP2040 datasheet but they took it from Cortex-M0+ Technical Reference Manual: https://developer.arm.com/documentation/ddi0484/latest
#        The ARM Cortex-M0+ processor is a very low gate count, highly energy efficient processor that is intended for microcontroller and deeply 
#        embedded applications that require an area optimized, low-power processor.
#        ARM Cortex-M0+ registers refer to internal ARM core stuff such as: SysTick, NVIC, SCR, CCR, VTOR, ICSR, MPU etc.

BIN_FILE="moduri.bin"
ELF_FILE="moduri.elf"
TOOLCHAIN_PREFIX=arm-none-eabi
OUTPUT_DIR="binary_analysis"

# Check if both the ELF and BIN files exist
if [ ! -f "$ELF_FILE" ]; then
    echo "Error: ELF file '$ELF_FILE' not found in the current directory."
    exit 1
fi

if [ ! -f "$BIN_FILE" ]; then
    echo "Error: Binary file '$BIN_FILE' not found in the current directory."
    exit 1
fi

# Create output directory
mkdir -p $OUTPUT_DIR

echo "Analyzing ELF: $ELF_FILE"
echo "Analyzing BIN: $BIN_FILE"
echo "Output will be saved in $OUTPUT_DIR/"

# 1. File Overview
echo "File overview:" > $OUTPUT_DIR/overview.txt
file $ELF_FILE >> $OUTPUT_DIR/overview.txt
${TOOLCHAIN_PREFIX}-size $ELF_FILE >> $OUTPUT_DIR/overview.txt

# 2. Disassembly
echo "Generating disassembly from ELF..."
${TOOLCHAIN_PREFIX}-objdump -d $ELF_FILE > $OUTPUT_DIR/disassembly.txt

# 3. Symbol Analysis
echo "Analyzing symbols from ELF (RAM and FLASH segregation)..."
echo -e "Address\tSize\tType\tName" > $OUTPUT_DIR/symbols_ram.txt
echo -e "Address\tSize\tType\tName" > $OUTPUT_DIR/symbols_flash.txt

# Temporary files to store lines before sorting
temp_ram_file=$(mktemp)
temp_flash_file=$(mktemp)

while read -r line; do
    # Extract address, size, type, and name from each line
    address=$(echo $line | awk '{print $1}')
    size=$(echo $line | awk '{print $2}')
    type=$(echo $line | awk '{print $3}')
    name=$(echo $line | awk '{print $4}')

    # Convert address to a decimal value for comparison
    address_dec=$((16#$address))

    # Segregate based on address ranges:
    if [[ $address_dec -ge 0x20000000 && $address_dec -lt 0x20042000 ]]; then
        # Address is in SRAM (RAM)
        echo -e "$line" >> $temp_ram_file
    elif [[ $address_dec -ge 0x10000000 && $address_dec -lt 0x10200000 ]]; then
        # Address is in FLASH
        echo -e "$line" >> $temp_flash_file
    else
        # Address is outside of the defined RAM and FLASH ranges
        echo "Error: Address $address (0x$address_dec) is outside of defined RAM/FLASH ranges" >> $OUTPUT_DIR/symbols_outside_ranges.txt
    fi
done < <(${TOOLCHAIN_PREFIX}-nm --numeric-sort --size-sort --print-size $ELF_FILE)

# Sort symbols by address (from lowest to highest)
sort -k1,1n $temp_ram_file > $OUTPUT_DIR/symbols_ram.txt
sort -k1,1n $temp_flash_file > $OUTPUT_DIR/symbols_flash.txt

# Clean up temporary files
rm $temp_ram_file $temp_flash_file

# Generate the top 10 largest symbols file (done once and used later in the summary)
echo "Generating top 10 largest symbols..."
# Combine the RAM and FLASH symbol files and sort them by size
echo -e "Address\tSize\tType\tName" > $OUTPUT_DIR/large_symbols.txt
sort -nrk 2 $OUTPUT_DIR/symbols_ram.txt $OUTPUT_DIR/symbols_flash.txt | head -n 10 >> $OUTPUT_DIR/large_symbols.txt

# 4. Hex Dump with Adjusted Addresses
FLASH_BASE_ADDR=0x10000000  # Base address for flash memory (for Raspberry Pi Pico W)

echo "Generating hex dump from BIN with flash address adjustment..."
xxd -g 1 -u $BIN_FILE | awk -v base=$FLASH_BASE_ADDR '
  BEGIN {
    address = base
  }
  {
    printf("%08X: ", address)  # Print the address
    address += 16              # Increment address by 16 for each line
    for (i=2; i<=NF; i++)      # Print the hex bytes
      printf("%s ", $i)
    print ""                    # Newline after each line
  }
' > $OUTPUT_DIR/hexdump.txt

# 5. Memory Sections
echo "Analyzing memory sections from ELF..."
${TOOLCHAIN_PREFIX}-objdump -h $ELF_FILE > $OUTPUT_DIR/memory_sections.txt

# 6. Generate Summary Report
echo "Generating summary report..."
SUMMARY_FILE="$OUTPUT_DIR/summary.txt"
{
    echo "Binary Analysis Summary"
    echo "======================="
    echo ""
    echo "File Overview:"
    file $ELF_FILE
    echo ""
    echo "Memory Usage [bytes] (from ELF):"
    ${TOOLCHAIN_PREFIX}-size $ELF_FILE
    echo ""
    echo "Total Size of Binary [bytes] (from BIN):"
    stat --format=%s $BIN_FILE  # Get size of the .bin file in bytes
    echo ""
    echo "Top 10 Largest Symbols:"
    cat $OUTPUT_DIR/large_symbols.txt
    echo ""
    echo "Memory Sections (from ELF):"
    ${TOOLCHAIN_PREFIX}-objdump -h $ELF_FILE | grep -E 'Idx|\.text|\.data|\.bss|\.rodata'
    echo ""
    echo "Hex Dump (from BIN with adjusted flash addresses):"
    echo "Saved in: hexdump.txt (full content not shown here)"
    echo ""
    echo "For detailed results, see the individual files in $OUTPUT_DIR/"
} > $SUMMARY_FILE

echo "Analysis complete. Check the $OUTPUT_DIR directory for results."
