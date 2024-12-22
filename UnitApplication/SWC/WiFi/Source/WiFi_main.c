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
#include "task.h"
#include "semphr.h"

/* SDK includes */
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"

/* WiFi includes */
#include "WiFi_UDP.h"
#include "WiFi_TCP.h"
#include "WiFi_Common.h"

/* OS includes */
#include "OS_manager.h"

/* Misc includes */
#include "Common.h"

/* HAL includes */
#include "GPIO_HAL.h"

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
    ACTIVE_SEND_AND_RECEIVE = 2,
    MONITOR = 3,
    LID_OPEN = 4,
    LID_CLOSED = 5,
} WiFiStateType;

/*******************************************************************************/
/*                             STATIC VARIABLES                                */
/*******************************************************************************/

/* WiFi Communication Type */
#ifndef PICO_AS_TCP_SERVER
static TransportLayerType TransportLayer = TCP_COMMUNICATION; /* select either TCP or UDP */
#else
static TransportLayerType TransportLayer = TCP_COMMUNICATION; /* Force TCP communication type when setting up Pico as TCP server */
#endif
/* Current Communication State */
static WiFiStateType WiFiState = INIT; 

/*******************************************************************************/
/*                          STATIC FUNCTION DECLARATIONS                       */
/*******************************************************************************/
static void WiFi_ProcessCommand(uint8_t command);
static void WiFi_ListenState(void);
static void WiFi_ActiveState(void);
static void WiFi_MonitorState(void);
static void WiFi_InitCommunication(void);
static void Wifi_LidOpen_State(void);
static void Wifi_LidClosed_State(void);

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
    switch (WiFiState)
    {
        case LISTEN:
            WiFi_ListenState();
            break;
        case ACTIVE_SEND_AND_RECEIVE:
            WiFi_ActiveState();
            break;
        case MONITOR:
            WiFi_MonitorState();
            break;
        case LID_OPEN:
            Wifi_LidOpen_State();
            break;
        case LID_CLOSED:
            Wifi_LidClosed_State();
            break;
        default: /* INIT */
            WiFi_InitCommunication();
            break;
    }
}

/* 
 * Function: end_switch_irq_handler
 * 
 * Description: Interrupt handler for the end switch GPIO pin.
 * 
 * Parameters:
 *   - gpio: GPIO pin number
 *   - events: Event type
 * 
 * Returns: void
 */
void end_switch_irq_handler(uint gpio, uint32_t events)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (gpio == GPIO_END_SWITCH)
    {
        // Notify the task
        vTaskNotifyGiveFromISR(networkTaskHandle, &xHigherPriorityTaskWoken);
    }

    // Force a context switch if needed
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
    LOG("Processing command nr %u... \n", command);

    switch (command)
    {
        case PICO_DO_NOTHING:
            /* do nothing */
            break;
        case PICO_TRANSITION_TO_ACTIVE_MODE:
            LOG("Transitioning to Active Mode...\n");
            WiFiState = ACTIVE_SEND_AND_RECEIVE;
            break;
        case PICO_TRANSITION_TO_LISTEN_MODE:
            LOG("Transitioning to Listen Mode...\n");
            WiFiState = LISTEN;
            break;
        case PICO_TRANSITION_TO_MONITOR_MODE:
            LOG("Transitioning to Monitor Mode...\n");
            WiFiState = MONITOR;
            break;
        case PICO_TRANSITION_TO_LID_OPEN_MODE:
            LOG("Transitioning to Lid Open Mode...\n");
            WiFiState = LID_OPEN;
            break;
        case PICO_TRANSITION_TO_LID_CLOSED_MODE:
            LOG("Transitioning to Lid Closed Mode...\n");
            WiFiState = LID_CLOSED;
            break;
        default:
            LOG("Command not supported \n");
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

#ifdef PICO_AS_TCP_SERVER
    tcp_server_process_recv_message(&received_command);
    if(received_command == PICO_TRANSITION_TO_ACTIVE_MODE) WiFi_ProcessCommand(received_command); 

#else /* defaults to Pico as TCP client */
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
        LOG("Communication Type not supported\n");
    }
#endif
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
    const char *message_not_allowed = "Transition not allowed";

#ifdef PICO_AS_TCP_SERVER
    /************** RX **************/
    /* Handle any received messages */
    tcp_server_process_recv_message(&received_command);

    if(received_command == PICO_TRANSITION_TO_LID_OPEN_MODE)
    {
        // Lock the mutex
        if (xSemaphoreTake(lidMutex, portMAX_DELAY) == pdTRUE)
        {
            if(lid_open_global == true)
            {
                received_command = PICO_DO_NOTHING; //transition not allowed, lid is already open
                (void)tcp_server_send(message_not_allowed, strlen(message_not_allowed));
            }
            // Unlock the mutex
            xSemaphoreGive(lidMutex);
        }
    }
    else if(received_command == PICO_TRANSITION_TO_LID_CLOSED_MODE)
    {
        // Lock the mutex
        if (xSemaphoreTake(lidMutex, portMAX_DELAY) == pdTRUE)
        {
            if(lid_open_global == false)
            {
                received_command = PICO_DO_NOTHING; //transition not allowed, lid is already closed
                (void)tcp_server_send(message_not_allowed, strlen(message_not_allowed));
            }
            // Unlock the mutex
            xSemaphoreGive(lidMutex);
        }
    }

    WiFi_ProcessCommand(received_command); 

    /************** TX **************/
    /* Send a message (for now just a test message) */
    (void)tcp_server_send(message, strlen(message));
#else /* defaults to Pico as TCP client */
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
        LOG("Communication Type not supported\n");
    }
#endif
}

/* 
 * Function: WiFi_MonitorState
 * 
 * Description: State in which the Pico W sends out Monitor data
 * 
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 *
 */
static void WiFi_MonitorState(void)
{
    uint8_t received_command = PICO_DO_NOTHING;

#ifdef PICO_AS_TCP_SERVER
    /************** RX **************/
    /* Handle any received messages */
    tcp_server_process_recv_message(&received_command);
    WiFi_ProcessCommand(received_command); 

    /************** TX **************/
    /* Signal Monitor Task to send data */
    //TODO (currently only supported with TCP client): xTaskNotifyGive(monitorTaskHandle);
#else /* defaults to Pico as TCP client */
    if(TransportLayer == TCP_COMMUNICATION)
    {
        /************** RX **************/
        /* Handle any received messages */
        tcp_client_process_recv_message(&received_command);
        WiFi_ProcessCommand(received_command); 

         /************** TX **************/
        /* Signal Monitor Task to send data */
        xTaskNotifyGive(monitorTaskHandle);
    }
    else if(TransportLayer == UDP_COMMUNICATION)
    {
        /* Monitoring supported only over TCP for now */
    }
    else
    {
        LOG("Communication Type not supported\n");
    }
#endif
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
        if(start_TCP_server() == true) WiFiState = LISTEN;
#else /* defaults to Pico as TCP client */
        if(start_TCP_client() == true) WiFiState = LISTEN;
#endif
    }
    else if(TransportLayer == UDP_COMMUNICATION)
    {
        if(start_UDP_client() == true) WiFiState = LISTEN;
    }
    else
    {
        LOG("Communication Type not supported \n");
    }
}

/* 
 * Function: Wifi_LidOpen_State
 * 
 * Description: State in which the Pico W controls the motor that opens the box lid.
 * 
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 *
 */
static void Wifi_LidOpen_State(void)
{
    uint8_t received_command = PICO_DO_NOTHING;

#ifdef PICO_AS_TCP_SERVER
    /************** RX **************/
    xTimerReset(aliveTimer, portMAX_DELAY); // Reset the Alive Timer (which is acting as a watchdog for the lid)

    // Clear any previously pending notifications
    xTaskNotifyStateClear(NULL);
    ulTaskNotifyValueClear(NULL, 0xFFFFFFFF);

    /* Open the Box Lid */
    GPIO_Set(GPIO_MOTOR_UP);

    /* Wait to receive a notification sent directly to this task from the
    interrupt service routine. */
    uint32_t ulEventsToProcess = ulTaskNotifyTake( pdTRUE, portMAX_DELAY);

    if( ulEventsToProcess != 0 )
    {
        /* To get here at least one event must have occurred. Clear it to indicate it has been processed */
        ulEventsToProcess = 0;

        vTaskDelay(pdMS_TO_TICKS(OPENING_TIME)); // Open the lid fully
        GPIO_Set(GPIO_LED); // Turn on the LED when lid is open
        GPIO_Clear(GPIO_MOTOR_UP);
    }
    else
    {
        LOG("Notification not received\n");
        GPIO_Clear(GPIO_MOTOR_UP);
        GPIO_Toggle(GPIO_LED); // Turn on the LED to indicate an error
    }

    // Lock the mutex
    if (xSemaphoreTake(lidMutex, portMAX_DELAY) == pdTRUE)
    {
        lid_open_global = true;

        // Unlock the mutex
        xSemaphoreGive(lidMutex);
    }

    received_command = PICO_TRANSITION_TO_ACTIVE_MODE; //once the lid is open, transition back to active mode
    WiFi_ProcessCommand(received_command); 

    /************** TX **************/
    //nothing to send for now
    
#else /* defaults to Pico as TCP client */
    /* This state only supported with Pico as TCP server */
    WiFiState = LISTEN; // leave this state so to not get stuck
#endif
}

/* 
 * Function: Wifi_LidClosed_State
 * 
 * Description: State in which the Pico W controls the motor that closes the box lid.
 * 
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 *
 */
static void Wifi_LidClosed_State(void)
{
    uint8_t received_command = PICO_DO_NOTHING;

#ifdef PICO_AS_TCP_SERVER
    /************** RX **************/
    xTimerReset(aliveTimer, portMAX_DELAY); // Reset the Alive Timer (which is acting as a watchdog for the lid)

    // Clear any previously pending notifications
    xTaskNotifyStateClear(NULL);
    ulTaskNotifyValueClear(NULL, 0xFFFFFFFF);

    /* Close the Box Lid */
    GPIO_Set(GPIO_MOTOR_DOWN);

    /* Wait to receive a notification sent directly to this task from the
    interrupt service routine. */
    uint32_t ulEventsToProcess = ulTaskNotifyTake( pdTRUE, portMAX_DELAY);

    if( ulEventsToProcess != 0 )
    {
        /* To get here at least one event must have occurred. Clear it to indicate it has been processed */
        ulEventsToProcess = 0;
        vTaskDelay(pdMS_TO_TICKS(CLOSING_TIME)); // Wait for the lid to fully close (experimentally determined)
        GPIO_Clear(GPIO_LED); // Turn off the LED when lid is closed
        GPIO_Clear(GPIO_MOTOR_DOWN);
    }
    else
    {
        LOG("Notification not received\n");
        GPIO_Clear(GPIO_MOTOR_DOWN);
        GPIO_Toggle(GPIO_LED); // Turn on the LED to indicate an error
    }

    // Lock the mutex
    if (xSemaphoreTake(lidMutex, portMAX_DELAY) == pdTRUE)
    {
        lid_open_global = false;
        
        // Unlock the mutex
        xSemaphoreGive(lidMutex);
    }

    received_command = PICO_TRANSITION_TO_ACTIVE_MODE; //once the lid is closed, transition back to active mode
    WiFi_ProcessCommand(received_command); 

    /************** TX **************/
    //nothing to send for now
    
#else /* defaults to Pico as TCP client */
    /* This state only supported with Pico as TCP server */
    WiFiState = LISTEN; // leave this state so to not get stuck
#endif
}

