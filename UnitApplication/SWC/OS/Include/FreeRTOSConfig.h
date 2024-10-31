/*
 * FreeRTOS V202107.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */
 
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

extern void Monitor_initRuntimeCounter(void);
extern unsigned long Monitor_getRuntimeCounter(void);

/* Scheduler Related */

/* Set to 1 to use preemption. This allows higher priority tasks to 
   preempt lower priority tasks. Set to 0 for cooperative scheduling. */
#define configUSE_PREEMPTION                    1

/* Set to 1 to enable tickless idle mode, which can save power. 
   Set to 0 to use the standard tick-based idle mode. */
#define configUSE_TICKLESS_IDLE                 0

/* Set to 1 to use the idle hook function, if defined. */
#define configUSE_IDLE_HOOK                     0

/* Set to 1 to use the tick hook function, if defined. */
#define configUSE_TICK_HOOK                     0

/* Tick rate in Hertz. Determines the frequency of the system tick interrupt. 
   1000 Hz means a tick every 1 millisecond. */
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )

/* Maximum number of task priorities. Adjust based on application needs. */
#define configMAX_PRIORITIES                    32

/* Minimum stack size in words. This value should be chosen based on 
   the task's stack usage. */
#define configMINIMAL_STACK_SIZE                ( configSTACK_DEPTH_TYPE ) 512

/* Set to 1 if the tick count is to be represented as a 16-bit value.
   Set to 0 for 32-bit tick count representation. */
#define configUSE_16_BIT_TICKS                  0

/* Set to 1 to yield the CPU to lower priority tasks when the idle task is running. */
#define configIDLE_SHOULD_YIELD                 1

/* Synchronization Related */

/* Set to 1 to enable mutexes for task synchronization. */
#define configUSE_MUTEXES                       1

/* Set to 1 to enable recursive mutexes. */
#define configUSE_RECURSIVE_MUTEXES             1

/* Set to 1 to use task tags for application-specific task identification. */
#define configUSE_APPLICATION_TASK_TAG          0

/* Set to 1 to enable counting semaphores. */
#define configUSE_COUNTING_SEMAPHORES           1

/* Size of the queue registry. This is used to keep track of created queues. */
#define configQUEUE_REGISTRY_SIZE               8

/* Set to 1 to enable queue sets. A queue set is a collection of queues. */
#define configUSE_QUEUE_SETS                    1

/* Set to 1 to enable time slicing between tasks of equal priority. */
#define configUSE_TIME_SLICING                  1

/* Set to 1 to make Newlib thread-safe. */
#define configUSE_NEWLIB_REENTRANT              0

/* Set to 1 to enable backward compatibility features. */
#define configENABLE_BACKWARD_COMPATIBILITY     1

/* Number of thread-local storage pointers available. */
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

/* System */

/* Type used to represent stack depth in the system. */
#define configSTACK_DEPTH_TYPE                  uint32_t

/* Type used to represent message buffer lengths. */
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t

/* Memory allocation related definitions. */

/* Set to 1 to support static memory allocation. */
#define configSUPPORT_STATIC_ALLOCATION         0

/* Set to 1 to support dynamic memory allocation using heap_4.c or heap_5.c. */
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* Total heap size available for dynamic allocation. */
#define configTOTAL_HEAP_SIZE                   (128*1024)

/* Set to 1 if the application provides its own heap. */
#define configAPPLICATION_ALLOCATED_HEAP        0

/* Hook function related definitions. */

/* Set to 1 to enable stack overflow checking. 
   Value of 2 provides detailed stack overflow checking. */
#define configCHECK_FOR_STACK_OVERFLOW          0

/* Set to 1 to enable a hook function that is called if malloc fails. */
#define configUSE_MALLOC_FAILED_HOOK            0

/* Set to 1 to enable a hook function that is called during the start-up of daemon tasks. */
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* Run time and task stats gathering related definitions. */

/* Set to 1 to generate run time statistics. */
#define configGENERATE_RUN_TIME_STATS              1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()   Monitor_initRuntimeCounter()
#define portGET_RUN_TIME_COUNTER_VALUE()           Monitor_getRuntimeCounter()

/* Set to 1 to enable tracing facilities. */
#define configUSE_TRACE_FACILITY                1

/* Set to 1 to enable task and queue statistics formatting functions. */
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* Co-routine related definitions. */

/* Set to 1 to enable co-routines. */
#define configUSE_CO_ROUTINES                   0

/* Maximum number of priorities for co-routines. */
#define configMAX_CO_ROUTINE_PRIORITIES         1

/* Software timer related definitions. */

/* Set to 1 to enable software timers. */
#define configUSE_TIMERS                        1

/* Priority of the timer task. This should be lower than the highest priority task. */
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )

/* Number of timer queues. */
#define configTIMER_QUEUE_LENGTH                10

/* Stack depth for the timer task. */
#define configTIMER_TASK_STACK_DEPTH            1024

/* Interrupt nesting behaviour configuration. */

/* These settings are specific to your processor and application. You may need
   to define them based on your system's interrupt priorities. */
/*
#define configKERNEL_INTERRUPT_PRIORITY         [dependent on processor]
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    [dependent on processor and application]
#define configMAX_API_CALL_INTERRUPT_PRIORITY   [dependent on processor and application]
*/

/* SMP (Symmetric Multiprocessing) port only */

/* Set the number of processor cores. */
#define configNUMBER_OF_CORES                   2

/* Core that the tick interrupt is handled by. */
#define configTICK_CORE                         0

/* Set to 1 to run tasks with multiple priorities on SMP systems. */
#define configRUN_MULTIPLE_PRIORITIES           1

#if configNUMBER_OF_CORES > 1
#define configUSE_CORE_AFFINITY                 1
#endif
#define configUSE_PASSIVE_IDLE_HOOK             0

/* RP2040 specific settings */

/* Set to 1 to support synchronization interoperability with the Pico SDK. */
#define configSUPPORT_PICO_SYNC_INTEROP         1

/* Set to 1 to support time interoperability with the Pico SDK. */
#define configSUPPORT_PICO_TIME_INTEROP         1

/* Define to trap errors during development. */
#include <assert.h>
#define configASSERT(x)                         assert(x)

/* Set to 1 to include the API function, or 0 to exclude it. */

#define INCLUDE_vTaskPrioritySet                1  /* Includes vTaskPrioritySet() function. */
#define INCLUDE_uxTaskPriorityGet               1  /* Includes uxTaskPriorityGet() function. */
#define INCLUDE_vTaskDelete                     1  /* Includes vTaskDelete() function. */
#define INCLUDE_vTaskSuspend                    1  /* Includes vTaskSuspend() function. */
#define INCLUDE_vTaskDelayUntil                 1  /* Includes vTaskDelayUntil() function. */
#define INCLUDE_vTaskDelay                      1  /* Includes vTaskDelay() function. */
#define INCLUDE_xTaskGetSchedulerState          1  /* Includes xTaskGetSchedulerState() function. */
#define INCLUDE_xTaskGetCurrentTaskHandle       1  /* Includes xTaskGetCurrentTaskHandle() function. */
#define INCLUDE_uxTaskGetStackHighWaterMark     1  /* Includes uxTaskGetStackHighWaterMark() function. */
#define INCLUDE_xTaskGetIdleTaskHandle          1  /* Includes xTaskGetIdleTaskHandle() function. */
#define INCLUDE_eTaskGetState                   1  /* Includes eTaskGetState() function. */
#define INCLUDE_xTimerPendFunctionCall          1  /* Includes xTimerPendFunctionCall() function. */
#define INCLUDE_xTaskAbortDelay                 1  /* Includes xTaskAbortDelay() function. */
#define INCLUDE_xTaskGetHandle                  1  /* Includes xTaskGetHandle() function. */
#define INCLUDE_xTaskResumeFromISR              1  /* Includes xTaskResumeFromISR() function. */
#define INCLUDE_xQueueGetMutexHolder            1  /* Includes xQueueGetMutexHolder() function. */

/* A header file that defines trace macros can be included here. */

#endif /* FREERTOS_CONFIG_H */
