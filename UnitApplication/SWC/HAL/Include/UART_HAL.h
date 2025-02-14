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
 typedef struct 
 {
    uart_inst_t* uart_id;     /**< UART instance (uart0 or uart1) */
    uint baud_rate;           /**< Baud rate in bps */
    uint data_bits;           /**< Data bits (5-8) */
    uint stop_bits;           /**< Stop bits (1-2) */
    uart_parity_t parity;     /**< Parity setting */
    uint tx_pin;              /**< TX pin number */
    uint rx_pin;              /**< RX pin number */
 } UART_Config_t;
 
 /** UART status/error codes */
 typedef enum 
 {
    UART_OK = 0,                /**< Operation successful */
    UART_ERROR_INVALID_PARAMS,  /**< Invalid parameters provided */
    UART_ERROR_NOT_INITIALIZED, /**< UART not initialized */
    UART_ERROR_BUSY,            /**< UART busy with operation */
    UART_ERROR_TIMEOUT,         /**< Operation timed out */
    UART_ERROR_BUFFER_FULL,     /**< Buffer full error */
    UART_ERROR_BUFFER_EMPTY     /**< Buffer empty error */
 } UART_Status_t;
 
 /** Callback function type for UART events */
 typedef void (*UART_CallbackFn_t)(void);
 

/**
 * @brief Initializes the UART peripheral.
 *
 * Configures the UART with the specified parameters, sets the correct GPIO functions for TX and RX,
 * and enables the FIFO for buffered (polling) operations.
 *
 * @param config Pointer to the UART configuration structure.
 * @return UART_Status_t UART_OK on success or an error code on failure.
 */
UART_Status_t UART_Init(const UART_Config_t* config);

/**
 * @brief Deinitializes the UART peripheral.
 *
 * Disables the UART and resets the internal state.
 *
 * @param uart_id Pointer to the UART instance (uart0 or uart1).
 * @return UART_Status_t UART_OK on success or an error code if not initialized.
 */
UART_Status_t UART_Deinit(uart_inst_t* uart_id);

/**
 * @brief Transmits data over UART.
 *
 * Sends the specified data and waits until every byte has been transmitted or the timeout occurs.
 *
 * @param uart_id Pointer to the UART instance.
 * @param data Pointer to the data buffer to transmit.
 * @param length Number of bytes to transmit.
 * @param timeout_ms Timeout period in milliseconds.
 * @return UART_Status_t UART_OK on success or an error code if the operation times out or parameters are invalid.
 */
UART_Status_t UART_Send(uart_inst_t* uart_id, const uint8_t* data, size_t length, uint32_t timeout_ms);

/**
 * @brief Receives data over UART.
 *
 * Reads the specified number of bytes from the UART interface, waiting until data is available
 * or the timeout period expires.
 *
 * @param uart_id Pointer to the UART instance.
 * @param data Buffer to store the received data.
 * @param length Number of bytes to receive.
 * @param timeout_ms Timeout period in milliseconds.
 * @return UART_Status_t UART_OK on success or an error code if the operation times out or parameters are invalid.
 */
UART_Status_t UART_Receive(uart_inst_t* uart_id, uint8_t* data, size_t length, uint32_t timeout_ms);

/**
 * @brief Registers a callback for UART receive events.
 *
 * Configures the UART to operate in interrupt mode by disabling FIFO buffering (for per-character handling),
 * clearing any pending data, and installing an exclusive IRQ handler. The provided callback is invoked
 * whenever a received byte triggers an interrupt.
 *
 * @param uart_id Pointer to the UART instance.
 * @param callback Function pointer to the callback. If NULL, any existing callback is cleared.
 * @return UART_Status_t UART_OK on success or an error code if the UART is not initialized.
 */
UART_Status_t UART_RegisterRxCallback(uart_inst_t* uart_id, UART_CallbackFn_t callback);

/**
 * @brief Checks if the UART transmit buffer is ready for sending a new byte.
 *
 * @param uart_id Pointer to the UART instance.
 * @return true if the UART transmit buffer is ready; otherwise false.
 */
bool UART_IsTxReady(uart_inst_t* uart_id);

/**
 * @brief Determines if data is available in the UART receive buffer.
 *
 * @param uart_id Pointer to the UART instance.
 * @return true if data is available for reading; otherwise false.
 */
bool UART_IsRxAvailable(uart_inst_t* uart_id);

/**
 * @brief Blocks until the UART transmit buffer is completely flushed.
 *
 * Waits until all data has been transmitted.
 *
 * @param uart_id Pointer to the UART instance.
 * @return UART_Status_t UART_OK on success or an error code if the UART is not initialized.
 */
UART_Status_t UART_FlushTx(uart_inst_t* uart_id);

/**
 * @brief Clears the UART receive buffer.
 *
 * Drains all unread data in the receive FIFO.
 *
 * @param uart_id Pointer to the UART instance.
 * @return UART_Status_t UART_OK on success or an error code if the UART is not initialized.
 */
UART_Status_t UART_FlushRx(uart_inst_t* uart_id);

/**
 * @brief Disables the UART receive interrupt.
 *
 * Disables the RX interrupt at both the NVIC and the UART peripheral level, clears the registered callback,
 * and resets the internal interrupt flag.
 *
 * @param uart_id Pointer to the UART instance.
 * @return UART_Status_t UART_OK on success or UART_ERROR_NOT_INITIALIZED if the UART is not initialized.
 */
UART_Status_t UART_DisableRxInterrupt(uart_inst_t* uart_id);
 
 #endif /* UART_HAL_H */