/********************************* SYSTEM *************************************/
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

/* WiFi includes */
#include "pico/cyw43_arch.h"
#include "lwip/sockets.h"
#include "WiFi_Credentials.h"
#include "WiFi_Transmit.h"

#ifndef WIFI_CREDENTIALS_PROVIDED
#error "Create WiFi_Credentials.h with your WiFi login and password as char* variables called ssid and pass. Define WIFI_CREDENTIALS_PROVIDED there to pass this check"                                                       
#endif

/* WiFi macros */
#define WIFI_CONNECTION_TIMEOUT_MS 10000 

/* Priorities for the tasks */
#define mainQUEUE_RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	mainQUEUE_SEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define NETWORK_TASK_PRIORITY 				( tskIDLE_PRIORITY + 3 )

/* Task periods */
#define NETWORK_TASK_PERIOD_MS				( 5000 )

/* The rate at which data is sent to the queue. The rate is once every mainQUEUE_SEND_FREQUENCY_MS (once every 1000ms by default) */
#define mainQUEUE_SEND_FREQUENCY_MS			( 1000 / portTICK_PERIOD_MS )

/* The number of items the queue can hold */
#define mainQUEUE_LENGTH					( 1 )

/* Stack sizes */
#define STACK_1024_BYTES					(configSTACK_DEPTH_TYPE)( 1024 )

/*-----------------------------------------------------------*/

void OS_start( void );

/*-----------------------------------------------------------*/

/* Tasks declarations */
static void prvQueueReceiveTask( void *pvParameters );
static void prvQueueSendTask( void *pvParameters );
static void networkTask(__unused void *params);

/*-----------------------------------------------------------*/

/* The queue instance */
static QueueHandle_t xQueue = NULL;

/*-----------------------------------------------------------*/

void OS_start( void )
{
	TaskHandle_t task;
	const char *rtos_type;

    /* Check if we're running FreeRTOS on single core or both RP2040 cores */
    /* - Standard FreeRTOS is designed for single-core systems, with simpler task 
       scheduling and communication mechanisms.
       - FreeRTOS SMP is an enhanced version for multi-core systems, allowing tasks to run 
       concurrently across multiple cores */
#if ( configNUMBER_OF_CORES > 1 )
    rtos_type = "FreeRTOS SMP";
#else
    rtos_type = "FreeRTOS";
#endif

#if ( configNUMBER_OF_CORES == 2 )
    printf("Starting %s on both cores\n", rtos_type);
#else
	printf("Starting %s on one of the cores\n", rtos_type);
#endif

	printf("Setting up the RTOS configuration... \n");
    /* Create the queue. */
	xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( uint32_t ) );

	if( xQueue != NULL )
	{
		/* Create the tasks */
		xTaskCreate( prvQueueReceiveTask,				/* The function that implements the task. */
					"Rx", 								/* The text name assigned to the task - for debug only as it is not used by the kernel. */
					STACK_1024_BYTES,		 			/* The size of the stack to allocate to the task. */
					NULL, 								/* The parameter passed to the task - not used in this case. */
					mainQUEUE_RECEIVE_TASK_PRIORITY, 	/* The priority assigned to the task. */
					NULL );								/* The task handle is not required, so NULL is passed. */

		xTaskCreate( prvQueueSendTask, "TX", STACK_1024_BYTES, NULL, mainQUEUE_SEND_TASK_PRIORITY, NULL );
    	xTaskCreate( networkTask, "NET", STACK_1024_BYTES , NULL, NETWORK_TASK_PRIORITY , &task);

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

static void networkTask(__unused void *params)
{
    /* Initializes the cyw43_driver code and initializes the lwIP stack (for use in specific country) */
    if (cyw43_arch_init()) 
    {
        printf("failed to initialize\n");
    } 
    else 
    {
        printf("initialized successfully\n");
    }

    /* Enables Wi-Fi in Station (STA) mode such that connections can be made to other Wi-Fi Access Points */
    cyw43_arch_enable_sta_mode();

    /* Attempt to connect to a wireless access point.
       Blocking until the network is joined, a failure is detected or a timeout occurs */
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECTION_TIMEOUT_MS)) 
    {
        printf("failed to connect\n");
    }
    else
    {
        printf("connected sucessfully\n");
    }

    for( ;; )
	{
        vTaskDelay(NETWORK_TASK_PERIOD_MS);

		send_message_TCP("Yo from Pico W!");
    }
}

static void prvQueueSendTask( void *pvParameters )
{
	TickType_t xNextWakeTime;
	const unsigned long ulValueToSend = 100UL;

	/* Remove compiler warning about unused parameter. */
	( void ) pvParameters;

	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for( ;; )
	{
		/* Place this task in the blocked state until it is time to run again. */
		int32_t SendToQueue_freq_ms = mainQUEUE_SEND_FREQUENCY_MS/2;
		vTaskDelayUntil( &xNextWakeTime, SendToQueue_freq_ms );
		printf("SendToQueue_freq_ms = %u ms\n", SendToQueue_freq_ms);
		
		/* Send to the queue - causing the queue receive task to unblock and
		toggle the LED.  0 is used as the block time so the sending operation
		will not block - it shouldn't need to block as the queue should always
		be empty at this point in the code. */
		xQueueSend( xQueue, &ulValueToSend, 0U );
	}
}

static void prvQueueReceiveTask( void *pvParameters )
{
	unsigned long ulReceivedValue;
	const unsigned long ulExpectedValue = 100UL;

	static int state_LED;

	/* Remove compiler warning about unused parameter. */
	( void ) pvParameters;

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






