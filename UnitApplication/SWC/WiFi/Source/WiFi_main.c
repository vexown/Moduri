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
#include "WiFi_Transmit.h"
#include "WiFi_Receive.h"
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

/* WiFi Direction Mode */
typedef enum {
    RECEIVE_MODE = 0,
    TRANSMIT_MODE = 1
} CommDirectionType;

/* Communication State */
typedef enum {
    ACTIVE = 0,
    PASSIVE_LISTEN = 1
} CommStateType;

/*******************************************************************************/
/*                             STATIC VARIABLES                                */
/*******************************************************************************/

/* WiFi Communication Type */
static TransportLayerType TransportLayer = TCP_COMMUNICATION; /* initial communication type is TCP */

/* WiFi Communication Direction */
static CommDirectionType CommDirection = RECEIVE_MODE; /* initial direction is receive */

/* Current Communication State */
static CommStateType CommState = ACTIVE; /* Active by default */

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

    if(CommState == PASSIVE_LISTEN)
    {
        return; /* Do nothing, will return and immedietaly block */
    }

    /* CommState ACTIVE */
    if(TransportLayer == UDP_COMMUNICATION)
    {
        if(CommDirection == RECEIVE_MODE)
        {
            receive_message_UDP(message_buffer, sizeof(message_buffer));
        }
        else if(CommDirection == TRANSMIT_MODE)
        {
            send_message_UDP("UDP yelling");
        }
        else
        {
            printf("Communication direction not supported. \n");
        }
    }
    else if(TransportLayer == TCP_COMMUNICATION)
    {
        if(CommDirection == RECEIVE_MODE)
        {
            if(start_TCP_server() == true) CommState = PASSIVE_LISTEN;
        }
        else if(CommDirection == TRANSMIT_MODE)
        {
            send_message_TCP("Yo from Pico W!");
        }
        else
        {
            printf("Communication direction not supported. \n");
        }
    }
    else
    {
        printf("Communication type not supported.");
    }
}






