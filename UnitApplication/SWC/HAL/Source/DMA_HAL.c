/**
 * @file DMA_HAL.c
 * @brief High-level DMA driver for RP2350
 * @details Provides simplified DMA operations with error handling and safety checks
 */

/**
 * The RP-series microcontroller Direct Memory Access (DMA) master performs bulk data transfers on a processor’s
 * behalf. This leaves processors free to attend to other tasks, or enter low-power sleep states. The
 * data throughput of the DMA is also significantly higher than one of RP-series microcontroller’s processors.
 *
 * The DMA can perform one read access and one write access, up to 32 bits in size, every clock cycle.
 * There are 12 independent channels, which each supervise a sequence of bus transfers, usually in
 * one of the following scenarios:
 *      - Memory to peripheral
 *      - Peripheral to memory
 *      - Memory to memory
 * 
 * More info at Chapter 10.7 of RP2350 Datasheet: https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf
 * 
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/
#include "DMA_HAL.h"

/*******************************************************************************/
/*                             STATIC VARIABLES                                */
/*******************************************************************************/
static bool dma_channels_used[DMA_MAX_CHANNELS] = {false};

/*******************************************************************************/
/*                        GLOBAL FUNCTION DEFINITIONS                          */
/*******************************************************************************/
/**
 * @brief Initialize a DMA channel with specified configuration
 * @param config Pointer to DMA configuration structure
 * @return Channel number if successful, -1 if error
 */
int8_t DMA_Init(DMA_Config_t *config) 
{
    if (config == NULL) return -1;

    /* Attempt to claim an unused DMA channel, return -1 if none available */
    int channel = dma_claim_unused_channel(false);
    if (channel < 0) return -1;

    /* Mark channel as used */
    dma_channels_used[channel] = true;

    /* Get the default channel configuration for a given channel */
    dma_channel_config channel_config = dma_channel_get_default_config(channel);

    /* Now modify the default configuration to match the user's requirements */
    channel_config_set_transfer_data_size(&channel_config, config->data_size);

    /* Set the increment mode for source and destination addresses - this will determine 
       whether the addresses are incremented by the data size after each transfer */
    channel_config_set_read_increment(&channel_config, config->src_increment);
    channel_config_set_write_increment(&channel_config, config->dst_increment);

    /* Configure the DMA channel with the specified parameters */
    dma_channel_configure(
        channel,
        &channel_config,
        config->dest_addr,
        config->src_addr,
        config->transfer_count,
        false
    );

    /* Return the channel number - it is our handle for DMA operations */
    return channel;
}

/**
 * @brief Start DMA transfer on specified channel
 * @param channel DMA channel number
 * @return true if successful, false if error
 */
bool DMA_Start(uint8_t channel) 
{
    if (channel >= DMA_MAX_CHANNELS || !dma_channels_used[channel]) return false;
    
    dma_channel_start(channel);
    return true;
}

/**
 * @brief Wait for DMA transfer to complete
 * @param channel DMA channel number
 * @return true if successful, false if timeout or error
 */
bool DMA_WaitComplete(uint8_t channel) 
{
    if (channel >= DMA_MAX_CHANNELS || !dma_channels_used[channel]) return false;

    dma_channel_wait_for_finish_blocking(channel);
    return true;
}

/**
 * @brief Check if DMA transfer is complete
 * @param channel DMA channel number
 * @return true if transfer is complete, false otherwise
 */
bool DMA_IsTransferComplete(uint8_t channel) 
{
    if (channel >= DMA_MAX_CHANNELS || !dma_channels_used[channel]) return false;
    
    return !dma_channel_is_busy(channel);
}

/**
 * @brief Abort a DMA transfer
 * @param channel DMA channel number
 * @return true if successful, false if error
 */
bool DMA_Abort(uint8_t channel)
{
    if (channel >= DMA_MAX_CHANNELS || !dma_channels_used[channel]) return false;
    
    dma_channel_abort(channel);

    return true;
}

/**
 * @brief Release a DMA channel
 * @param channel DMA channel number
 */
void DMA_Release(uint8_t channel) 
{
    if (channel < DMA_MAX_CHANNELS && dma_channels_used[channel]) 
    {
        /* Abort any ongoing transfer */
        dma_channel_abort(channel);

        /* Unclaim the channel and mark it as unused */
        dma_channel_unclaim(channel);
        dma_channels_used[channel] = false;
    }
}