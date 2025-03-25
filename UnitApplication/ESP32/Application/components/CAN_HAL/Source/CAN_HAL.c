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

/*******************************************************************************/
/*                            GLOBAL VARIABLES                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                        STATIC FUNCTION DEFINITIONS                          */
/*******************************************************************************/

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

esp_err_t send_CAN_message(uint32_t message_id, uint8_t *data, uint8_t data_length) 
{
    esp_err_t status = ESP_OK;
    twai_message_t message;

    if(data_length > 8)
    {
        LOG("Data length is greater than 8 bytes\n"); // For now we only support the standard CAN frame with 8 bytes of data
        return ESP_ERR_INVALID_SIZE;
    }
    else
    {
        /*  Formulate a TWAI message to transmit. The message contains the identifier, data length code, and data */
        message.identifier = message_id;
        message.data_length_code = data_length;

        /*  Populate the message buffer with the data provided by the user */
        for (uint8_t i = 0; i < data_length; i++)
        {
            message.data[i] = data[i];
            LOG("Data[%d] = 0x%X\n", i, message.data[i]);
        }
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
    return status;
} 

void receive_CAN_message(void *pvParameters) 
{
    while (1) {
        twai_message_t message;
        if (twai_receive(&message, pdMS_TO_TICKS(1500)) == ESP_OK) {
            if (message.identifier == ESP32_1_CAN_ID) // receiving from ESP32_1
            {
                LOG("Received message from other ESP32: ID=0x%lX, Data=0x%X\n",
                       (unsigned long)message.identifier, message.data[0]);
            }
        } else {
            LOG("Failed to receive message\n");
        }
    }
}



