/**
 * @file CAN_HAL.h
 * @brief Header file for the CAN Hardware Abstraction Layer
 */

#ifndef CAN_HAL_H
#define CAN_HAL_H

/*******************************************************************************/
/*                                 DEFINES                                     */
/*******************************************************************************/
#define ESP32_1_CAN_ID (uint32_t)0xA1
#define ESP32_2_CAN_ID (uint32_t)0xA2
/*******************************************************************************/
/*                               DATA TYPES                                    */
/*******************************************************************************/

/*******************************************************************************/
/*                            GLOBAL VARIABLES                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                        GLOBAL FUNCTION DELCARATION                          */
/*******************************************************************************/

/**
 * @brief Initializes the TWAI peripheral (Two-Wire Automotive Interface)
 * 
 * This function initializes the TWAI peripheral with the default configuration
 * for the ESP32. The default configuration uses the following settings:
 * - Normal mode
 * - 500 kbps baud rate
 * - Accept all messages
 * 
 * @param None
 * @return esp_err_t Returns ESP_OK if the TWAI peripheral was successfully initialized or the error code if it failed
 * 
 * Note - See more about the TWAI peripheral here: 
 * https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/api-reference/peripherals/twai.html
 */
esp_err_t init_twai(void);

/**
 * @brief Sends a CAN message
 * 
 * This function sends a CAN message with the specified message ID and data.
 * The data length is also specified.
 * 
 * @param message_id The message ID of the CAN message
 * @param data The data to be sent in the CAN message
 * @param data_length The length of the data to be sent
 * @return esp_err_t Returns ESP_OK if the message was successfully sent or the error code if it failed
 */
esp_err_t send_CAN_message(uint32_t message_id, uint8_t *data, uint8_t data_length);

void receiver_task(void *pvParameters);


#endif /* CAN_HAL_H */