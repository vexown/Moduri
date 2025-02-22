#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/platform_defs.h"
#include "flash_layout.h"
#include "flash_operations.h"
#include "metadata.h"

/* Validation */
static_assert((APP_BANK_A_START & 0xFFFF) == 0, "Bank A must be 64KB aligned");
static_assert((APP_BANK_B_START & 0xFFFF) == 0, "Bank B must be 64KB aligned");

/* RAM copy of metadata */
static boot_metadata_t ram_current_metadata = 
{
    .magic = 0xFFFFFFFF, // Set to invalid value initially
    .active_bank = BANK_A,
    .version = 0,
    .app_size = 0,
    .app_crc = 0,
    .update_pending = false,
    .boot_attempts = 0
};

bool validate_bank(uint32_t bank_start)
{
    // TODO - this has only basic validation, add more checks such as CRC, signature, etc.
    // Check vector table
    const uint32_t* vector_table = (const uint32_t*)bank_start;
    
    // Stack pointer should be in RAM
    if (vector_table[0] < 0x20000000 || vector_table[0] > 0x20082000) return false;
    
    // Reset vector should be in flash
    if (vector_table[1] < 0x10000000 || vector_table[1] > 0x10400000) return false;
    
    return true;
}

bool find_valid_application(void)
{
    /* First try to read existing metadata */
    if (read_metadata_from_flash(&ram_current_metadata)) 
    {
        return true;  /* Valid metadata found */
    }

    /* No valid metadata - try to recover */
    if (validate_bank(APP_BANK_A_START)) 
    {
        /* Bank A seems valid - create new metadata */
        ram_current_metadata.magic = BOOT_METADATA_MAGIC;
        ram_current_metadata.active_bank = BANK_A;
        ram_current_metadata.version = 0;  /* Start from 0 */
        ram_current_metadata.boot_attempts = 0;
        
        return write_metadata_to_flash(&ram_current_metadata);
    }
    
    if (validate_bank(APP_BANK_B_START)) 
    {
        /* Bank B seems valid - create new metadata */
        ram_current_metadata.magic = BOOT_METADATA_MAGIC;
        ram_current_metadata.active_bank = BANK_B;
        ram_current_metadata.version = 0;  /* Start from 0 */
        ram_current_metadata.boot_attempts = 0;
        
        return write_metadata_to_flash(&ram_current_metadata);
    }

    /* No valid banks found */
    return false;
}

/**
 * @brief Handles a pending firmware update by validating and switching banks
 * @return true if update handled successfully, false if error occurred
 */
static bool handle_pending_update(void)
{
    /* Get the inactive bank address */
    uint32_t inactive_bank = (ram_current_metadata.active_bank == BANK_A) ? 
                            APP_BANK_B_START : APP_BANK_A_START;
    
    /* Validate the inactive bank (where update should be) */
    if (!validate_bank(inactive_bank)) 
    {
        /* Update validation failed - clear pending flag and keep current bank */
        ram_current_metadata.update_pending = false;
        ram_current_metadata.boot_attempts = 0;
        return write_metadata_to_flash(&ram_current_metadata);
    }

    /* Update is valid - switch banks */
    ram_current_metadata.active_bank = (ram_current_metadata.active_bank == BANK_A) ? BANK_B : BANK_A;
    ram_current_metadata.update_pending = false;
    ram_current_metadata.boot_attempts = 0;
    
    /* Save new metadata */
    return write_metadata_to_flash(&ram_current_metadata);
}

/**
  \brief   Set Main Stack Pointer
  \details Assigns the given value to the Main Stack Pointer (MSP). 
           Function from cmsis_gcc.h from official ARM CMSIS_5 repository (https://github.com/ARM-software/CMSIS_5/tree/develop)
  \param [in]    topOfMainStack  Main Stack Pointer value to set
 */
static inline void __set_MSP(uint32_t topOfMainStack)
{
    __asm volatile ("MSR msp, %0" : : "r" (topOfMainStack) : );
}

static void jump_to_application(uint32_t app_bank_address)
{
    // Validate bank address
    if (app_bank_address != APP_BANK_A_START && 
        app_bank_address != APP_BANK_B_START) {
        // Invalid bank address - could add error handling here
        return;
    }

    // Disable interrupts before VTOR change
    (void)save_and_disable_interrupts();

    // Clear any pending memory transactions
    __dsb();
    __isb();
    
    #ifdef USE_TRUSTZONE
    SAU->CTRL |= SAU_CTRL_ENABLE_Msk;
    #endif
    
    // Set vector table location to specified bank
    scb_hw->vtor = app_bank_address;
    
    // Get application stack pointer and reset handler from vector table
    uint32_t *app_vector_table = (uint32_t *)app_bank_address;
    uint32_t app_stack_pointer = app_vector_table[0];
    uint32_t app_reset_handler = app_vector_table[1];
    
    // Set main stack pointer
    __set_MSP(app_stack_pointer);
    
    // Memory barriers before jump
    __dsb();
    __isb();
    
    // Jump to application
    typedef void (*reset_handler_t)(void);
    reset_handler_t reset_handler = (reset_handler_t)app_reset_handler;
    reset_handler();
}

int main() 
{
    stdio_init_all();

    // Find and validate application
    // TODO - add authentication and integrity checks
    if (!find_valid_application()) 
    {
        //enter_recovery_mode(); TODO - implement recovery mode
        while(1) tight_loop_contents(); // For now, just loop forever
    }

    // TODO - add version checks, anti-rollback, etc.
    if (ram_current_metadata.update_pending) 
    {
        handle_pending_update();
    }
    
    if (ram_current_metadata.active_bank == BANK_A) 
    {
        jump_to_application(APP_BANK_A_START);
    } 
    else 
    {
        jump_to_application(APP_BANK_B_START);
    }

    // Should never reach here
    while (1) 
    {
        tight_loop_contents();
    }
}