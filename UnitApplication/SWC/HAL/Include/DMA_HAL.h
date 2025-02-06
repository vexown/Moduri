/**
 * @file DMA_HAL.h
 * @brief Header file for high-level DMA driver for RP2350
 * @details Provides interface declarations for simplified DMA operations
 */

#ifndef DMA_HAL_H
#define DMA_HAL_H

#if 0 /* Test Code - you can use this to test the DMA HAL or as a reference to get an idea how to use it */

#include <stdio.h>
#include "pico/stdlib.h"
#include "DMA_HAL.h"
#include <string.h>

// Test data
static const char __attribute__((aligned(4))) test_src[] = "Hello, world! (from DMA HAL)";
static char __attribute__((aligned(4))) test_dst[32];
static uint8_t test_channel = 0;

// Test functions
static void test_dma_init(void) {
    printf("\nTesting DMA_Init...\n");
    
    DMA_Config_t config = {
        .dest_addr = test_dst,
        .src_addr = test_src,
        .transfer_count = sizeof(test_src),
        .data_size = DMA_SIZE_8,
        .src_increment = true,
        .dst_increment = true,
        .transfer_req_sig = HAL_DREQ_FORCE,
        .treq_timer_rate_hz = 0, // Not used with HAL_DREQ_FORCE
        .enableIRQ0 = true
    };

    test_channel = DMA_Init(&config);
    printf("DMA_Init returned channel: %d\n", test_channel);
    assert(test_channel >= 0);
}

static void test_dma_transfer(void) {
    printf("\nTesting DMA transfer...\n");
    
    // Clear destination buffer
    memset(test_dst, 0, sizeof(test_dst));
    
    // Start transfer
    bool start_ok = DMA_Start(test_channel);
    assert(start_ok);
    printf("DMA transfer started\n");
    
    // Wait for completion with timeout
    bool wait_ok = DMA_WaitComplete(test_channel);
    if (!wait_ok) {
        printf("DMA transfer timeout!\n");
        assert(false);
        return;
    }
    printf("DMA transfer complete\n");
    
    // Add small delay to ensure DMA finished
    sleep_ms(1);
    
    // Verify transfer
    if (memcmp(test_src, test_dst, strlen(test_src)) == 0) {
        printf("Transfer verified: %s\n", test_dst);
    } else {
        printf("Transfer verification failed!\n");
        assert(false);
    }
}

static void test_dma_error_handling(void) {
    printf("\nTesting error handling...\n");
    
    // Test invalid channel
    assert(DMA_Start(DMA_MAX_CHANNELS + 1) == false);
    
    // Test NULL config
    assert(DMA_Init(NULL) == -1);
}

static void test_dma_timer_treq(void) {
    printf("\nTesting DMA Timer TREQ (Timer 0) configuration...\n");
    
    /* Test data */
    static const char __attribute__((aligned(4))) test_src[] = "DMA Timer TREQ";
    static char __attribute__((aligned(4))) test_dst[32] = {0};
    
    /* Configure DMA to use DMA Timer0 as pacing signal */
    DMA_Config_t config = {
        .dest_addr = test_dst,
        .src_addr = test_src,
        .transfer_count = sizeof(test_src),
        .data_size = DMA_SIZE_8,
        .src_increment = true,
        .dst_increment = true,
        .transfer_req_sig = HAL_DREQ_DMA_TIMER0,  // Use Timer 0 as TREQ source
        .treq_timer_rate_hz = 10000, // 10kHz TREQ rate
        .enableIRQ0 = true
    };
    
    int8_t channel = DMA_Init(&config);
    printf("DMA_Init returned channel: %d\n", channel);
    assert(channel >= 0);
    
    bool start_ok = DMA_Start(channel);
    assert(start_ok);
    printf("DMA transfer started (Timer TREQ)...\n");
    
    bool wait_ok = DMA_WaitComplete(channel);
    assert(wait_ok);
    printf("DMA transfer complete (Timer TREQ)...\n");
    
    /* Verify the transfer */
    if (memcmp(test_src, test_dst, strlen(test_src)) == 0)
        printf("Transfer verified (Timer TREQ): %s\n", test_dst);
    else {
       printf("Transfer verification failed (Timer TREQ)!\n");
       assert(false);
    }
    
    DMA_Release(channel);
    printf("DMA Timer TREQ test complete\n");
}

int main(void)
{
    setupHardware();

    printf("\nStarting DMA HAL tests...\n");

    // Run tests
    test_dma_init();
    test_dma_transfer();
    test_dma_error_handling();
    test_dma_timer_treq();
    
    printf("\nDMA HAL tests completed!\n");

    while(1);

    OS_start();
}

#endif

#include "hardware/dma.h"
#include "hardware/sync.h"
#include "pico/time.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief DMA-related macros
 */
#define DMA_MAX_CHANNELS 12     /* Maximum number of DMA channels */
#define DMA_TIMEOUT_MS 1000     /* Timeout for DMA operations in milliseconds */
#define DMA_TIMING_PIN 16       /* GPIO pin for measuring DMA complete transfer time - can be used with a Logic Analyzer */ 

/**
 * @brief Reduced version of dreq_num_t enum from pico-sdk
 */
typedef enum hal_dreq_num 
{
    HAL_DREQ_DMA_TIMER0 = 59,  ///< Select DMA_TIMER0 as DREQ
    HAL_DREQ_FORCE = 63        ///< Select FORCE as DREQ
} hal_dreq_num_t;

/**
 * @brief Configuration structure for DMA channel initialization
 */
typedef struct 
{
    void* dest_addr;                 /*!< Destination address for DMA transfer */
    const void* src_addr;            /*!< Source address for DMA transfer */
    uint32_t transfer_count;         /*!< Number of transfers to perform */
    uint8_t data_size;               /*!< Size of each transfer (4 bytes - full word, 2 bytes - half word, 1 byte - byte) */
    bool src_increment;              /*!< Whether to increment source address after each transfer */
    bool dst_increment;              /*!< Whether to increment destination address after each transfer */
    hal_dreq_num_t transfer_req_sig; /*!< Transfer Request (TREQ) Signal to pace the transfer rate. Use HAL_DREQ_FORCE for unpaced transfers */
    uint32_t treq_timer_rate_hz;     /*!< Timer-based TREQ rate in Hz, from 2288Hz to 150MHz (use only with transfer_req_sig set to HAL_DREQ_DMA_TIMER0) */
    bool enableIRQ0;                    /*!< Enable debug output */
} DMA_Config_t;

/**
 * @brief Initialize a DMA channel with specified configuration
 * @param config Pointer to DMA configuration structure
 * @return Channel number if successful, -1 if error
 */
int8_t DMA_Init(DMA_Config_t *config);

/**
 * @brief Start DMA transfer on specified channel
 * @param channel DMA channel number
 * @return true if successful, false if error
 */
bool DMA_Start(uint8_t channel);

/**
 * @brief Wait for DMA transfer to complete
 * @param channel DMA channel number
 * @return true if successful, false if timeout or error
 */
bool DMA_WaitComplete(uint8_t channel);

/**
 * @brief Release a DMA channel
 * @param channel DMA channel number
 */
void DMA_Release(uint8_t channel);

/**
 * @brief Check if DMA transfer is complete
 * @param channel DMA channel number
 * @return true if transfer is complete, false otherwise
 */
bool DMA_IsTransferComplete(uint8_t channel);

/**
 * @brief Abort a DMA transfer
 * @param channel DMA channel number
 * @return true if successful, false if error
 */
bool DMA_Abort(uint8_t channel);

#endif /* DMA_HAL_H */