/**
 * @file J1939.h
 * @brief Header file for the J1939 protocol implementation
 */

#ifndef J1939_H
#define J1939_H

/*******************************************************************************/
/*                                 DEFINES                                     */
/*******************************************************************************/
#define J1939_ENABLED 1

/*******************************************************************************/
/*                               DATA TYPES                                    */
/*******************************************************************************/
/**
 * @brief Structure to hold J1939 message components and data.
 */
typedef struct {
    uint8_t priority;       ///< Message priority (0-7)
    uint8_t data_page;      ///< Data Page (0 or 1)
    uint8_t pdu_format;     ///< PDU Format (0-255)
    uint8_t pdu_specifics;  ///< PDU Specific (0-255, Destination Address or Group Extension)
    uint8_t src_address;    ///< Source Address (0-255)
    uint8_t data[8];        ///< Data field payload (up to 8 bytes for standard CAN)
    uint8_t data_length;    ///< Length of the data field in bytes
    // Add timestamp or other metadata if needed later
} J1939_Message_t;

/*******************************************************************************/
/*                            GLOBAL VARIABLES                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                        GLOBAL FUNCTION DELCARATION                          */
/*******************************************************************************/

/**
 * @brief Receives a J1939 message.
 *
 * @param message Pointer to the J1939_Message_t structure to fill with received data.
 *
 * @return esp_err_t ESP_OK on success, ESP_FAIL or other error codes on failure.
 */
esp_err_t receive_J1939_message(J1939_Message_t *message);

/**
 * @brief Sends a J1939 message.
 *
 * @param void (for now it just sends a predefined message for testing - TODO)
 *
 * @return esp_err_t ESP_OK on success, ESP_FAIL or other error codes on failure.
 */
esp_err_t send_J1939_message(void);

#endif /* J1939_H */
