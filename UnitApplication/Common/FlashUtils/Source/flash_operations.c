
#include <string.h>
#include "flash_operations.h"
#include "flash_layout.h"
#include "metadata.h"

bool read_metadata_from_flash(boot_metadata_t *ram_metadata)
{
    /* Point to the Flash Memory Location of the Boot Config sector which contains the metadata */
    const boot_metadata_t* flash_metadata = (boot_metadata_t*)BOOT_CONFIG_START;
    
    /* First check if we have valid metadata */
    if (flash_metadata->magic != BOOT_METADATA_MAGIC) 
    {
        /* No valid metadata found */
        return false;
    }
    
    /* Copy metadata to RAM */
    memcpy(ram_metadata, flash_metadata, sizeof(boot_metadata_t));

    return true;
}

bool write_metadata_to_flash(const boot_metadata_t *ram_metadata)
{
    uint32_t flash_offset = BOOT_CONFIG_START - FLASH_BASE; // Account for the XIP base address (0x10000000)
    uint32_t size_in_bytes = BOOT_CONFIG_SIZE;

    /*******************  Erase metadata sector *****************/

    /* Validate sizes for erase operation */
    if ((flash_offset & 0xFFF) != 0 || (size_in_bytes & 0xFFF) != 0) 
    {
        return false;  /* Must be aligned to 4096-byte sectors */
    }

    /* Disable interrupts during flash operations */
    uint32_t ints = save_and_disable_interrupts();

    /* flash_offset - Offset into flash, in bytes, to start the erase. Must be aligned to a 4096-byte flash sector.
       size_in_bytes - How many bytes to erase. Must be a multiple of 4096 bytes (one sector) */
    flash_range_erase(flash_offset, size_in_bytes);
    
    /*******************  Program metadata sector *****************/
    /* Calculate aligned size for programming */
    uint32_t aligned_size = ((sizeof(boot_metadata_t) + 255) & ~255);  // Round up to next 256-byte boundary

    /* Prepare aligned buffer */
    uint8_t aligned_buffer[aligned_size];
    memset(aligned_buffer, 0xFF, aligned_size);  // Fill with 0xFF (erased flash state)
    memcpy(aligned_buffer, ram_metadata, sizeof(boot_metadata_t));

    /* Validate offset alignment */
    if ((flash_offset & 0xFF) != 0) 
    {
        restore_interrupts(ints);
        return false;  /* Must be aligned to 256-byte pages */
    }

    /* flash_offset - Offset into flash, in bytes, to start to program. Must be aligned to a 256-byte flash page.
       data - Pointer to the data to be programmed.
       size_in_bytes - Must be a multiple of 256 bytes (one page) */
    flash_range_program(BOOT_CONFIG_START - FLASH_BASE, 
                       (const uint8_t*)aligned_buffer,
                       sizeof(boot_metadata_t)); 

    /* Re-enable interrupts */
    restore_interrupts(ints);

    return true;
}

