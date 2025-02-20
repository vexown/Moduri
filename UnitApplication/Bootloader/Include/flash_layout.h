#ifndef FLASH_LAYOUT_H
#define FLASH_LAYOUT_H

/*
 * Flash Layout for RP2350 4MB Flash (XIP addressing) in OTA use case:
 * 
 * TODO - design the layout
 */

/* TODO - values below are not fully correct, just a placeholder to give the idea for OTA, correct it before removing this TODO */
/* Flash Configuration */
#define FLASH_BASE             0x10000000   // XIP mapped base address
#define FLASH_PAGE_SIZE        0x100        // 256 bytes
#define FLASH_SECTOR_SIZE      0x1000       // 4KB
#define FLASH_TOTAL_SIZE       0x400000     // 4MB

/* Region Definitions (XIP addresses) */
#define BOOT2_START            (FLASH_BASE + 0x100)    // Fixed by hardware
#define BOOT2_SIZE             0x100                   // 256 bytes

#define BOOTLOADER_START       (FLASH_BASE + 0x200)    // After Boot2
#define BOOTLOADER_SIZE        0x8000                  // 32KB

#define METADATA_START        (BOOTLOADER_START + BOOTLOADER_SIZE)
#define METADATA_SIZE         0x2000                   // 8KB

#define APP_BANK_A_START      (FLASH_BASE + 0x10000)   // 64KB aligned
#define APP_BANK_B_START      (FLASH_BASE + 0x180000)  // 1.5MB after Bank A
#define APP_BANK_SIZE         0x170000                 // 1.5MB per bank

#define RESERVED_START        (FLASH_BASE + 0x300000)  // Last 1MB reserved
#define RESERVED_SIZE         0x100000                 // 1MB

/* Validation */
static_assert((APP_BANK_A_START & 0xFFFF) == 0, "Bank A must be 64KB aligned");
static_assert((APP_BANK_B_START & 0xFFFF) == 0, "Bank B must be 64KB aligned");
static_assert(APP_BANK_B_START + APP_BANK_SIZE <= RESERVED_START, "Banks must not overlap reserved area");

#endif // FLASH_LAYOUT_H