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

/* Misc includes */
#include "Common.h"

/*******************************************************************************/
/*                                 MACROS                                      */
/*******************************************************************************/

/* Priorities for the tasks */
#define mainQUEUE_RECEIVE_TASK_PRIORITY		(tskIDLE_PRIORITY + 2)
#define	mainQUEUE_SEND_TASK_PRIORITY		(tskIDLE_PRIORITY + 1)
#define NETWORK_TASK_PRIORITY 				(tskIDLE_PRIORITY + 3)

/* Task periods: portTICK_PERIOD_MS = (1/configTICK_RATE_HZ) * 1000 = 1ms per tick */
#define QUEUE_SEND_TASK_PERIOD_TICKS		(TickType_t)(500 / portTICK_PERIOD_MS) //500ms
#define NETWORK_TASK_PERIOD_TICKS			(TickType_t)(5000 / portTICK_PERIOD_MS) //5s

/* Other Task-related macros */
#define NUM_OF_TASKS 						(3)

/* Queue configuration */
#define mainQUEUE_LENGTH					(1)

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
/*                         TASK FUNCTION DECLARATIONS                          */
/*******************************************************************************/

/***************************** Tasks declarations ******************************/
static void queueReceiveTask(__unused void *taskParams );
static void queueSendTask(__unused void *taskParams );
static void networkTask(__unused void *taskParams);

/*******************************************************************************/
/*                             STATIC VARIABLES                                */
/*******************************************************************************/

/* The queue instance */
static QueueHandle_t xQueue = NULL;

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
	BaseType_t taskCreationStatus[NUM_OF_TASKS];

    /** Check if we're running FreeRTOS on single core or both RP2040 cores:
      * - Standard FreeRTOS is designed for single-core systems, with simpler task scheduling and communication mechanisms.
	  * - FreeRTOS SMP is an enhanced version for multi-core systems, allowing tasks to run concurrently across multiple cores */
#if (configNUMBER_OF_CORES == 2)
 	rtos_type = "FreeRTOS SMP";
    printf("Running %s on both cores \n", rtos_type);
#else
	rtos_type = "FreeRTOS";
	printf("Running %s on one core \n", rtos_type);
#endif

	printf("Setting up the RTOS configuration... \n");

    /* Create a queue that can hold mainQUEUE_LENGTH number of items, each of which is sizeof(uint32_t) bytes in size (4bytes) */
	xQueue = xQueueCreate(mainQUEUE_LENGTH, sizeof(uint32_t)); // 1 x uint32_t queue

	if( xQueue != NULL )
	{
		/* Create the tasks */
		taskCreationStatus[0] = xTaskCreate( queueReceiveTask,					/* The function that implements the task. */
											"RX", 								/* The text name assigned to the task - for debug only as it is not used by the kernel. */
											STACK_1024_BYTES,		 			/* The size of the stack to allocate to the task. */
											NULL, 								/* The parameter passed to the task - not used in this case. */
											mainQUEUE_RECEIVE_TASK_PRIORITY, 	/* The priority assigned to the task. */
											NULL );								/* The task handle is not required, so NULL is passed. */
		taskCreationStatus[1] = xTaskCreate( queueSendTask, "TX", STACK_1024_BYTES, NULL, mainQUEUE_SEND_TASK_PRIORITY, NULL );
    	taskCreationStatus[2] = xTaskCreate( networkTask, "NET", STACK_2048_BYTES , NULL, NETWORK_TASK_PRIORITY , NULL);

		/* Check if the tasks were created successfully */
		for(uint8_t i = 0; i < NUM_OF_TASKS; i++)
		{
			if(taskCreationStatus[i] == pdPASS)
			{
				printf("Task number %u created successfully \n", i);
			}
			else /* pdFAIL */
			{
				CriticalErrorHandler(MODULE_ID_OS, ERROR_ID_TASK_FAILED_TO_CREATE);
			}
		}

		/* Start the tasks and timer running. */
		printf("RTOS configuration finished, starting the scheduler... \n");
		vTaskStartScheduler();
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

	if(connectToWifi() == false)
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
 * Function: queueSendTask
 * 
 * Description: Demo task for sending data to a queue
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 */
static void queueSendTask(__unused void *taskParams)
{
	/*******************************************************************************/
	/*                          Task Initialization Code                           */
	/*******************************************************************************/
	TickType_t xLastWakeTime;
	const unsigned long ulValueToSend = 100UL;

	/* Initialize xLastWakeTime - this only needs to be done once. */
	xLastWakeTime = xTaskGetTickCount();

	/*******************************************************************************/
	/*                               Task Loop Code                                */
	/*******************************************************************************/
	for( ;; )
	{
		/* Place this task in the blocked state until it is time to run again. */
		vTaskDelayUntil(&xLastWakeTime, QUEUE_SEND_TASK_PERIOD_TICKS); // Execute periodically at consistent intervals based on a reference time
		
		/* Send to the queue - causing the queue receive task to unblock and
		toggle the LED.  0 is used as the block time so the sending operation
		will not block - it shouldn't need to block as the queue should always
		be empty at this point in the code. */
		xQueueSend( xQueue, &ulValueToSend, 0U );
	}
}

/* 
 * Function: queueReceiveTask
 * 
 * Description: Demo task for receiving data from a queue (also flashes the on-board LED)
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 */
static void queueReceiveTask(__unused void *taskParams)
{
	/*******************************************************************************/
	/*                          Task Initialization Code                           */
	/*******************************************************************************/
	unsigned long ulReceivedValue;
	static int state_LED;
	const unsigned long ulExpectedValue = 100UL;

	/*******************************************************************************/
	/*                               Task Loop Code                                */
	/*******************************************************************************/
	for( ;; )
	{
		/* Wait until something arrives in the queue - this task will block
		indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
		FreeRTOSConfig.h. */
		xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );

		/*  To get here something must have been received from the queue, but
		is it the expected value?  If it is, perform task activities */
		if( ulReceivedValue == ulExpectedValue )
		{
			/* Change the LED State */
			state_LED = !state_LED;
			cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state_LED);

			/* Clear the variable so the next time this task runs a correct value needs to be supplied again */
			ulReceivedValue = 0U;
		}
	}
}







