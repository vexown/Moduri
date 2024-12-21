/**
 * File: OS_manager.c
 * Description: High-level FreeRTOS configuration. Setup of tasks and other 
 * system-related mechanisms 
 */

/* FreeRTOS V202107.00
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
 *******************************************************************************/

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/
/* Standard includes. */
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"

/* SDK includes */
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include "lwip/sockets.h"

/* WiFi includes */
#include "WiFi_UDP.h"
#include "WiFi_TCP.h"
#include "WiFi_Common.h"

/* Monitor includes */
#include "Monitor_Common.h"

/* OS includes */
#include "OS_manager.h"

/* Misc includes */
#include "Common.h"

/*******************************************************************************/
/*                                 MACROS                                      */
/*******************************************************************************/

/* Priorities for the tasks (bigger number = higher prio) */
#define MONITOR_TASK_PRIORITY				(tskIDLE_PRIORITY + 1)
#define NETWORK_TASK_PRIORITY 				(tskIDLE_PRIORITY + 2)

/* Context for time-related macros, we use these two main helper macros from FreeRTOS (configTICK_RATE_HZ is defined in FreeRTOSConfig.h)
	- pdMS_TO_TICKS(TimeInMs):     (TimeInMs * configTICK_RATE_HZ) / 1000
	- pdTICKS_TO_MS(TimeInTicks):  (TimeInTicks * 1000 ) / (configTICK_RATE_HZ)
*/
/* Task periods */
#define NETWORK_TASK_PERIOD_TICKS			pdMS_TO_TICKS(5000)  //5s
#define MONITOR_TASK_PERIOD_TICKS			pdMS_TO_TICKS(11000) //11s

/* Timer periods */
#define ALIVE_TIMER_PERIOD                  pdMS_TO_TICKS(500) //500ms

/* Stack sizes - This parameter is in WORDS (on Pico W: 1 word = 32bit = 4bytes) */ 
#define STACK_1024_BYTES					(configSTACK_DEPTH_TYPE)(256) 
#define STACK_2048_BYTES					(configSTACK_DEPTH_TYPE)(512) 

/*******************************************************************************/
/*                               DATA TYPES                                    */
/*******************************************************************************/


/*******************************************************************************/
/*                        GLOBAL FUNCTION DECLARATIONS                         */
/*******************************************************************************/
void OS_start(void);

/*******************************************************************************/
/*                       STATIC FUNCTION DECLARATIONS                          */
/*******************************************************************************/

/***************************** Tasks declarations ******************************/
static void networkTask(__unused void *taskParams);
static void monitorTask(__unused void *taskParams);

/******************************* Timer callbacks *******************************/
static void aliveTimerCallback(TimerHandle_t xTimer);

/*******************************************************************************/
/*                             STATIC VARIABLES                                */
/*******************************************************************************/

/*******************************************************************************/
/*                             GLOBAL VARIABLES                                */
/*******************************************************************************/
TaskHandle_t monitorTaskHandle = NULL;

/*******************************************************************************/
/*                          GLOBAL FUNCTION DEFINITIONS                        */
/*******************************************************************************/
/* 
 * Function: OS_start
 * 
 * Description: Global function called from main.c. Initial OS setup and starts scheduler
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 */
void OS_start( void )
{
	const char *rtos_type;
	BaseType_t taskCreationStatus[NUM_OF_TASKS_TO_CREATE];
	TimerHandle_t aliveTimer;
	BaseType_t aliveTimerStarted;

    /** Check if we're running FreeRTOS on single core or both RP2040 cores:
      * - Standard FreeRTOS is designed for single-core systems, with simpler task scheduling and communication mechanisms.
	  * - FreeRTOS SMP is an enhanced version for multi-core systems, allowing tasks to run concurrently across multiple cores */
#if (configNUMBER_OF_CORES == 2)
 	rtos_type = "FreeRTOS SMP";
    LOG("Running %s on both cores \n", rtos_type);
#else
	rtos_type = "FreeRTOS";
	LOG("Running %s on one core \n", rtos_type);
#endif

	LOG("Setting up the RTOS configuration... \n");

	/* Create the Software Timers. These use System Tick as time base. They are handled in a dedicated Timer Service task (aka Daemon Task) */
	/* "Alive" Timer which is used to toggle the on-board LED to show the system is not stuck (is alive) */
    aliveTimer = xTimerCreate( "AliveTimer",                     // Timer name (for debugging)
								ALIVE_TIMER_PERIOD,    			 // Timer period in ticks
								pdTRUE,                          // Is it Auto-reload timer (one-shot timer if pdFALSE)
								0,                               // Timer ID (optional user data, not used here)
								aliveTimerCallback);     		 // Callback function (executes when timer expires)

	/* If all FreeRTOS objects were created sucessfully proceed with task creation and starting the scheduler */
	if(aliveTimer != NULL)
	{
		/* Create the tasks */
		taskCreationStatus[0] = xTaskCreate( networkTask,				/* The function that implements the task. */
											"Network", 					/* The text name assigned to the task - for debug only as it is not used by the kernel. */
											STACK_2048_BYTES,		 	/* The size of the stack to allocate to the task (in words) */
											NULL, 						/* The parameter passed to the task - not used in this case. */
											NETWORK_TASK_PRIORITY, 		/* The priority assigned to the task. */
											NULL );						/* The task handle, if not needed put NULL */
		taskCreationStatus[1] = xTaskCreate( monitorTask, "Monitor", STACK_2048_BYTES, NULL, MONITOR_TASK_PRIORITY , &monitorTaskHandle);

		/* Check if the tasks were created successfully */
		for(uint8_t i = 0; i < NUM_OF_TASKS_TO_CREATE; i++)
		{
			if(taskCreationStatus[i] == pdPASS)
			{
				LOG("Task number %u created successfully \n", i);
			}
			else /* pdFAIL */
			{
				CriticalErrorHandler(MODULE_ID_OS, ERROR_ID_TASK_FAILED_TO_CREATE);
			}
		}

		/* Start the Alive Timer */
		aliveTimerStarted = xTimerStart(aliveTimer, NO_TIMEOUT); //don't wait for space in the timer command queue, if there is no space we throw an error

		/* Check if the Software Timers have successfully started */
		if(aliveTimerStarted == pdPASS)
		{
			LOG("Alive Timer started successfully \n");
		}
		else /* pdFAIL */
		{
			CriticalErrorHandler(MODULE_ID_OS, ERROR_ID_SW_TIMER_FAILED_TO_START);
		}

		LOG("RTOS configuration finished, starting the scheduler... \n");
		vTaskStartScheduler();
	}
	else
	{
		CriticalErrorHandler(MODULE_ID_OS, ERROR_ID_RTOS_OBJECTS_FAILED_TO_CREATE);
	}

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the Idle and/or
	timer tasks to be created.  See the memory management section on the
	FreeRTOS web site for more details on the FreeRTOS heap
	http://www.freertos.org/a00111.html. */
	for( ;; );
}

/*******************************************************************************/
/*                            TASK FUNCTION DEFINITIONS                        */
/*******************************************************************************/

/* 
 * Function: monitorTask
 * 
 * Description: Monitoring Task providing information about the system behavior and performance
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 */
static void monitorTask(__unused void *taskParams) 
{
	/*******************************************************************************/
	/*                          Task Initialization Code                           */
	/*******************************************************************************/
	TickType_t xLastWakeTime;

	/* Initialize xLastWakeTime - this only needs to be done once. */
	xLastWakeTime = xTaskGetTickCount();
	/*******************************************************************************/
	/*                               Task Loop Code                                */
	/*******************************************************************************/
    for( ;; )
	{
		vTaskDelayUntil(&xLastWakeTime, MONITOR_TASK_PERIOD_TICKS); // Execute periodically at consistent intervals based on a reference time

		Monitor_MainFunction();
    }
}

/* 
 * Function: networkTask
 * 
 * Description: Network task handling the Wi-Fi communication (receive/send TCP and UDP)
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 */
static void networkTask(__unused void *taskParams)
{
	/*******************************************************************************/
	/*                          Task Initialization Code                           */
	/*******************************************************************************/
	TickType_t xLastWakeTime;

	if(setupWifiAccessPoint() == false)
	{
		CriticalErrorHandler(MODULE_ID_OS, ERROR_ID_WIFI_DID_NOT_CONNECT);
	}

	/* Initialize xLastWakeTime - this only needs to be done once. */
	xLastWakeTime = xTaskGetTickCount();

	/*******************************************************************************/
	/*                               Task Loop Code                                */
	/*******************************************************************************/
    for( ;; )
	{
        vTaskDelayUntil(&xLastWakeTime, NETWORK_TASK_PERIOD_TICKS); /* Execute periodically at consistent intervals based on a reference time */

		WiFi_MainFunction();
	}

}

/* 
 * Function: aliveTimerCallback
 * 
 * Description: Callback function for the Alive Timer, executed every ALIVE_TIMER_PERIOD 
 * 
 * Parameters:
 *   - xTimer (handle to the Alive Timer)
 * 
 * Returns: void
 */
static void aliveTimerCallback(TimerHandle_t xTimer)
{
	static bool state_LED;

	(void)xTimer; // Timer handle not used in the callback currently

	/* Toggle the LED State */
	state_LED = !state_LED;
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state_LED); //Set a GPIO pin on the wireless chip to a given value (LED pin in this case) 
}







