#ifndef FLASH_OPERATIONS_H
#define FLASH_OPERATIONS_H

#include <stdbool.h>
#include <stdint.h>

// Magic value for metadata validation
#define BOOT_METADATA_MAGIC 0xB007B007

#define BANK_A 0
#define BANK_B 1

// Metadata structure - TODO - this is a general idea for metadata needed for OTA updates, verify and correct it if needed
typedef struct 
{
    uint32_t magic;           // Magic number for validation
    uint8_t active_bank;      // 0 = Bank A, 1 = Bank B
    uint32_t version;         // Firmware version
    uint32_t app_size;        // Size of application
    uint32_t app_crc;         // CRC of application
    bool update_pending;      // Update requested flag
    uint8_t boot_attempts;    // Number of boot attempts
} boot_metadata_t;

// TODO - functions, data structures, etc. for flash operations, just a placeholder for now, define as needed
// Global metadata instance
extern boot_metadata_t current_metadata;

// Function prototypes
bool read_metadata(void);
bool write_metadata(void);
bool validate_app_image(uint32_t addr);

#endif // FLASH_OPERATIONS_H