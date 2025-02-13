
#include "UART_HAL.h"
#include <string.h>
#include "hardware/irq.h"
#include <stdio.h>

/** Internal state tracking structure */
typedef struct {
    bool initialized;
    UART_CallbackFn_t rx_callback;
    uint32_t last_error;
    uart_inst_t* uart_id;
    volatile bool interrupt_enabled;
    volatile bool rx_callback_triggered;  // New flag
} UART_Internal_State_t;

static UART_Internal_State_t uart0_state = {false, NULL, 0, uart0, false, false};
static UART_Internal_State_t uart1_state = {false, NULL, 0, uart1, false, false};

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

static void uart_irq_handler(uart_inst_t *uart_id) {
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state || !state->interrupt_enabled) {
        return;
    }

    uart_hw_t *hw = uart_get_hw(uart_id);
    uint32_t status = hw->mis;

    if ((status & UART_UARTMIS_RXMIS_BITS) && state->rx_callback) {
        // Read single byte if available
        if (uart_is_readable(uart_id)) {
            uint8_t byte = uart_getc(uart_id);
            state->rx_callback_triggered = true;
            state->rx_callback();
        }
        
        // Clear RX interrupt flag
        hw->icr = UART_UARTICR_RXIC_BITS;
    }

    // Clear any other interrupts
    hw->icr = status;
}


/**
 * @brief UART0 IRQ handler
 */
static void uart0_irq_handler(void) {
    uart_irq_handler(uart0);
}

/**
 * @brief UART1 IRQ handler
 */
static void uart1_irq_handler(void) {
    uart_irq_handler(uart1);
}

UART_Status_t UART_Init(const UART_Config_t* config) {
    if (!config || !config->uart_id || 
        config->data_bits < 5 || config->data_bits > 8 ||
        config->stop_bits < 1 || config->stop_bits > 2) {
        return UART_ERROR_INVALID_PARAMS;
    }

    UART_Internal_State_t* state = get_uart_state(config->uart_id);
    if (!state) return UART_ERROR_INVALID_PARAMS;

    // Disable any existing interrupts first
    irq_set_enabled(config->uart_id == uart0 ? UART0_IRQ : UART1_IRQ, false);
    uart_set_irq_enables(config->uart_id, false, false);
    
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
    state->uart_id = config->uart_id;
    state->interrupt_enabled = false;

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

    // Disable all interrupts
    irq_set_enabled(uart_id == uart0 ? UART0_IRQ : UART1_IRQ, false);
    uart_set_irq_enables(uart_id, false, false);

    uart_hw_t *hw = uart_get_hw(uart_id);
    hw->imsc = 0;
    hw->icr = 0x7FF;

    // Reset state
    state->rx_callback = NULL;
    state->interrupt_enabled = false;
    state->rx_callback_triggered = false;

    if (!callback) {
        return UART_OK;
    }

    // Empty FIFO
    while (uart_is_readable(uart_id)) {
        uart_getc(uart_id);
    }

    // Configure UART
    hw->ifls = 0x0;  // FIFO trigger level = 1 byte
    hw->icr = UART_UARTICR_RXIC_BITS;
    
    // Set handler and callback
    irq_handler_t handler = (uart_id == uart0) ? uart0_irq_handler : uart1_irq_handler;
    irq_set_exclusive_handler(uart_id == uart0 ? UART0_IRQ : UART1_IRQ, handler);
    state->rx_callback = callback;
    state->interrupt_enabled = true;

    // Enable only RX interrupt
    hw->imsc = UART_UARTIMSC_RXIM_BITS;
    
    // Enable UART RX interrupt and NVIC
    uart_set_irq_enables(uart_id, true, false);
    irq_set_enabled(uart_id == uart0 ? UART0_IRQ : UART1_IRQ, true);

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

 bool UART_IsCallbackTriggered(uart_inst_t* uart_id) {
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state) return false;
    return state->rx_callback_triggered;
}