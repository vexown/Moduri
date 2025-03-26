/**
 * ****************************************************************************
 * CAN_HAL.c
 *
 * ****************************************************************************
 */

/*******************************************************************************/
/*                                INCLUDES                                     */
/*******************************************************************************/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "CAN_HAL.h"
#include "Common.h"

/*******************************************************************************/
/*                                 MACROS                                      */
/*******************************************************************************/
#define CAN_TX_PIN  GPIO_NUM_5
#define CAN_RX_PIN  GPIO_NUM_4
/*******************************************************************************/
/*                               DATA TYPES                                    */
/*******************************************************************************/

/*******************************************************************************/
/*                        GLOBAL FUNCTION DECLARATIONS                         */
/*******************************************************************************/

/*******************************************************************************/
/*                        STATIC FUNCTION DECLARATIONS                         */
/*******************************************************************************/

/*******************************************************************************/
/*                            STATIC VARIABLES                                 */
/*******************************************************************************/
static const uint8_t response_templates[3][STANDARD_CAN_MAX_DATA_LENGTH] = 
{
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xF0, 0xF7, 0xFF},
    {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}
};
/*******************************************************************************/
/*                            GLOBAL VARIABLES                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                        STATIC FUNCTION DEFINITIONS                          */
/*******************************************************************************/

static esp_err_t remote_frame_responder(twai_message_t *message)
{
    const uint8_t* response = NULL;

    /* Analyze the received message and prepare a response (for now just respond based on the message ID) */
    switch (message->identifier)
    {
        case ESP32_1_CAN_ID:
            response = response_templates[ESP32_1_CAN_ID_RESPONSE_ID];
            break;
        case ESP32_2_CAN_ID:
            response = response_templates[ESP32_2_CAN_ID_RESPONSE_ID];
            break;
        default: // Respond with all FFs for unsupported message IDs
            response = response_templates[DEFAULT_RESPONSE_ID];
            LOG("Unsupported message ID received\n");
            break;
    }

    /* Send the response under the same message ID */
    esp_err_t status = send_CAN_message(message->identifier, response, STANDARD_CAN_MAX_DATA_LENGTH);

    return status;
}

/*******************************************************************************/
/*                        GLOBAL FUNCTION DEFINITIONS                          */
/*******************************************************************************/

esp_err_t init_twai(void) 
{
    esp_err_t status = ESP_OK;
    /*  Set the general configuration of TWAI to default values plus specify the CAN pins plus the mode.
        For the default values, see the TWAI_GENERAL_CONFIG_DEFAULT_V2 macro in twai.h
        The CAN pins are the TX and RX pins of the CAN/TWAI controller which connect to the CAN transceiver which connects to the CAN bus.
        For the modes see twai_mode_t - in NORMAL mode, the controller can send/receive/acknowledge messages. */
    twai_general_config_t general_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    
    /*  Set the timing configuration of TWAI to default values plus specify the bit rate 
        Available bit rates are 25kbps, 50kbps, 100kbps, 125kbps, 250kbps, 500kbps, 800kbps, 1Mbps */
    twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();

    /*  Set the filter configuration of TWAI. In this case, accept all messages.
        This acceptance filter allows you to selectively receive only certain CAN messages based on their identifiers.
        For example .acceptance_code = (0x123 << 21), .acceptance_mask = ~(0x7FF << 21) would accept only messages with ID 0x123 
         We shift the value 21 bits because that's the location 11-bit message CAN ID in some TWAI register (bits 21-31) */
    twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    /*  Install TWAI driver using the previously set configurations. Required memory is allocated 
        and the driver is placed in the stopped state */
    status = twai_driver_install(&general_config, &timing_config, &filter_config);
    if (status == ESP_OK) 
    {
        LOG("TWAI driver installed successfully\n");
    } 
    else 
    {
        LOG("Failed to install TWAI driver. Error code: %d\n", status);
        return status;
    }

    /*  Start the TWAI driver. Puts the driver into the running state. This allows the TWAI driver to participate
        in TWAI bus activities such as transmitting/receiving messages. This can only be called when the 
        TWAI driver is in the stopped state. */
    status = twai_start();
    if (status == ESP_OK) 
    {
        LOG("TWAI driver started successfully\n");
    } 
    else 
    {
        LOG("Failed to start TWAI driver. Error code: %d\n", status);
        return status;
    }

    return status; // ESP_OK, otherwise it would have returned earlier with an error code
}

esp_err_t send_CAN_message(uint32_t message_id, const uint8_t *data, uint8_t data_length) 
{
    esp_err_t status = ESP_OK;
    twai_message_t message;

    if((data_length > 8) || (data == NULL))
    {
        LOG("Data length is greater than 8 bytes or data is NULL\n"); // For now we only support the standard CAN frame
        status = ESP_ERR_INVALID_SIZE;
    }
    else
    {
        /*  Formulate a TWAI message to transmit. The message contains the identifier, data length code, and data */
        message.identifier = message_id;
        message.data_length_code = data_length;

        /*  Populate the message buffer with the data provided by the user */
        LOG("Sending data...\n");
        for (uint8_t i = 0; i < data_length; i++)
        {
            message.data[i] = data[i];
            LOG("Data[%d] = 0x%X\n", i, message.data[i]);
        }

        /* Specify the TWAI flags for the message. The flags are used to specify the message type, such as standard or extended frame, 
        remote frame, single shot transmission, self reception request, and data length code non-compliant. */
        message.extd = 0;         // 1 - Extended Frame Format (29bit ID) or 0 - Standard Frame Format (11bit ID)
        message.rtr = 0;          // 1 - Message is a Remote Frame or 0 - Message is a Data Frame
        message.ss = 0;           // 1 - Transmit as a Single Shot Transmission (the message will not be retransmitted upon error or arbitration loss)
        message.self = 0;         // 1 - Transmit as a Self Reception Request (the msg will be received by the transmitting device) or 0 for normal transmission
        message.dlc_non_comp = 0; // 1 - Message's Data length code is larger than 8 (this will break compliance with ISO 11898-1) or 0 the opposite

        /*  Transmit a CAN message. The message is copied to the driver's internal buffer.
            The driver will transmit the message as soon as the bus is available. */
        status = twai_transmit(&message, pdMS_TO_TICKS(1000));
        if (status == ESP_OK) 
        {
            LOG("Message with ID=0x%lX sent successfully\n", (unsigned long)message_id);
        } 
        else 
        {
            LOG("Failed to send message. Error code: %d\n", status);
        }
    }

    return status;
} 

esp_err_t receive_CAN_message(uint8_t* buffer, uint8_t* buffer_length)
{
    bool message_received = false;
    esp_err_t status = ESP_OK;
    twai_message_t message;

    /* Check for NULL pointers */
    if (buffer == NULL || buffer_length == NULL) 
    {
        LOG("Invalid parameters: buffer or buffer_length is NULL\n");
        status = ESP_ERR_INVALID_ARG;
    }
    else
    {
        /* Attempt to receive a CAN message */
        if (twai_receive(&message, pdMS_TO_TICKS(1000)) == ESP_OK) 
        {
            message_received = true;
            LOG("Message received with ID=0x%lX (Extended=%d, RTR=%d, SS=%d, Self=%d, DLC Non-Comp=%d)\n", 
                (unsigned long)message.identifier, message.extd, message.rtr, message.ss, message.self, message.dlc_non_comp);
        } 
        else 
        {
            LOG("Failed to receive message\n");
            status = ESP_FAIL;
        }

        /* Process the received message if one was received */
        if(message_received)
        {
            /* Verify the length of the received data */
            if((message.data_length_code > 8) || (*buffer_length < message.data_length_code))
            {
                LOG("Data length %d is greater than 8 bytes or too big for the buffer of size %d\n", message.data_length_code, *buffer_length);
                status = ESP_ERR_INVALID_SIZE;
            }
            else
            {
                /*  Copy the received data to the buffer provided by the user */
                LOG("Receiving data...\n");
                for (uint8_t i = 0; i < message.data_length_code; i++)
                {
                    buffer[i] = message.data[i];
                    LOG("Data[%d] = 0x%X\n", i, buffer[i]);
                }
                *buffer_length = message.data_length_code; // Store the actual length of the data received
            }

            /* Handle remote frames (frames that request data) */
            if(message.rtr)
            {
                LOG("Received message is a Remote Frame. Responding with a Data Frame...\n");
                status = remote_frame_responder(&message);
            }
        }
    }
    
    return status;
}



