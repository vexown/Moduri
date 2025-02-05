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
/*                        STATIC FUNCTION DEFINITIONS                         */
/*******************************************************************************/
static bool calculate_timer_fraction(uint32_t desired_rate, uint16_t *denominator) 
{
    const uint32_t SYSTEM_CLOCK_FREQ = 150000000; // 150MHz
    const uint32_t MIN_TRANSFER_RATE = SYSTEM_CLOCK_FREQ / UINT16_MAX;
    const uint32_t MAX_TRANSFER_RATE = SYSTEM_CLOCK_FREQ;

    if (desired_rate < MIN_TRANSFER_RATE || desired_rate > MAX_TRANSFER_RATE) 
    {
        return false; // Desired rate out of range
    }

    /* Calculate fraction: desired_rate = SYSTEM_CLOCK_FREQ * (num/den)
     * Therefore: num/den = desired_rate/SYSTEM_CLOCK_FREQ
     * (Numinator is assumed to be 1) */
    uint32_t denominator_local = SYSTEM_CLOCK_FREQ / desired_rate;
    if (denominator_local > UINT16_MAX) 
    {
        denominator_local = UINT16_MAX;
    }
    
    *denominator = (uint16_t)denominator_local;
    
    return true;
}

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

    /* In default config this is set to DREQ_FORCE which essentially means no pacing signal (send data as fast as possible) */
    /* If you want to use such signal, you need to configure the transfer request (TREQ) signal to pace the transfer rate */
    channel_config_set_dreq(&channel_config, config->transfer_req_sig);

    /* If the transfer request signal is set to use DMA timer, attempt to claim it and set the multiplier */
    if(config->transfer_req_sig == HAL_DREQ_DMA_TIMER0)
    {
        /* Check if DMA timer 0 is already claimed */
        bool timerClaimed = dma_timer_is_claimed(0);

        if (!timerClaimed)
        {
            dma_timer_claim(0);
        }
        else
        {
            dma_channel_unclaim(channel);
            dma_channels_used[channel] = false;
            return -1;
        }
        
        /* Set the multiplier for DMA timer 0 */
        /* The timer will run at the system_clock_freq (150MHz on RP2350) * numerator / denominator, so this is the speed
           that data elements will be transferred at via a DMA channel using this timer as a DREQ. The multiplier must be less than or equal to one.
           Minimum transfer rate is 150MHz * 1/65536 =~ 2.29kHz. 
           Maximum transfer rate is 150MHz * 1/1 = 150MHz */
        const uint16_t numerator = 1;
        uint16_t denominator;
        if (!calculate_timer_fraction(config->treq_timer_rate_hz, &denominator)) 
        {
            dma_channel_unclaim(channel);
            dma_channels_used[channel] = false;
            return -1;
        }

        dma_timer_set_fraction(0, numerator, denominator);
    }

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