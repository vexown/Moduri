/**
 * File: Monitor_main.c
 * Description: Implementation of the main Monitor function 
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/

/* Standard includes. */
#include <stdio.h>
#include <string.h>

/* Kernel includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* pico-sdk includes */
#include "pico/stdlib.h"
#include "hardware/timer.h"

/* WiFi includes */
#include "WiFi_TCP.h"

/* OS includes */
#include "OS_manager.h"

/* Misc includes */
#include "Common.h"

/*******************************************************************************/
/*                                 MACROS                                      */
/*******************************************************************************/

/*******************************************************************************/
/*                               DATA TYPES                                    */
/*******************************************************************************/

/* Structure to hold system statistics */
typedef struct 
{
    HeapStats_t heap_stats;
    uint32_t totalRunTime;
    UBaseType_t currentNumOfTasks;
} SystemStats_t;

/*******************************************************************************/
/*                             STATIC VARIABLES                                */
/*******************************************************************************/

static TaskStatus_t taskStatusArray[MAX_NUM_OF_TASKS];
static absolute_time_t stats_start_time;

/*******************************************************************************/
/*                          STATIC FUNCTION DECLARATIONS                       */
/*******************************************************************************/

/*******************************************************************************/
/*                          GLOBAL FUNCTION DEFINITIONS                        */
/*******************************************************************************/

/* 
 * Function: Monitor_MainFunction
 * 
 * Description: Main implementation of Monitor functionalities of the system
 * It is periodically (every MONITOR_TASK_PERIOD_TICKS) executed from the Monitor 
 * FreeRTOS task
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 *
 */
void Monitor_MainFunction(void)
{
    static SystemStats_t stats;
    static uint32_t prevTotalRunTime;
    static configRUN_TIME_COUNTER_TYPE totalRunTime;
    static UBaseType_t arraySize, populatedArraySize;
    
    /* Get heap statistics */ 
    vPortGetHeapStats(&stats.heap_stats);
    
    /* Get task statistics */ 
    arraySize = uxTaskGetNumberOfTasks();
    stats.currentNumOfTasks = arraySize;
    totalRunTime = 0;
    populatedArraySize = uxTaskGetSystemState(taskStatusArray, MAX_NUM_OF_TASKS, &totalRunTime); // returns 0 if if the MAX_NUM_OF_TASKS is too small 
    prevTotalRunTime = totalRunTime;
    
    /* If WiFi State Machine is in Monitoring mode send stats via TCP, otherwise simply LOG them */
    if(ulTaskNotifyTake(pdTRUE, NO_TIMEOUT) == pdFALSE) //clear Notification on read, move on immediately if there is no Notifications  
    {
        LOG("\n=== System Statistics ===\n");
        LOG("Available Heap Space (sum of free blocks): %u bytes\n",      stats.heap_stats.xAvailableHeapSpaceInBytes);
        LOG("Size of Largest Free Block: %u bytes\n",                     stats.heap_stats.xSizeOfLargestFreeBlockInBytes);
        LOG("Size of Smallest Free Block: %u bytes\n",                    stats.heap_stats.xSizeOfSmallestFreeBlockInBytes);
        LOG("Number of Free Blocks: %u \n",                               stats.heap_stats.xNumberOfFreeBlocks);
        LOG("Minimum amount of total free memory since boot: %u bytes\n", stats.heap_stats.xMinimumEverFreeBytesRemaining);
        LOG("Number of successfull pvPortMalloc calls %u \n",             stats.heap_stats.xNumberOfSuccessfulAllocations);
        LOG("Number of successfull vPortFree calls %u \n",                stats.heap_stats.xNumberOfSuccessfulFrees);

        LOG("\n=== Task Statistics ===\n");
        LOG("Number of Tasks: %u\n", stats.currentNumOfTasks);
        LOG("Name\t\tState\tPrio\tRemainingStack\tTaskNum\n");

        for (UBaseType_t task_num = 0; task_num < populatedArraySize; task_num++) 
        {
            char taskState = 'X';
            /* Cast the numeric taskState to a char representation */
            switch (taskStatusArray[task_num].eCurrentState) 
            {
                case eRunning:   taskState = 'R'; break;
                case eReady:     taskState = 'r'; break;
                case eBlocked:   taskState = 'B'; break;
                case eSuspended: taskState = 'S'; break;
                case eDeleted:   taskState = 'D'; break;
            }
            
            /* Print all the relevant stats for the given Task */
            LOG("%-16s%c\t%u\t%u\t\t%u\n",
                    taskStatusArray[task_num].pcTaskName,
                    taskState,
                    taskStatusArray[task_num].uxCurrentPriority,
                    taskStatusArray[task_num].usStackHighWaterMark,
                    taskStatusArray[task_num].xTaskNumber);
        }
        
        LOG("\n");
    }
    else /* Notification was received - send Monitor data over TCP */
    {
        char buffer[256];

        snprintf(buffer, sizeof(buffer), "\n=== System Statistics ===\n");
        tcp_client_send(buffer, strlen(buffer));

        snprintf(buffer, sizeof(buffer), "Available Heap Space (sum of free blocks): %u bytes\n", stats.heap_stats.xAvailableHeapSpaceInBytes);
        tcp_client_send(buffer, strlen(buffer));

        snprintf(buffer, sizeof(buffer), "Size of Largest Free Block: %u bytes\n", stats.heap_stats.xSizeOfLargestFreeBlockInBytes);
        tcp_client_send(buffer, strlen(buffer));

        snprintf(buffer, sizeof(buffer), "Size of Smallest Free Block: %u bytes\n", stats.heap_stats.xSizeOfSmallestFreeBlockInBytes);
        tcp_client_send(buffer, strlen(buffer));

        snprintf(buffer, sizeof(buffer), "Number of Free Blocks: %u\n", stats.heap_stats.xNumberOfFreeBlocks);
        tcp_client_send(buffer, strlen(buffer));

        snprintf(buffer, sizeof(buffer), "Minimum amount of total free memory since boot: %u bytes\n", stats.heap_stats.xMinimumEverFreeBytesRemaining);
        tcp_client_send(buffer, strlen(buffer));

        snprintf(buffer, sizeof(buffer), "Number of successful pvPortMalloc calls: %u\n", stats.heap_stats.xNumberOfSuccessfulAllocations);
        tcp_client_send(buffer, strlen(buffer));

        snprintf(buffer, sizeof(buffer), "Number of successful vPortFree calls: %u\n", stats.heap_stats.xNumberOfSuccessfulFrees);
        tcp_client_send(buffer, strlen(buffer));

        snprintf(buffer, sizeof(buffer), "\n=== Task Statistics ===\n");
        tcp_client_send(buffer, strlen(buffer));

        snprintf(buffer, sizeof(buffer), "Number of Tasks: %u\n", stats.currentNumOfTasks);
        tcp_client_send(buffer, strlen(buffer));

        snprintf(buffer, sizeof(buffer), "Name\t\tState\tPrio\tRemainingStack\tTaskNum\n");
        tcp_client_send(buffer, strlen(buffer));

        for (UBaseType_t task_num = 0; task_num < populatedArraySize; task_num++) 
        {
            char taskState = 'X';
            /* Cast the numeric taskState to a char representation */
            switch (taskStatusArray[task_num].eCurrentState) 
            {
                case eRunning:   taskState = 'R'; break;
                case eReady:     taskState = 'r'; break;
                case eBlocked:   taskState = 'B'; break;
                case eSuspended: taskState = 'S'; break;
                case eDeleted:   taskState = 'D'; break;
            }
            
            /* Format and send task statistics */
            snprintf(buffer, sizeof(buffer), "%-16s%c\t%u\t%u\t\t%u\n",
                    taskStatusArray[task_num].pcTaskName,
                    taskState,
                    taskStatusArray[task_num].uxCurrentPriority,
                    taskStatusArray[task_num].usStackHighWaterMark,
                    taskStatusArray[task_num].xTaskNumber);
            tcp_client_send(buffer, strlen(buffer));
        }

        snprintf(buffer, sizeof(buffer), "\n");
        tcp_client_send(buffer, strlen(buffer));
    }
}

/* 
 * Function: Monitor_initRuntimeCounter
 * 
 * Description: Function provided for portCONFIGURE_TIMER_FOR_RUN_TIME_STATS macro in FreeConfig.h
 * Gets and saves the initial value of the System Timer for reference as the 'start' time.
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 *
 */
void Monitor_initRuntimeCounter(void) 
{
    /* Get the start value of the timer */
    stats_start_time = get_absolute_time(); //returns the absolute time (now) of Pico's System Timer (hardware timer)
}

/* 
 * Function: Monitor_getRuntimeCounter
 * 
 * Description: Function provided for portGET_RUN_TIME_COUNTER_VALUE macro in FreeConfig.h
 * Returns the time since start based on the current value of System Timer and the 'start' value
 * Parameters:
 *   - none
 * 
 * Returns: unsigned long (microseconds since start)
 *
 */
unsigned long Monitor_getRuntimeCounter(void) 
{
    absolute_time_t current_time = get_absolute_time(); //returns the absolute time (now) of Pico's System Timer (hardware timer)
    return absolute_time_diff_us(stats_start_time, current_time);
}

/*******************************************************************************/
/*                          STATIC FUNCTION DEFINITIONS                        */
/*******************************************************************************/

