#ifndef FLASH_OPERATIONS_H
#define FLASH_OPERATIONS_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "metadata.h"

/*********************** WARNING ***********************/
/* FLASH ADDRESSING QUICK REFERENCE
 * ------------------------------
 * SDK flash functions (flash_range_erase, flash_range_program) expect PHYSICAL addresses:
 * - Physical addresses start at 0x00000000
 * - Must convert from XIP/virtual addresses by subtracting 0x10000000
 * 
 * Example:
 * XIP address:    0x10040000 (where code runs from)
 * Physical addr:  0x00040000 (what to pass to flash functions)
 * 
 * Usage:
 * flash_range_erase(0x40000, FLASH_SECTOR_SIZE);    // ✓ Correct   (physical address)
 * flash_range_erase(0x10040000, FLASH_SECTOR_SIZE); // ✗ Incorrect (XIP address)
 */
/******************************************************/

/**
 * @brief Reads bootloader metadata from flash memory into RAM
 *
 * This function reads the boot metadata structure from the flash memory's boot config sector
 * into a RAM copy. It validates the metadata by checking for a magic number.
 *
 * @param ram_metadata Pointer to a boot_metadata_t structure in RAM (to store the read metadata)
 * 
 * @return true if valid metadata was read successfully
 * @return false if metadata was invalid (magic number mismatch)
 */
bool read_metadata_from_flash(boot_metadata_t *ram_metadata);

/**
 * @brief Writes the current RAM metadata to flash memory
 * 
 * This function writes the boot metadata from RAM to the flash memory's boot config sector.
 * It performs the following steps:
 * 1. Erases the metadata sector (must be 4096-byte aligned)
 * 2. Programs the new metadata (must be 256-byte aligned)
 * 
 * @param ram_metadata Pointer to a boot_metadata_t structure in RAM (to write to flash)
 *
 * @return true if metadata was written successfully
 * @return false if alignment requirements were not met
 */
bool write_metadata_to_flash(const boot_metadata_t *ram_metadata);


bool validate_app_image(uint32_t addr);

#endif // FLASH_OPERATIONS_H