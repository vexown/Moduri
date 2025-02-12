
 #include "UART_HAL.h"
 #include <string.h>
 
 /** Internal state tracking structure */
 typedef struct {
     bool initialized;
     UART_CallbackFn_t rx_callback;
     uint32_t last_error;
 } UART_Internal_State_t;
 
 static UART_Internal_State_t uart0_state = {false, NULL, 0};
 static UART_Internal_State_t uart1_state = {false, NULL, 0};
 
 /**
  * @brief Get internal state structure for UART instance
  * @param uart_id UART instance
  * @return Pointer to internal state structure
  */
 static UART_Internal_State_t* get_uart_state(uart_inst_t* uart_id) {
     if (uart_id == uart0) return &uart0_state;
     if (uart_id == uart1) return &uart1_state;
     return NULL;
 }
 
 /**
  * @brief UART RX interrupt handler
  * @param uart_id UART instance
  */
 static void uart_rx_handler(uart_inst_t* uart_id) {
     UART_Internal_State_t* state = get_uart_state(uart_id);
     if (state && state->rx_callback) {
         state->rx_callback();
     }
     // Clear interrupt
     uart_get_hw(uart_id)->icr = UART_UARTICR_RXIC_BITS;
 }
 
 UART_Status_t UART_Init(const UART_Config_t* config) {
     if (!config || !config->uart_id || 
         config->data_bits < 5 || config->data_bits > 8 ||
         config->stop_bits < 1 || config->stop_bits > 2) {
         return UART_ERROR_INVALID_PARAMS;
     }
 
     UART_Internal_State_t* state = get_uart_state(config->uart_id);
     if (!state) return UART_ERROR_INVALID_PARAMS;
 
     // Initialize UART
     uart_init(config->uart_id, config->baud_rate);
     
     // Configure UART parameters
     uart_set_format(config->uart_id, config->data_bits, config->stop_bits, config->parity);
     
     // Set GPIO pins for UART
     gpio_set_function(config->tx_pin, GPIO_FUNC_UART);
     gpio_set_function(config->rx_pin, GPIO_FUNC_UART);
     
     // Enable FIFO
     uart_set_fifo_enabled(config->uart_id, true);
     
     // Update internal state
     state->initialized = true;
     state->rx_callback = NULL;
     state->last_error = 0;
 
     return UART_OK;
 }
 
 UART_Status_t UART_Deinit(uart_inst_t* uart_id) {
     UART_Internal_State_t* state = get_uart_state(uart_id);
     if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;
 
     uart_deinit(uart_id);
     state->initialized = false;
     state->rx_callback = NULL;
 
     return UART_OK;
 }
 
 UART_Status_t UART_Send(uart_inst_t* uart_id, const uint8_t* data, size_t length, uint32_t timeout_ms) {
     UART_Internal_State_t* state = get_uart_state(uart_id);
     if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;
     if (!data || length == 0) return UART_ERROR_INVALID_PARAMS;
 
     absolute_time_t timeout_time = make_timeout_time_ms(timeout_ms);
     size_t sent = 0;
 
     while (sent < length) {
         if (uart_is_writable(uart_id)) {
             uart_putc_raw(uart_id, data[sent++]);
         } else if (timeout_ms && time_reached(timeout_time)) {
             return UART_ERROR_TIMEOUT;
         }
     }
 
     return UART_OK;
 }
 
 UART_Status_t UART_Receive(uart_inst_t* uart_id, uint8_t* data, size_t length, uint32_t timeout_ms) {
     UART_Internal_State_t* state = get_uart_state(uart_id);
     if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;
     if (!data || length == 0) return UART_ERROR_INVALID_PARAMS;
 
     absolute_time_t timeout_time = make_timeout_time_ms(timeout_ms);
     size_t received = 0;
 
     while (received < length) {
         if (uart_is_readable(uart_id)) {
             data[received++] = uart_getc(uart_id);
         } else if (timeout_ms && time_reached(timeout_time)) {
             return UART_ERROR_TIMEOUT;
         }
     }
 
     return UART_OK;
 }
 
 UART_Status_t UART_RegisterRxCallback(uart_inst_t* uart_id, UART_CallbackFn_t callback) {
     UART_Internal_State_t* state = get_uart_state(uart_id);
     if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;
 
     state->rx_callback = callback;
 
     if (callback) {
         // Enable RX interrupt
         uart_set_irq_enables(uart_id, true, false);
         
         // Set up interrupt handler
         int irq = (uart_id == uart0) ? UART0_IRQ : UART1_IRQ;
         irq_set_exclusive_handler(irq, (irq_handler_t)uart_rx_handler);
         irq_set_enabled(irq, true);
     } else {
         // Disable RX interrupt
         uart_set_irq_enables(uart_id, false, false);
     }
 
     return UART_OK;
 }
 
 bool UART_IsTxReady(uart_inst_t* uart_id) {
     UART_Internal_State_t* state = get_uart_state(uart_id);
     if (!state || !state->initialized) return false;
     
     return uart_is_writable(uart_id);
 }
 
 bool UART_IsRxAvailable(uart_inst_t* uart_id) {
     UART_Internal_State_t* state = get_uart_state(uart_id);
     if (!state || !state->initialized) return false;
     
     return uart_is_readable(uart_id);
 }
 
 UART_Status_t UART_FlushTx(uart_inst_t* uart_id) {
     UART_Internal_State_t* state = get_uart_state(uart_id);
     if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;
 
     uart_tx_wait_blocking(uart_id);
     return UART_OK;
 }
 
 UART_Status_t UART_FlushRx(uart_inst_t* uart_id) {
     UART_Internal_State_t* state = get_uart_state(uart_id);
     if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;
 
     while (uart_is_readable(uart_id)) {
         uart_getc(uart_id);
     }
     return UART_OK;
 }