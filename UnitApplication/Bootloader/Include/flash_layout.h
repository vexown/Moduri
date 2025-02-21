#ifndef FLASH_LAYOUT_H
#define FLASH_LAYOUT_H

/*
 * Flash Layout for RP2350 4MB Flash (XIP addressing) in OTA use case:
 * 
 * TODO - design the layout
 */

/* TODO - values below are not fully correct, just a placeholder to give the idea for OTA, correct it before removing this TODO */
/* Flash Configuration */
#define FLASH_BASE                  0x10000000   // XIP mapped base address
#define FLASH_TOTAL_SIZE            0x400000     // 4MB

/* Region Definitions (XIP addresses) */
#define BOOTLOADER_START            (FLASH_BASE)    
#define BOOTLOADER_SIZE             0x3F000      // 252KB

#define BOOT_CONFIG_START           (BOOTLOADER_START + BOOTLOADER_SIZE)
#define BOOT_CONFIG_SIZE            0x1000       // 4KB

#define APP_BANK_ACTIVE_START       (FLASH_BASE + BOOTLOADER_SIZE + BOOT_CONFIG_SIZE)   
#define APP_BANK_SIZE               0x1A0000     // 1664KB
#define APP_BANK_INACTIVE_START     (APP_BANK_ACTIVE_START + APP_BANK_SIZE)  

#define RESERVED_START              (APP_BANK_INACTIVE_START + APP_BANK_SIZE)
#define RESERVED_SIZE               0x80000      // 4MB - 256KB - 1664KB - 1664KB = 512KB

/* Validation */
static_assert((APP_BANK_ACTIVE_START & 0xFFFF) == 0, "Active Bank must be 64KB aligned");
static_assert((APP_BANK_INACTIVE_START & 0xFFFF) == 0, "Inactive Bank must be 64KB aligned");
static_assert(APP_BANK_INACTIVE_START + APP_BANK_SIZE <= RESERVED_START, "Banks must not overlap reserved area");

#endif // FLASH_LAYOUT_H