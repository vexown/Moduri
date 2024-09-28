/**
 * File: Common.c
 * Description: Various project-wide common functions, types, macros etc. Not specific to any SWC.
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/

/* Standard includes. */
#include <stdio.h>

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
    
    /* Infinite loop to trap execution */
    while(1)
    {
        /* TODO - You can also add some flashing LEDs or other indicators  */
    }
}


