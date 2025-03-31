/**
 * ****************************************************************************
 * @file    J1939.c
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
/* PGN (Parameter Group Number) is a key to understanding the content of a J1939 message.It enables you to find the definitions 
    of standard messages within the J1939 documentation. It is the primary way that the J1939 standard organizes its message definitions.
    PGN is not directly placed into the message ID, but is used to derive the PDU format and PDU specifics. */
#define ESP32_1_PGN         65262 // PGN (Parameter Group Number) for ESP32_1 (0xFEEE)
#define ESP32_2_PGN         65266 // PGN (Parameter Group Number) for ESP32_2 (0xFEF2)

/* Source address identifies the sender of the message. It is a unique address assigned to each device on the CAN network. */
#define ESP32_1_SRC_ADDR    0x01  // Source Address for ESP32_1
#define ESP32_2_SRC_ADDR    0x02  // Source Address for ESP32_2

/* Priority for J1939 messages (0-7, occupies the 3 most significant bits (MSB) of the 29-bit identifier) */
#define PRIORITY_HIGH       (uint8_t)0b000  // 0 - highest priority
#define PRIORITY_NORMAL     (uint8_t)0b011  // 3 - normal priority
#define PRIORITY_LOW        (uint8_t)0b111  // 7 - lowest priority

/* Macros for PDU types */
#define J1939_PDU1_PF_LOWER_BOUND 0x00
#define J1939_PDU1_PF_UPPER_BOUND 0xEF
#define J1939_PDU2_PF_LOWER_BOUND 0xF0
#define J1939_PDU2_PF_UPPER_BOUND 0xFF

/* Macro to check if a PF value corresponds to PDU1 (PDU Specific Field is then a destination address) */
#define J1939_IS_PDU1(pf) ((pf >= J1939_PDU1_PF_LOWER_BOUND) && (pf <= J1939_PDU1_PF_UPPER_BOUND))

/* Macro to check if a PF value corresponds to PDU2 (PDU Specific Field is then a group extension) */
#define J1939_IS_PDU2(pf) ((pf >= J1939_PDU2_PF_LOWER_BOUND) && (pf <= J1939_PDU2_PF_UPPER_BOUND))

/* Macro to extract the destination address from the PS field, if the message is PDU1 */
#define J1939_DESTINATION_ADDRESS(pf, ps) (J1939_IS_PDU1(pf) ? ps : 0xFF) //0xff is commonly used to signify no destination address.

/* Macro to extract the group extension from the PS field, if the message is PDU2 */
#define J1939_GROUP_EXTENSION(pf, ps) (J1939_IS_PDU2(pf) ? ps : 0xFF) //0xff is commonly used to signify no group extension.

/*******************************************************************************/
/*                               DATA TYPES                                    */
/*******************************************************************************/

/*******************************************************************************/
/*                        GLOBAL FUNCTION DECLARATIONS                         */
/*******************************************************************************/

/*******************************************************************************/
/*                        STATIC FUNCTION DECLARATIONS                         */
/*******************************************************************************/
uint32_t assembleJ1939MessageID(uint8_t priority, uint8_t data_page, uint8_t pdu_format, uint8_t pdu_specifics, uint8_t src_address) 
{
    uint32_t messageID = 0;

    /* Assemble the message ID components as per J1939 standard */
    messageID |= ((uint32_t)priority << 26);
    messageID |= ((uint32_t)data_page << 25);
    messageID |= ((uint32_t)pdu_format << 16);
    messageID |= ((uint32_t)pdu_specifics << 8);
    messageID |= src_address;

    return messageID;
}

void disassembleJ1939MessageID(uint32_t messageID, uint8_t *priority, uint8_t *data_page, uint8_t *pdu_format, uint8_t *pdu_specifics, uint8_t *src_address) 
{
    /* Extract the components from the message ID */
    *priority = (uint8_t)((messageID >> 26) & 0x07);
    *data_page = (uint8_t)((messageID >> 25) & 0x01);
    *pdu_format = (uint8_t)((messageID >> 16) & 0xFF);
    *pdu_specifics = (uint8_t)((messageID >> 8) & 0xFF);
    *src_address = (uint8_t)(messageID & 0xFF);
}


/*******************************************************************************/
/*                            STATIC VARIABLES                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                            GLOBAL VARIABLES                                 */
/*******************************************************************************/
esp_err_t send_J1393_message(uint32_t message_id, const uint8_t *data, uint8_t data_length)
{
    /* Below core values are often taken from one of the predefined PGNs defined in the J1939-71 standard (except manufacturer specific PGNs)
        Example of a standard PGN: 
        EEC1 (Electronic Engine Controller 1) with PGN 61444 (0xF004)
            -> Data Length: 8 bytes
            -> Data Page: 0
            -> PDU Format: 240
            -> PDU Specifics: 3
            -> Default Priority: 3 */
    uint8_t priority = PRIORITY_NORMAL; // Decide the priority level for the message (0-7, lower number = higher priority)
    uint8_t data_page = 0;      // Data page (0 or 1, used for multi-page messages)
    uint8_t pdu_format = 240;   // PDU format (0-255, decides whether PDU Specific Field is a destination address (PDU1) or group extension (PDU2))
    uint8_t pdu_specifics = ESP32_2_SRC_ADDR; // PDU Specifics (0-255, used for destination address in PDU1 or group extension in PDU2)

    /* Node addresses in J1939 are typically assigned via the Address Claiming process (see J1939-81) */
    /* Here we are using static addresses for simplicity (TODO - Implement Address Claiming) */
    uint8_t src_address = ESP32_1_SRC_ADDR; // Source address (0-255, identifies the sender of the message)

    assembleJ1939MessageID(priority, data_page, pdu_format, pdu_specifics, src_address);

    return send_CAN_message(message_id, data, data_length);
}

/*******************************************************************************/
/*                        STATIC FUNCTION DEFINITIONS                          */
/*******************************************************************************/


/*******************************************************************************/
/*                        GLOBAL FUNCTION DEFINITIONS                          */
/*******************************************************************************/




