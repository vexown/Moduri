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
/*                          STATIC FUNCTION DECLARATIONS                       */
/*******************************************************************************/
void WiFi_ProcessCommand(uint8_t command);

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
    const char *message = "Yo from Pico W!";
    uint8_t received_command = PICO_DO_NOTHING;

    if(CommState == LISTEN)
    {
        tcp_client_process_recv_message(&received_command);
        /* Note - for Pico to process the command send them from the Central Application in the following format: "cmd:X" where x = 0..255 */
        if(received_command != PICO_DO_NOTHING) WiFi_ProcessCommand(received_command); 
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
            if(start_TCP_client() == true) CommState = LISTEN;
#endif
        }
    }
}

/*******************************************************************************/
/*                          STATIC FUNCTION DEFINITIONS                        */
/*******************************************************************************/

void WiFi_ProcessCommand(uint8_t command)
{
    printf("Processing command nr %u... \n", command);
}




