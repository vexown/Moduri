/**
 * @file CAN_HAL.h
 * @brief Header file for the CAN Hardware Abstraction Layer
 */

#ifndef CAN_HAL_H
#define CAN_HAL_H

/*******************************************************************************/
/*                                 DEFINES                                     */
/*******************************************************************************/
#define USE_CAN_AS_TRANSMITTER  0
#define USE_CAN_AS_RECEIVER     1
#if (USE_CAN_AS_TRANSMITTER == 1) && (USE_CAN_AS_RECEIVER == 1)
#error "Only one CAN mode can be selected at a time for now (TODO - implement both modes at the same time)"
#endif
/*******************************************************************************/
/*                               DATA TYPES                                    */
/*******************************************************************************/

/*******************************************************************************/
/*                            GLOBAL VARIABLES                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                        GLOBAL FUNCTION DELCARATION                          */
/*******************************************************************************/
void init_twai(void);

#if (USE_CAN_AS_TRANSMITTER == 1)
void sender_task(void *pvParameters);
#endif

#if (USE_CAN_AS_RECEIVER == 1)
void receiver_task(void *pvParameters);
#endif

#endif /* CAN_HAL_H */