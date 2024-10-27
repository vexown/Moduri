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
static void WiFi_ProcessCommand(uint8_t command);
static void WiFi_ListenState(void);
static void WiFi_ActiveState(void);
static void WiFi_InitCommunication(void);

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
    switch (CommState)
    {
        case LISTEN:
            WiFi_ListenState();
            break;
        case ACTIVE_SEND_AND_RECEIVE:
            WiFi_ActiveState();
            break;
        default: /* INIT */
            WiFi_InitCommunication();
            break;
    }
}

/*******************************************************************************/
/*                          STATIC FUNCTION DEFINITIONS                        */
/*******************************************************************************/

/* 
 * Function: WiFi_ProcessCommand
 * 
 * Description: Processes commands received via TCP or UDP communication from 
 * the Central App. Note - for Pico to process the command correctly, send them 
 * from the Central Application in the following format: "cmd:X" where x = 0..255
 * 
 * Parameters:
 *   - uint8_t command: 0..255 command number (mapped in WiFi_Common.h to a macro)
 * 
 * Returns: void
 *
 */
static void WiFi_ProcessCommand(uint8_t command)
{
    printf("Processing command nr %u... \n", command);

    switch (command)
    {
        case PICO_DO_NOTHING:
            /* do nothing */
            break;
        case PICO_TRANSITION_TO_ACTIVE_MODE:
            printf("Transitioning to Active Mode...\n");
            CommState = ACTIVE_SEND_AND_RECEIVE;
            break;
        case PICO_TRANSITION_TO_LISTEN_MODE:
            printf("Transitioning to Listen Mode...\n");
            CommState = LISTEN;
            break;
        default:
            printf("Command not supported \n");
            break;
    }
}

/* 
 * Function: WiFi_ListenState
 * 
 * Description: State in which the Pico W is simply listening to the TCP/IP comms.
 * The only way to exit this passive state is to use PICO_TRANSITION_TO_ACTIVE_MODE
 * command which transitions the Pico to the active state where it can send messages
 * and process various commands.
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 *
 */
static void WiFi_ListenState(void)
{
    uint8_t received_command = PICO_DO_NOTHING;

    if(TransportLayer == TCP_COMMUNICATION)
    {
        tcp_client_process_recv_message(&received_command);
        if(received_command == PICO_TRANSITION_TO_ACTIVE_MODE) WiFi_ProcessCommand(received_command); 
    }
    else if(TransportLayer == UDP_COMMUNICATION)
    {
        udp_client_process_recv_message(&received_command);
        if(received_command == PICO_TRANSITION_TO_ACTIVE_MODE) WiFi_ProcessCommand(received_command); 
    }
    else
    {
        printf("Communication Type not supported\n");
    }
}

/* 
 * Function: WiFi_ActiveState
 * 
 * Description: State in which the Pico W can both receive/send messages and process 
 * various commands. This state can be achieved by sending PICO_TRANSITION_TO_ACTIVE_MODE 
 * command to take Pico out of the passive LISTEN state.
 * 
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 *
 */
static void WiFi_ActiveState(void)
{
    uint8_t received_command = PICO_DO_NOTHING;
    const char *message = "Yo from Pico W!";

    if(TransportLayer == TCP_COMMUNICATION)
    {
        /************** RX **************/
        /* Handle any received messages */
        tcp_client_process_recv_message(&received_command);
        WiFi_ProcessCommand(received_command); 

         /************** TX **************/
        /* Send a message (for now just a test message) */
        tcp_client_send(message, strlen(message));
    }
    else if(TransportLayer == UDP_COMMUNICATION)
    {
        /************** RX **************/
        /* Handle any received messages */
        udp_client_process_recv_message(&received_command);
        WiFi_ProcessCommand(received_command); 

        /************** TX **************/
        /* Send a message (for now just a test message) */
        udp_client_send(message);
    }
    else
    {
        printf("Communication Type not supported\n");
    }
}

/* 
 * Function: WiFi_InitCommunication
 * 
 * Description: Initializes the WiFi communication as:
 * - Pico as TCP server (requires PICO_AS_TCP_SERVER macro to be defined)
 * - Pico as TCP client (TransportLayer set to TCP_COMMUNICATION)
 * - Pico as UDP server (currently not supported)
 * - Pico as UDP client (TransportLayer set to UDP_COMMUNICATION)
 * 
 * After initialization the default state of communication is LISTEN.
 * This means the Pico W will passively listen to incoming messages.
 * In this mode it will only print the received messages, and react
 * only to one command - PICO_TRANSITION_TO_ACTIVE_MODE, which transitions
 * the Pico to an active mode.
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 *
 */
static void WiFi_InitCommunication(void)
{
    if(TransportLayer == TCP_COMMUNICATION)
    {
#ifdef PICO_AS_TCP_SERVER
        if(start_TCP_server() == true) CommState = LISTEN;
#else /* defaults to Pico as TCP client */
        if(start_TCP_client() == true) CommState = LISTEN;
#endif
    }
    else if(TransportLayer == UDP_COMMUNICATION)
    {
        if(start_UDP_client() == true) CommState = LISTEN;
    }
    else
    {
        printf("Communication Type not supported \n");
    }
}





