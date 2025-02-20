#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "flash_layout.h"


int main() 
{
    // TODO - below is a general flow of the bootloader, just to give an idea, correct it as needed
    
    // 1. Initialize critical hardware
    //    - Configure clocks
    //    - Setup watchdog
    //    - Initialize flash interface

    // 2. Read boot metadata from reserved flash sector
    //    - Magic number validation
    //    - CRC check on metadata
    //    - Version information
    //    - Active/Backup bank status

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

    // Should never reach here
    while (1) 
    {
        tight_loop_contents();
    }
}