/**
 * File: WiFi_main.c
 * Description: Implementation of the main WiFi function 
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/

/* Standard includes. */
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"

/* SDK includes */
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

/* WiFi includes */
#include "WiFi_UDP.h"
#include "WiFi_TCP.h"
#include "WiFi_Common.h"

/*******************************************************************************/
/*                                 MACROS                                      */
/*******************************************************************************/

/*******************************************************************************/
/*                               DATA TYPES                                    */
/*******************************************************************************/

/* Wifi Communication Types */
typedef enum {
    UDP_COMMUNICATION = 0xAA,
    TCP_COMMUNICATION = 0xAB
} TransportLayerType;

/* Communication State */
typedef enum {
    INIT = 0,
    LISTEN = 1,
    ACTIVE_SEND_AND_RECEIVE = 2
} CommStateType;

/*******************************************************************************/
/*                             STATIC VARIABLES                                */
/*******************************************************************************/

/* WiFi Communication Type */
static TransportLayerType TransportLayer = TCP_COMMUNICATION; /* default communication type is TCP */

/* Current Communication State */
static CommStateType CommState = INIT; 

/*******************************************************************************/
/*                          GLOBAL FUNCTION DEFINITIONS                        */
/*******************************************************************************/
/* 
 * Function: WiFi_MainFunction
 * 
 * Description: Main implementation of WiFi-related functionalities of the board.
 * It is periodically (every NETWORK_TASK_PERIOD_TICKS) executed from the WiFi 
 * FreeRTOS task
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 *
 */

void WiFi_MainFunction(void)
{
    char message_buffer[128] = {0};
    const char *message = "Yo from Pico W!";

    if(CommState == LISTEN)
    {
        return; /* Do nothing, will return and immedietaly block */
    }

    if(CommState == ACTIVE_SEND_AND_RECEIVE)
    {
        tcp_client_send(message, strlen(message));
    }

    if(CommState == INIT)
    {
        if(TransportLayer == TCP_COMMUNICATION)
        {
#ifdef PICO_AS_TCP_SERVER
            if(start_TCP_server() == true) CommState = LISTEN;
#else /* defaults to Pico as TCP client */
            if(start_TCP_client() == true) CommState = ACTIVE_SEND_AND_RECEIVE;
#endif
        }
    }
}






