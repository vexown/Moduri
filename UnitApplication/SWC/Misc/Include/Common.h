#ifndef COMMON_H
#define COMMON_H

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/

/* Standard includes. */
#include <stdint.h>

/*******************************************************************************/
/*                                 MACROS                                      */
/*******************************************************************************/

/* Error IDs */
#define ERROR_ID_TASK_FAILED_TO_CREATE      (uint8_t)(0x1U)
#define ERROR_ID_WIFI_DID_NOT_CONNECT       (uint8_t)(0x2U)

/* Module IDs */
#define MODULE_ID_OS                        (uint8_t)(0x1U)

/*******************************************************************************/
/*                         GLOBAL FUNCTION DECLARATIONS                        */
/*******************************************************************************/

void CriticalErrorHandler(uint8_t moduleId, uint8_t errorId);

#endif