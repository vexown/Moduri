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

/******************************************************************************
 *
 * This file implements the code that prepares the system, including the
 * hardware setup and standard FreeRTOS hook functions.
 *
 */


/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"

/* Library includes. */
#include <stdio.h>
#include "pico/stdlib.h"

/* WiFi includes */
#include "pico/cyw43_arch.h"
#include "WiFi_Credentials.h"

#ifndef WIFI_CREDENTIALS_PROVIDED
#error "Create WiFi_Credentials.h with your WiFi login and password as char* variables called ssid and pass. Define WIFI_CREDENTIALS_PROVIDED there to pass this check"                                                       
#endif

/*-----------------------------------------------------------*/

#define WIFI_CONNECTION_TIMEOUT_MS 10000 

/*-----------------------------------------------------------*/

/* Hardware setup function */
static void prvSetupHardware( void );
/* Main function which does some set up and starts the scheduler */
extern void blinky_main( void );

/* Prototypes for the standard FreeRTOS callback/hook functions implemented within this file. */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

/*-----------------------------------------------------------*/

int main( void )
{
    /* Configure and initialize Raspberry Pico W hardware */
    prvSetupHardware();

    /* Go to the main C file which does some setup and starts the scheduler */
    blinky_main();
}

/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
    stdio_init_all();

    /* Initializes the cyw43_driver code and initializes the lwIP stack (for use in specific country) */
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_POLAND)) 
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
    if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECTION_TIMEOUT_MS)) 
    {
        printf("failed to connect\n");
    }
    else
    {
        printf("connected sucessfully\n");
    }

}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

    /* Force an assert. */
    configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
    configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */

    /* Force an assert. */
    configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
    volatile size_t xFreeHeapSpace;

    /* This is just a trivial example of an idle hook.  It is called on each
    cycle of the idle task.  It must *NOT* attempt to block.  In this case the
    idle task just queries the amount of FreeRTOS heap that remains.  See the
    memory management section on the http://www.FreeRTOS.org web site for memory
    management options.  If there is a lot of heap memory free then the
    configTOTAL_HEAP_SIZE value in FreeRTOSConfig.h can be reduced to free up
    RAM. */
    xFreeHeapSpace = xPortGetFreeHeapSize();

    /* Remove compiler warning about xFreeHeapSpace being set but never used. */
    ( void ) xFreeHeapSpace;
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{

}
