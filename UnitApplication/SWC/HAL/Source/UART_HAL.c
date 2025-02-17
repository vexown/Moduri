
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
 * @brief UART Receive IRQ handler.
 *
 * @details The UART RX interrupt behavior depends on FIFO mode:
 * 
 * WITH FIFO ENABLED:
 * -----------------
 * Interrupt triggers based on FIFO level (UARTIFLS register):
 * - FIFO is 16 bytes deep
 * - Trigger levels:
 *   000 = 1/8  full (2 chars)    - Lowest latency, most interrupts (default)
 *   001 = 1/4  full (4 chars)
 *   010 = 1/2  full (8 chars)    - Default balance
 *   011 = 3/4  full (12 chars)
 *   100 = 7/8  full (14 chars)   - Fewest interrupts, highest latency
 * 
 * WITH FIFO DISABLED:
 * ------------------
 * - Interrupt triggers immediately when one byte is received
 * - Must read byte quickly before next arrives
 * - Higher interrupt frequency but lower latency
 * - Useful for byte-by-byte processing
 * 
 * Note: RX timeout interrupt will still trigger if data sits in FIFO
 * for more than 32-bit periods, regardless of FIFO level (FIFO mode only).
 *
 * @param uart_id Pointer to the UART instance.
 */
static void uart_rx_irq_handler(uart_inst_t *uart_id) 
{
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state || !state->interrupt_enabled) return;

    /* Drain all characters in the RX FIFO or just the one byte if FIFO disabled */
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
    /* Call the generic RX IRQ handler with the UART0 instance */
    uart_rx_irq_handler(uart0);
}

/**
 * @brief UART1 IRQ handler
 */
static void uart1_irq_handler(void) 
{
    /* Call the generic RX IRQ handler with the UART1 instance */
    uart_rx_irq_handler(uart1);
}

/**
 * @brief Verify if the actual baud rate is within the acceptable error range (most devices accept up to 3% error).
 *
 * @param requested Requested baud rate
 * @param actual Actual baud rate
 * @return UART_Status_t UART_OK if within acceptable error range, otherwise UART_ERROR_INVALID_BAUDRATE
 */
static UART_Status_t verify_baudrate(uint32_t requested, uint32_t actual) 
{
    /* Avoid division by zero */
    if (requested == 0) return UART_ERROR_INVALID_PARAMS;
    
    /* Calculate error percentage (use float for precision) */
    float error_percent = ((float)abs((int)actual - (int)requested) / requested) * 100.0f;
    
    /* Industry standard typically accepts up to 3% error */
    const float MAX_BAUDRATE_ERROR = 3.0f;
    
    return (error_percent <= MAX_BAUDRATE_ERROR) ? UART_OK : UART_ERROR_INVALID_BAUDRATE;
}

/*******************************************************************************/
/*                        GLOBAL FUNCTION DEFINITIONS                          */
/*******************************************************************************/

UART_Status_t UART_Init(const UART_Config_t* config) 
{   
    /* Validate UART configuration parameters. Performes the following checks:
        1. Check if the UART instance is valid.
        2. Check if the data bits are within the valid range of 5 to 8 bits (valid range according to UART specification).
        3. Check if the stop bits are within the valid range of 1 to 2 bits (valid range according to UART specification). */
    if (!config || !config->uart_id || 
        config->data_bits < 5 || config->data_bits > 8 ||
        config->stop_bits < 1 || config->stop_bits > 2) 
    {
        return UART_ERROR_INVALID_PARAMS;
    }

    /* Get the internal state structure for the configured UART instance (either UART0 or UART1) */
    UART_Internal_State_t* state = get_uart_state(config->uart_id);
    if (!state) return UART_ERROR_INVALID_PARAMS;

    /* Disable any existing interrupts initially */
    irq_set_enabled(config->uart_id == uart0 ? UART0_IRQ : UART1_IRQ, false);
    uart_set_irq_enables(config->uart_id, false, false);
    
    /* Initialize UART with default settings (8 data bits, no parity bit, one stop bit) and specified baud rate */
    uint actual_baudrate = uart_init(config->uart_id, config->baud_rate);
    if (verify_baudrate(config->baud_rate, actual_baudrate) != UART_OK) return UART_ERROR_INVALID_BAUDRATE;
    
    /* Now change the UART settings to the user-specified configuration (UART interrupts must be disabled for this) */
    uart_set_format(config->uart_id, config->data_bits, config->stop_bits, config->parity);
    
    /* Set function of the configured GPIO pins for UART */
    gpio_set_function(config->tx_pin, GPIO_FUNC_UART);
    gpio_set_function(config->rx_pin, GPIO_FUNC_UART);
    
    /* Enable FIFOs for the specified UART instance (interrupts must be disabled for this) */
    uart_set_fifo_enabled(config->uart_id, true);
    
    /* Initialize internal state */
    state->initialized = true;
    state->rx_callback = NULL;
    state->last_error = 0;
    state->interrupt_enabled = false;

    return UART_OK;
}
 
 UART_Status_t UART_Deinit(uart_inst_t* uart_id) 
 {
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;

    uart_deinit(uart_id);
    state->initialized = false;
    state->rx_callback = NULL;

    return UART_OK;
 }
 
UART_Status_t UART_Send(uart_inst_t* uart_id, const uint8_t* data, size_t length, uint32_t timeout_ms) 
{
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;
    if (!data || length == 0) return UART_ERROR_INVALID_PARAMS;

    /* Get the timestamp of the current time + 'timeout_ms' */
    absolute_time_t timeout_time = make_timeout_time_ms(timeout_ms);
    size_t sent = 0; // Number of bytes sent so far

    /* Loop until all bytes are sent or timeout occurs */
    while (sent < length) 
    {
        /* Check if there is space available in the TX FIFO */
        if (uart_is_writable(uart_id)) 
        {
            /* Write single char to UART for transmission (blocks until the data has been sent to the UART transmit buffer hardware) */
            uart_putc_raw(uart_id, data[sent++]);
        } 
        else if (timeout_ms && time_reached(timeout_time)) //timeout_ms is non-zero and the current time has reached the timeout time
        {
            return UART_ERROR_TIMEOUT;
        }
    }

    return UART_OK;
}
 
UART_Status_t UART_Receive(uart_inst_t* uart_id, uint8_t* data, size_t length, uint32_t timeout_ms) 
{
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;
    if (!data || length == 0) return UART_ERROR_INVALID_PARAMS;

    /* Get the timestamp of the current time + 'timeout_ms' */
    absolute_time_t timeout_time = make_timeout_time_ms(timeout_ms);
    size_t received = 0; // Number of bytes received so far

    /* Loop until all bytes are received or timeout occurs */
    while (received < length) 
    {
        /* Check if there is data waiting in the RX FIFO */
        if (uart_is_readable(uart_id)) 
        {
            /* Read a single char from the UART instance and store it in the data buffer (blocks until char has been read) */
            data[received++] = uart_getc(uart_id);
        } 
        else if (timeout_ms && time_reached(timeout_time)) //timeout_ms is non-zero and the current time has reached the timeout time 
        {
            return UART_ERROR_TIMEOUT;
        }
    }

    return UART_OK;
}
 
UART_Status_t UART_RegisterRxCallback(uart_inst_t* uart_id, UART_CallbackFn_t callback) 
{
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;

    /* Reset parameters related to interrupt handling */
    state->rx_callback = NULL;
    state->interrupt_enabled = false;
    state->rx_callback_triggered = false;

    /* Nothing to do if the callback is NULL but not an error per se */
    if (!callback) return UART_OK;

    /* Drain all characters in the RX FIFO */
    while (uart_is_readable(uart_id)) 
    {
        /* Read and discard a single character from the UART instance (blocking until char has been read) */
        (void)uart_getc(uart_id);
    }
    
    /* Disable FIFO for per-character interrupt handling */
    uart_set_fifo_enabled(uart_id, false);  // Disable FIFO in interrupt mode. TODO - figure out interrupts with FIFO enabled

    /* Set the handler (low-level function that will directly handle the interrupt request) */
    irq_handler_t handler = (uart_id == uart0) ? uart0_irq_handler : uart1_irq_handler;
    irq_set_exclusive_handler(uart_id == uart0 ? UART0_IRQ : UART1_IRQ, handler);

    /* Set the callback (more high-level user-provided function that will be called from the handler to do app-specific stuff) */
    state->rx_callback = callback;
    state->interrupt_enabled = true;

    /* Enable IRQ in UART and NVIC */
    uart_set_irq_enables(uart_id, true, false);
    irq_set_enabled(uart_id == uart0 ? UART0_IRQ : UART1_IRQ, true);

    return UART_OK;
}
 
bool UART_IsTxReady(uart_inst_t* uart_id) 
{
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state || !state->initialized) return false;
    
    /* Check if there is space available in the TX FIFO and return the result */
    return uart_is_writable(uart_id);
}
 
bool UART_IsRxAvailable(uart_inst_t* uart_id) 
{
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state || !state->initialized) return false;
    
    /* Check if there is data waiting in the RX FIFO and return the result */
    return uart_is_readable(uart_id);
}
 
UART_Status_t UART_FlushTx(uart_inst_t* uart_id) 
{
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;

    /* Wait until the TX FIFO is completely flushed */
    /* Once data enters TX FIFO, the transmission process is autonomous, 
       UART hardware will push the data out on the TX line. We just need to wait for it to finish. */
    uart_tx_wait_blocking(uart_id);
    return UART_OK;
}
 
UART_Status_t UART_FlushRx(uart_inst_t* uart_id) 
{
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;

    /* Drain all characters in the RX FIFO */
    while (uart_is_readable(uart_id)) 
    {
        /* Read and discard a single character from the UART instance (blocking until char has been read) */
        uart_getc(uart_id);
    }
    return UART_OK;
}

bool UART_IsCallbackTriggered(uart_inst_t* uart_id) 
{
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state) return false;

    /* Return the current state of the callback trigger flag */
    return state->rx_callback_triggered;
}

UART_Status_t UART_DisableRxInterrupt(uart_inst_t* uart_id) 
{
    UART_Internal_State_t* state = get_uart_state(uart_id);
    if (!state || !state->initialized) return UART_ERROR_NOT_INITIALIZED;

    /* Disable IRQ in UART and NVIC */
    irq_set_enabled(uart_id == uart0 ? UART0_IRQ : UART1_IRQ, false);
    uart_set_irq_enables(uart_id, false, false);

    /* Reset parameters related to interrupt handling */
    state->interrupt_enabled = false;
    state->rx_callback = NULL;
    state->rx_callback_triggered = false;

    return UART_OK;
}
