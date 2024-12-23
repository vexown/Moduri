/**
 * File: WiFi_main.c
 * Description: Implementation of the main WiFi function 
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>

/* Kernel includes. */
#include "FreeRTOS.h"

/* SDK includes */
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

/* WiFi includes */
#include "WiFi_UDP.h"
#include "WiFi_TCP.h"
#include "WiFi_Common.h"

/* OS includes */
#include "OS_manager.h"

/* Misc includes */
#include "Common.h"

/* HAL includes */
#include "ADC_HAL.h"

/*******************************************************************************/
/*                                 MACROS                                      */
/*******************************************************************************/

#define VOLTAGE_BUFFER_SIZE 32  // Buffer size for the voltage string

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
    MEASURE = 4
} WiFiStateType;

/*******************************************************************************/
/*                             STATIC VARIABLES                                */
/*******************************************************************************/

/* WiFi Communication Type */
static TransportLayerType TransportLayer = TCP_COMMUNICATION; /* default communication type is TCP */

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
static void WiFi_MeasureVoltage_State(void);

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
        case MEASURE:
            WiFi_MeasureVoltage_State();
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
        case PICO_TRANSITION_TO_MEASURE_MODE:
            LOG("Transitioning to Measure Mode...\n");
            WiFiState = MEASURE;
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

    if(TransportLayer == TCP_COMMUNICATION)
    {
        tcp_client_process_recv_message(&received_command);

        /* Force the Pico to transition to Active Mode */
        received_command = PICO_TRANSITION_TO_ACTIVE_MODE;

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

        /* Force the Pico to transition to Measure Mode */
        received_command = PICO_TRANSITION_TO_MEASURE_MODE;

        WiFi_ProcessCommand(received_command); 

        /************** TX **************/

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
 * Function: WiFi_MeasureVoltage_State
 * 
 * Description: State in which the Pico W measures the voltage on the ADC channels and sends the data to the Central App.
 *              This function was created for the purpose of the 2024 Christmas Gift Box project, and is not for general use.
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 *
 */
static void WiFi_MeasureVoltage_State(void)
{
    uint8_t received_command = PICO_DO_NOTHING;

    if(TransportLayer == TCP_COMMUNICATION)
    {
        /************** RX **************/
        /* Handle any received messages */
        tcp_client_process_recv_message(&received_command);
        WiFi_ProcessCommand(received_command); 

        /************** TX **************/
        /* Variables related to the HTTP GET request */
        const char *server_host = PICO_W_AP_TCP_SERVER_ADDRESS;
        const char *pathON = "/ledtest?led=1";
        const char *pathOFF = "/ledtest?led=0";

        /* Variables related to voltage */
        float voltage = 0.0;
        char voltage_message[VOLTAGE_BUFFER_SIZE * 3] = {0}; // 3 channels

        /* Variables related to sensor and box lid status */
        bool all_below_threshold = true;

        /* Variables related to timing of the sensor detection*/
        static uint32_t holdCount = 0; // Num of cycles with voltage below threshold. 1 cycle = 0.5s (hold for 5s)

        /* Read ADC Channels 0-3 */
        for (int channel = 0; channel < 3; channel++) 
        {
            (void)ADC_ReadVoltage(channel, &voltage);
#ifdef DEBUG_BUILD
            char single_voltage_message[VOLTAGE_BUFFER_SIZE];
            snprintf(single_voltage_message, VOLTAGE_BUFFER_SIZE, "Ch%d: %.4f V ", channel, voltage);
            strncat(voltage_message, single_voltage_message, VOLTAGE_BUFFER_SIZE);
#endif
            if (voltage >= 1.1) 
            {
                all_below_threshold = false;
                holdCount = 0;
            }
        }

#ifdef DEBUG_BUILD
        tcp_client_send(voltage_message, strlen(voltage_message));
#endif
        /* Check if all sensors are below the threshold */
        if (all_below_threshold)
        {
            if(holdCount < 10)
            {
                holdCount++;
            }
            else
            {   /* Send "open" message if all sensors are held below the threshold for at least 5s (10cycles at 0.5s) */
                /* Send HTTP GET request to open the box */
                send_http_get_request(server_host, pathON);

                /* Reset the hold count */
                holdCount = 0;
            }
        }
        else
        {  /* Send "closed" message if at least one sensor is above the threshold */
            /* Send HTTP GET request to close the box */
            send_http_get_request(server_host, pathOFF);
            holdCount = 0;
        }
    }
    else if(TransportLayer == UDP_COMMUNICATION)
    {
        /* Voltage measurements supported only over TCP for now */
    }
    else
    {
        LOG("Communication Type not supported\n");
    }
}



