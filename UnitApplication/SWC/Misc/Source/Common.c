/**
 * File: Common.c
 * Description: Various project-wide common functions, types, macros etc. Not specific to any SWC.
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/

/* Standard includes. */
#include <stdio.h>

/* Kernel includes */
#include "FreeRTOS.h"
#include "task.h"   

/* Misc includes */
#include "Common.h"

/*******************************************************************************/
/*                          GLOBAL FUNCTION DEFINITIONS                        */
/*******************************************************************************/

/* 
 * Function: CriticalErrorHandler
 * 
 * Description:  Function to trap execution and with an error code
 * 
 * Parameters:
 *   - moduleId: id of the module (SWC) where the error happened
 *   - errorId: id of the specific error
 * 
 * Returns: void
 */
void CriticalErrorHandler(uint8_t moduleId, uint8_t errorId)
{
    printf("CRTICIAL ERROR OCCURED. moduleId: %u, errorId: %u. Going into endless loop...\n", moduleId, errorId);

    /* Enter a critical section. The idea here is to prevent leaving this endless loop by disabling all interrupts globally.
    As stated in FreeRTOS documentation:
        Preemptive context switches only occur inside an interrupt, so will not occur when interrupts are disabled. Therefore, the 
        task that called taskENTER_CRITICAL() is guaranteed to remain in the Running state until the critical section is exited, 
        unless the task explicitly attempts to block or yield (which it should not do from inside a critical section). */ 
    taskENTER_CRITICAL();
    
    /* Infinite loop to trap execution */
    while(1)
    {
        /* TODO - You can also add some flashing LEDs or other indicators  */
    }
}

