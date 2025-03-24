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

/*******************************************************************************/
/*                                 MACROS                                      */
/*******************************************************************************/
#define ESP32_1_CAN_ID 0xA1
#define ESP32_2_CAN_ID 0xA2
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

void init_twai(void) 
{
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("TWAI driver installed\n");
    } else {
        printf("Failed to install TWAI driver\n");
        return;
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK) {
        printf("TWAI driver started\n");
    } else {
        printf("Failed to start TWAI driver\n");
        return;
    }
}

void sender_task(void *pvParameters) 
{
    uint8_t counter = 0;
    while (1) {
        twai_message_t message;
        message.identifier = ESP32_2_CAN_ID; // sending from ESP32_2
        message.data_length_code = 1;
        message.data[0] = counter++;
        if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
            printf("Sent message with ID=0x%lX, Data=0x%X\n", (unsigned long)ESP32_2_CAN_ID, counter - 1);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));  // Send every 1 second
    }
}


void receiver_task(void *pvParameters) 
{
    while (1) {
        twai_message_t message;
        if (twai_receive(&message, pdMS_TO_TICKS(1500)) == ESP_OK) {
            if (message.identifier == ESP32_1_CAN_ID) // receiving from ESP32_1
            {
                printf("Received message from other ESP32: ID=0x%lX, Data=0x%X\n",
                       (unsigned long)message.identifier, message.data[0]);
            }
        } else {
            printf("Failed to receive message\n");
        }
    }
}



