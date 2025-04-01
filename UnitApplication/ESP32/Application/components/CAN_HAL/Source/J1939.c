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

// Define masks and shifts for J1939 fields
#define J1939_PRIORITY_MASK     0x07 // 3 bits
#define J1939_DP_MASK           0x01 // 1 bit
#define J1939_PF_MASK           0xFF // 8 bits
#define J1939_PS_MASK           0xFF // 8 bits
#define J1939_SA_MASK           0xFF // 8 bits

#define J1939_PRIORITY_SHIFT    26
#define J1939_DP_SHIFT          24
#define J1939_PF_SHIFT          16
#define J1939_PS_SHIFT          8
#define J1939_SA_SHIFT          0
/*******************************************************************************/
/*                               DATA TYPES                                    */
/*******************************************************************************/

/*******************************************************************************/
/*                        GLOBAL FUNCTION DECLARATIONS                         */
/*******************************************************************************/

/*******************************************************************************/
/*                        STATIC FUNCTION DECLARATIONS                         */
/*******************************************************************************/

/**
 * @brief Assembles a J1939 29-bit CAN message identifier.
 *
 * @param priority     Message priority (0-7).
 * @param data_page    Data Page (0 or 1).
 * @param pdu_format   PDU Format (0-255). Determines message type.
 * @param pdu_specific PDU Specific (0-255). Destination Address (if PF < 240)
 * or Group Extension (if PF >= 240).
 * @param src_address  Source Address (0-255, excluding 254/255 usually).
 * @return uint32_t The assembled 29-bit J1939 identifier.
 */
static uint32_t assembleJ1939MessageID( uint8_t priority,
                                        uint8_t data_page,
                                        uint8_t pdu_format,
                                        uint8_t pdu_specific,
                                        uint8_t src_address);

/**
* @brief Disassembles a J1939 29-bit CAN message identifier into its components.
*
* @param messageID    The 29-bit J1939 identifier to disassemble.
* @param priority     Pointer to store the extracted priority (0-7).
* @param data_page    Pointer to store the extracted Data Page (0 or 1).
* @param pdu_format   Pointer to store the extracted PDU Format (0-255).
* @param pdu_specifics Pointer to store the extracted PDU Specific (0-255).
* @param src_address  Pointer to store the extracted Source Address (0-255).
*/
static void disassembleJ1939MessageID(  uint32_t messageID,
                                        uint8_t *priority,
                                        uint8_t *data_page,
                                        uint8_t *pdu_format,
                                        uint8_t *pdu_specifics,
                                        uint8_t *src_address);
/*******************************************************************************/
/*                            STATIC VARIABLES                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                            GLOBAL VARIABLES                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                        STATIC FUNCTION DEFINITIONS                          */
/*******************************************************************************/

static uint32_t assembleJ1939MessageID( uint8_t priority,
                                        uint8_t data_page,
                                        uint8_t pdu_format,
                                        uint8_t pdu_specific,
                                        uint8_t src_address)
{
    uint32_t messageID = 0;

    /* J1939 ID Structure (based on J1939-21 Rev Dec 2006):
    *      Bits 28-26: Priority (P)
    *      Bit 25:     Extended Data Page (EDP) - Must be 0 for standard messages per this revision.
    *      Bit 24:     Data Page (DP)
    *      Bits 23-16: PDU Format (PF)
    *      Bits 15-8:  PDU Specific (PS)
    *      Bits 7-0:   Source Address (SA)
    */

    /* Validate the non-8-bits elements to throw an error if they exceed their limits (should never happen) */
    assert((priority & ~J1939_PRIORITY_MASK) == 0); // Make sure priority fits in 3 bits
    assert((data_page & ~J1939_DP_MASK) == 0); // Make sure data_page fits in 1 bit
    /* No need to assert PF, PS, SA if uint8_t, as they fit 8 bits. */

    /* Assemble the message ID using masks and shifts */
    /* Masking ensures inputs don't exceed their allocated bits. */
    messageID |= ((uint32_t)(priority    & J1939_PRIORITY_MASK) << J1939_PRIORITY_SHIFT);
    /* Bit 25 (EDP) is implicitly 0 because we start with messageID = 0 and the priority shift doesn't affect it.*/
    messageID |= ((uint32_t)(data_page   & J1939_DP_MASK)       << J1939_DP_SHIFT);
    messageID |= ((uint32_t)(pdu_format  & J1939_PF_MASK)       << J1939_PF_SHIFT);
    messageID |= ((uint32_t)(pdu_specific& J1939_PS_MASK)       << J1939_PS_SHIFT);
    messageID |= ((uint32_t)(src_address & J1939_SA_MASK)       << J1939_SA_SHIFT); // Shift is 0, but shown for consistency

    return messageID;
}

static void disassembleJ1939MessageID(  uint32_t messageID,
                                        uint8_t *priority,
                                        uint8_t *data_page,
                                        uint8_t *pdu_format,
                                        uint8_t *pdu_specifics,
                                        uint8_t *src_address) 
{
    /* Extract the components from the message ID */
    *priority = (uint8_t)((messageID >> J1939_PRIORITY_SHIFT) & J1939_PRIORITY_MASK);
    *data_page = (uint8_t)((messageID >> J1939_DP_SHIFT) & J1939_DP_MASK);
    *pdu_format = (uint8_t)((messageID >> J1939_PF_SHIFT) & J1939_PF_MASK);
    *pdu_specifics = (uint8_t)((messageID >> J1939_PS_SHIFT) & J1939_PS_MASK);
    *src_address = (uint8_t)((messageID >> J1939_SA_SHIFT) & J1939_SA_MASK);
}

/*******************************************************************************/
/*                        GLOBAL FUNCTION DEFINITIONS                          */
/*******************************************************************************/

esp_err_t send_J1393_message(const uint8_t *data_field, uint8_t data_field_length)
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
    uint8_t data_page = 0;    // Data page (0 or 1, used for multi-page messages)
    uint8_t pdu_format = 240; // PDU format (0-255, decides whether PDU Specific Field is a destination address (PDU1) or group extension (PDU2))
    uint8_t pdu_specifics = ESP32_2_SRC_ADDR; // PDU Specifics (0-255, used for destination address in PDU1 or group extension in PDU2)

    /* Node addresses in J1939 are typically assigned via the Address Claiming process (see J1939-81) */
    /* Here we are using static addresses for simplicity (TODO - Implement Address Claiming) */
    uint8_t src_address = ESP32_1_SRC_ADDR; // Source address (0-255, identifies the sender of the message)

    uint32_t message_id = assembleJ1939MessageID(priority, data_page, pdu_format, pdu_specifics, src_address);

    return send_CAN_message(message_id, data_field, data_field_length);
}



