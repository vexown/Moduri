/**
 * @file UART_HAL.h
 * @brief Hardware Abstraction Layer for UART functionality on RP2350
 * @details This HAL provides a simplified interface for UART operations on the Raspberry Pi Pico 2 W (RP2350) microcontroller.
 */

 #ifndef UART_HAL_H
 #define UART_HAL_H

 #if 0 /* Test Code - you can use this to test the UART HAL or as a reference to get an idea how to use it */

 //TODO - add test code

 #endif
 
 #include "pico/stdlib.h"
 #include "hardware/uart.h"
 #include "hardware/irq.h"
 #include <stdbool.h>
 #include <stdint.h>
 
 /** Maximum buffer size for UART operations */
 #define UART_MAX_BUFFER_SIZE 256
 
 /** UART configuration structure */
 typedef struct {
     uart_inst_t* uart_id;     /**< UART instance (uart0 or uart1) */
     uint baud_rate;           /**< Baud rate in bps */
     uint data_bits;           /**< Data bits (5-8) */
     uint stop_bits;           /**< Stop bits (1-2) */
     uart_parity_t parity;     /**< Parity setting */
     uint tx_pin;              /**< TX pin number */
     uint rx_pin;              /**< RX pin number */
 } UART_Config_t;
 
 /** UART status/error codes */
 typedef enum {
     UART_OK = 0,             /**< Operation successful */
     UART_ERROR_INVALID_PARAMS,/**< Invalid parameters provided */
     UART_ERROR_NOT_INITIALIZED,/**< UART not initialized */
     UART_ERROR_BUSY,         /**< UART busy with operation */
     UART_ERROR_TIMEOUT,      /**< Operation timed out */
     UART_ERROR_BUFFER_FULL,  /**< Buffer full error */
     UART_ERROR_BUFFER_EMPTY  /**< Buffer empty error */
 } UART_Status_t;
 
 /** Callback function type for UART events */
 typedef void (*UART_CallbackFn_t)(void);
 
 /**
  * @brief Initialize UART with specified configuration
  * @param config Pointer to UART configuration structure
  * @return UART_Status_t Status code
  */
 UART_Status_t UART_Init(const UART_Config_t* config);
 
 /**
  * @brief Deinitialize UART
  * @param uart_id UART instance to deinitialize
  * @return UART_Status_t Status code
  */
 UART_Status_t UART_Deinit(uart_inst_t* uart_id);
 
 /**
  * @brief Send data over UART
  * @param uart_id UART instance
  * @param data Pointer to data buffer
  * @param length Length of data to send
  * @param timeout_ms Timeout in milliseconds (0 for no timeout)
  * @return UART_Status_t Status code
  */
 UART_Status_t UART_Send(uart_inst_t* uart_id, const uint8_t* data, size_t length, uint32_t timeout_ms);
 
 /**
  * @brief Receive data over UART
  * @param uart_id UART instance
  * @param data Pointer to data buffer
  * @param length Length of data to receive
  * @param timeout_ms Timeout in milliseconds (0 for no timeout)
  * @return UART_Status_t Status code
  */
 UART_Status_t UART_Receive(uart_inst_t* uart_id, uint8_t* data, size_t length, uint32_t timeout_ms);
 
 /**
  * @brief Register callback for UART RX events
  * @param uart_id UART instance
  * @param callback Function pointer to callback
  * @return UART_Status_t Status code
  */
 UART_Status_t UART_RegisterRxCallback(uart_inst_t* uart_id, UART_CallbackFn_t callback);
 
 /**
  * @brief Check if UART is ready to send data
  * @param uart_id UART instance
  * @return bool True if ready to send
  */
 bool UART_IsTxReady(uart_inst_t* uart_id);
 
 /**
  * @brief Check if UART has received data
  * @param uart_id UART instance
  * @return bool True if data is available
  */
 bool UART_IsRxAvailable(uart_inst_t* uart_id);
 
 /**
  * @brief Flush UART TX buffer
  * @param uart_id UART instance
  * @return UART_Status_t Status code
  */
 UART_Status_t UART_FlushTx(uart_inst_t* uart_id);
 
 /**
  * @brief Flush UART RX buffer
  * @param uart_id UART instance
  * @return UART_Status_t Status code
  */
 UART_Status_t UART_FlushRx(uart_inst_t* uart_id);
 
 #endif /* UART_HAL_H */