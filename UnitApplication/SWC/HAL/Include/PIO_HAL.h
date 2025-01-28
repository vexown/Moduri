#ifndef PIO_HAL_H
#define PIO_HAL_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

/* Test Code - can be used to test the PIO HAL or to get an idea of how to use it */
#if 0
// Test PIO program (Simple square wave generator)
static const uint16_t test_program[] = {
    0xe081, // set pins, 1       ; Set pin high
    0xe080, // set pins, 0       ; Set pin low
    0x0000  // jmp 0            ; Loop forever
};

static bool test_pio_init(void) {
    PIO_HAL_Config_t config = {
        .pio = pio0,
        .sm = 0,
        .program_offset = 0,
        .clock_div = 125.0f,     // 1MHz @ 125MHz system clock
        .pin_base = 28,          // Using GPIO28 for Pico W
        .pin_count = 1,
        .fifo_join_tx = false,
        .fifo_join_rx = false,
        .out_shift_right = true,
        .in_shift_right = true,
        .push_threshold = 32,
        .pull_threshold = 32
    };

    PIO_HAL_Status_t status = PIO_HAL_Init(&config);
    printf("Init test: %s\n", status == PIO_HAL_OK ? "PASS" : "FAIL");
    return status == PIO_HAL_OK;
}

static bool test_program_load(void) {
    PIO_HAL_Status_t status = PIO_HAL_LoadProgram(pio0, test_program, sizeof(test_program)/sizeof(uint16_t));
    printf("Program load test: %s\n", status == PIO_HAL_OK ? "PASS" : "FAIL");
    return status == PIO_HAL_OK;
}

static bool test_fifo_operations(void) {
    // Enable state machine
    PIO_HAL_SM_Enable(pio0, 0, true);
    
    // Test TX FIFO
    PIO_HAL_TX(pio0, 0, 0xAA55AA55);
    bool tx_full = PIO_HAL_TX_IsFull(pio0, 0);
    
    // Test RX FIFO
    bool rx_empty = PIO_HAL_RX_IsEmpty(pio0, 0);
    
    printf("FIFO operations test: %s\n", (!tx_full && rx_empty) ? "PASS" : "FAIL");
    return (!tx_full && rx_empty);
}

static bool test_irq_handling(void) {
    // Enable IRQ
    PIO_HAL_IRQ_Enable(pio0, 0, 0, true);
    
    // Clear and check IRQ
    PIO_HAL_IRQ_Clear(pio0, 0);
    bool irq_status = PIO_HAL_IRQ_Get(pio0, 0);
    
    printf("IRQ handling test: %s\n", !irq_status ? "PASS" : "FAIL");
    return !irq_status;
}

void run_pio_hal_tests(void) {
    printf("\nStarting PIO HAL tests...\n");
    
    bool all_passed = true;
    all_passed &= test_pio_init();
    all_passed &= test_program_load();
    all_passed &= test_fifo_operations();
    all_passed &= test_irq_handling();
    
    // Cleanup
    PIO_HAL_DeInit(pio0, 0);
    
    printf("\nPIO HAL tests %s\n", all_passed ? "PASSED" : "FAILED");
}
#endif

// Status codes
typedef enum {
    PIO_HAL_OK = 0,
    PIO_HAL_ERROR_INVALID_PARAM = -1,
    PIO_HAL_ERROR_NO_FREE_SM = -2,
    PIO_HAL_ERROR_PROGRAM_TOO_LARGE = -3
} PIO_HAL_Status_t;

// Configuration structure
typedef struct {
    PIO pio;                    // PIO block (pio0/pio1)
    uint sm;                    // State machine index (0-3)
    uint program_offset;        // Program offset in instruction memory
    float clock_div;            // Clock divider
    uint pin_base;              // Base GPIO pin number
    uint pin_count;             // Number of pins to use
    bool fifo_join_tx;          // Join TX FIFOs
    bool fifo_join_rx;          // Join RX FIFOs
    bool out_shift_right;       // Shift direction for output
    bool in_shift_right;        // Shift direction for input
    uint push_threshold;        // FIFO push threshold
    uint pull_threshold;        // FIFO pull threshold
} PIO_HAL_Config_t;

typedef struct {
    uint pin_base;
    uint pin_count;
    bool in_use;
} PIO_HAL_PinConfig_t;

// Core functions
PIO_HAL_Status_t PIO_HAL_Init(PIO_HAL_Config_t *config);
void PIO_HAL_DeInit(PIO pio, uint sm);

// Program management
PIO_HAL_Status_t PIO_HAL_LoadProgram(PIO pio, const uint16_t *program, uint length);
void PIO_HAL_ClearProgram(PIO pio, uint offset, uint length);

// State machine control
void PIO_HAL_SM_Enable(PIO pio, uint sm, bool enable);
void PIO_HAL_SM_Restart(PIO pio, uint sm);
void PIO_HAL_SM_ClearFIFOs(PIO pio, uint sm);

// Data transfer
void PIO_HAL_TX(PIO pio, uint sm, uint32_t data);
uint32_t PIO_HAL_RX(PIO pio, uint sm);
bool PIO_HAL_TX_IsFull(PIO pio, uint sm);
bool PIO_HAL_RX_IsEmpty(PIO pio, uint sm);

// IRQ functions
void PIO_HAL_IRQ_Enable(PIO pio, uint sm, uint irq_num, bool enable);
void PIO_HAL_IRQ_Clear(PIO pio, uint irq_num);
bool PIO_HAL_IRQ_Get(PIO pio, uint irq_num);

#endif // PIO_HAL_H