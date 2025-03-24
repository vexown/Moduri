/**
 * @file CAN_HAL.h
 * @brief Header file for the CAN Hardware Abstraction Layer
 */

#ifndef CAN_HAL_H
#define CAN_HAL_H

/*******************************************************************************/
/*                                 DEFINES                                     */
/*******************************************************************************/

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

void sender_task(void *pvParameters);
void receiver_task(void *pvParameters);


#endif /* CAN_HAL_H */