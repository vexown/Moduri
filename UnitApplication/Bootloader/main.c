#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "flash_layout.h"
#include "flash_operations.h"
#include "hardware/platform_defs.h"
#include "hardware/sync.h"

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
    // TODO - below is a general flow of the bootloader, just to give an idea, correct it as needed

    // 2. Read boot metadata from reserved flash sector
    //    - Magic number validation
    //    - CRC check on metadata
    //    - Version information
    //    - Active/Backup bank status
    /* Initialize the metadata structure */
    boot_metadata_t current_metadata =
    {
        .magic = BOOT_METADATA_MAGIC,
        .active_bank = BANK_A,
        .version = 0,
        .app_size = 0,
        .app_crc = 0,
        .update_pending = false,
        .boot_attempts = 0
    };

    // 3. Check for update trigger
    //    - New firmware flag
    //    - Force update command
    //    - Previous boot failures counter

    // 4. Determine which bank to boot
    //    - Check active bank status
    //    - Verify backup bank if update pending
    //    - Handle fallback logic

    // 5. Validate application image
    //    - Verify header magic
    //    - Check CRC/signature
    //    - Validate vector table
    //    - Check firmware version

    // 6. Pre-boot tasks
    //    - Update boot counters
    //    - Clear update flags if needed
    //    - Save any diagnostic info

    // 7. Boot preparation
    //    - Disable interrupts
    //    - Configure vector table
    //    - Setup initial stack pointer

    // 8. Jump to application
    //    - If validation fails, try backup bank
    //    - If all fails, enter recovery mode
    
    if (current_metadata.active_bank == BANK_A) 
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