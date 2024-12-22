#ifndef OS_MANAGER_H
#define OS_MANAGER_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

/*******************************************************************************/
/*                                 MACROS                                      */
/*******************************************************************************/

/* Task-related macros */
#define NUM_OF_TASKS_TO_CREATE 				(2)
#define MAX_NUM_OF_TASKS				    (20)

/*******************************************************************************/
/*                             GLOBAL VARIABLES                                */
/*******************************************************************************/
extern TaskHandle_t monitorTaskHandle;
extern TaskHandle_t networkTaskHandle;
extern SemaphoreHandle_t lidMutex;
extern TimerHandle_t aliveTimer;
extern bool lid_open_global;

#endif