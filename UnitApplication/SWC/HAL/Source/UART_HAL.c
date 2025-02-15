
/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/
#include "UART_HAL.h"
#include "hardware/irq.h"
#include <stdio.h>

/*******************************************************************************/
/*                           PRIVATE TYPE DEFINITIONS                          */
/*******************************************************************************/

/** Internal state tracking structure */
typedef struct
{
    bool initialized;                     /* Flag indicating if the UART has been initialized */
    UART_CallbackFn_t rx_callback;        /* Function pointer to receive callback handler */
    uint32_t last_error;                  /* Last error code recorded during operation */
    uart_inst_t* const uart_id;           /* Pointer to the hardware UART instance */
    volatile bool interrupt_enabled;      /* Flag indicating if interrupts are enabled */
    volatile bool rx_callback_triggered;  /* Flag indicating if receive callback was triggered */
} UART_Internal_State_t; 

/*******************************************************************************/
/*                            PRIVATE VARIABLES                                */
/*******************************************************************************/

/* Initial state for UART0 and UART1 */
static UART_Internal_State_t uart0_state = {false, NULL, 0, uart0, false, false};
static UART_Internal_State_t uart1_state = {false, NULL, 0, uart1, false, false};

/*******************************************************************************/
/*                        PRIVATE FUNCTION DECLARATIONS                        */
/*******************************************************************************/

/**
 * @brief Get internal state structure for UART instance (either UART0 or UART1)
 * @param uart_id UART instance
 * @return Pointer to internal state structure
 */
static UART_Internal_State_t* get_uart_state(uart_inst_t* uart_id) 
{
    if (uart0_state.uart_id == uart_id) return &uart0_state;
    if (uart1_state.uart_id == uart_id) return &uart1_state;

    return NULL;
}

/**
 * @brief UART Receive IRQ handler. Drains the RX FIFO and then triggers the callback.
 *
 * This version loops over all characters in the RX FIFO and discards them (or could store them if needed),
 * and then, after the FIFO is completely drained, sets the callback trigger flag and calls the callback.
 *
 * @param uart_id Pointer to the UART instance.
 */
static void uart_rx_irq_handler(uart_inst_t *uart_id) 
{
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state || !state->interrupt_enabled) return;

    /* Drain all characters in the RX FIFO */
    while (uart_is_readable(uart_id))  // Check whether data is waiting in the RX FIFO of the UART instance (true if FIFO not empty)
    {
        /* Read a single character from the UART instance (blocking until char has been read) */
        (void)uart_getc(uart_id); // Discard the chars for now (TODO: process them if needed)
    }

    /* After processing all characters, trigger the callback once */
    if (state->rx_callback)
    {
        state->rx_callback_triggered = true;
        state->rx_callback();
    }
}

/**
 * @brief UART0 IRQ handler
 */
static void uart0_irq_handler(void) 
{
    uart_rx_irq_handler(uart0);
}

/**
 * @brief UART1 IRQ handler
 */
static void uart1_irq_handler(void) 
{
    uart_rx_irq_handler(uart1);
}

/*******************************************************************************/
/*                        GLOBAL FUNCTION DEFINITIONS                          */
/*******************************************************************************/

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
    
    // Enable FIFO for buffering in polling mode
    uart_set_fifo_enabled(config->uart_id, true);  // Changed: enable FIFO
    
    // Update internal state
    state->initialized = true;
    state->rx_callback = NULL;
    state->last_error = 0;
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

    // Reset state
    state->rx_callback = NULL;
    state->interrupt_enabled = false;
    state->rx_callback_triggered = false;

    if (!callback) {
        return UART_OK;
    }

    // Empty FIFO
    while (uart_is_readable(uart_id)) {
        (void)uart_getc(uart_id);
    }
    
    // Disable FIFO for per-character interrupt handling
    uart_set_fifo_enabled(uart_id, false);  // Disable FIFO in interrupt mode

    // Set handler and callback
    irq_handler_t handler = (uart_id == uart0) ? uart0_irq_handler : uart1_irq_handler;
    irq_set_exclusive_handler(uart_id == uart0 ? UART0_IRQ : UART1_IRQ, handler);

    state->rx_callback = callback;
    state->interrupt_enabled = true;

    // Enable only RX interrupt and NVIC
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

UART_Status_t UART_DisableRxInterrupt(uart_inst_t* uart_id) 
{
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;

    // Disable IRQ in NVIC and UART
    irq_set_enabled(uart_id == uart0 ? UART0_IRQ : UART1_IRQ, false);
    uart_set_irq_enables(uart_id, false, false);
    state->interrupt_enabled = false;
    state->rx_callback = NULL;
    state->rx_callback_triggered = false;
    return UART_OK;
}