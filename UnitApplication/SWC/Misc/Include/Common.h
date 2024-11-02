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

/* Enable printf only in Debug builds */
#ifdef DEBUG_BUILD
    #define LOG printf
#else /* RELEASE_BUILD or build type not defined (or defined with invalid type) */
    #define LOG(...)
#endif

/* Error IDs */
#define ERROR_ID_TASK_FAILED_TO_CREATE              (uint8_t)(0x1U)
#define ERROR_ID_WIFI_DID_NOT_CONNECT               (uint8_t)(0x2U)
#define ERROR_ID_RTOS_OBJECTS_FAILED_TO_CREATE      (uint8_t)(0x3U)
#define ERROR_ID_SW_TIMER_FAILED_TO_START           (uint8_t)(0x4U)

/* Module IDs */
#define MODULE_ID_OS    (uint8_t)(0x1U)

/* Misc defines */
#define E_OK           0
#define ERRNO_FAIL     -1
#define NO_FLAG        0
#define NO_DELAY       0

/*******************************************************************************/
/*                         GLOBAL FUNCTION DECLARATIONS                        */
/*******************************************************************************/

void CriticalErrorHandler(uint8_t moduleId, uint8_t errorId);

#endif