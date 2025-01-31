/**
 * File: PIO_HAL.c
 * Description: High-level HAL for easier use of PIO on Raspberry Pico 2 W
 */

/**
 * PIO Hardware Abstraction Layer
 * 
 * Documentation references:
 * - PIO APIs Reference: SDK Documentation - Hardware PIO: https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#group_hardware_pio
 * - PIO Theory: RP2350 datasheet, PIO chapter: https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf
 * 
 * Overview:
 * A programmable input/output block (PIO) is a versatile hardware interface supporting various IO standards.
 * - RP2040 has two PIO blocks
 * - RP2350 has three PIO blocks (our Raspberry Pico 2 W board uses RP2350)
 * 
 * Each PIO contains four state machines that independently execute sequential programs to manipulate GPIOs
 * and transfer data. These are optimized for deterministic timing and IO operations.
 * 
 * State Machine Features:
 * - Two 32-bit bidirectional shift registers with configurable shift count
 * - Two 32-bit scratch registers
 * - 4x32 bit bus FIFO in each direction (TX/RX), reconfigurable as 8x32 in single direction
 * - Fractional clock divider (16 integer, 8 fractional bits)
 * - Flexible GPIO mapping
 * - DMA interface with sustained throughput up to 1 word per clock
 * - IRQ flag management (set/clear/status)
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/
#include "PIO_HAL.h"

/*******************************************************************************/
/*                        GLOBAL FUNCTION DEFINITIONS                          */
/*******************************************************************************/

PIO_HAL_Status_t PIO_SetupAndRun(PIO_HAL_Config_t *PIO_config)
{
    /* Error checking */
    if (PIO_config == NULL) return PIO_HAL_ERROR_INVALID_PARAM;

    if (PIO_config->program == NULL || PIO_config->program_length == 0) return PIO_HAL_ERROR_INVALID_PARAM;

    if (PIO_config->pio_instance == NULL) return PIO_HAL_ERROR_INVALID_PARAM;

    if (PIO_config->state_machine_num >= 4) return PIO_HAL_ERROR_INVALID_PARAM;

    if (PIO_config->pin_count == 0) return PIO_HAL_ERROR_INVALID_PARAM;

    if (PIO_config->pin_base > 25) return PIO_HAL_ERROR_INVALID_PARAM; //On Raspberry Pico 2 W board GPIO0-GPIO25 can be used for PIO, ADC pins cannot be used for PIO apparently

    /* Attempt to load the program into the PIO instance */
    if(pio_can_add_program(PIO_config->pio_instance, PIO_config->program))
    {
        PIO_config->program_offset = pio_add_program(PIO_config->pio_instance, PIO_config->program);
    }
    else
    {
        return PIO_HAL_ERROR_PROGRAM_TOO_LARGE;
    }

    /**
     * Get the default PIO state machine configuration for the PIO program
     * 
     * This function creates a default state machine config and sets up the wrap
     * boundaries for the program loop. The wrap points are specified relative 
     * to the program's loaded position (offset) in instruction memory.
     * 
     * In PIO programs, wrap defines where the program counter loops back to after
     * reaching the end, creating an infinite execution loop of instructions.
     */
    pio_sm_config sm_config = PIO_config->default_config(PIO_config->program_offset);

    /* Setup the function select for a GPIO to use output from the given PIO instance */
    pio_gpio_init(PIO_config->pio_instance, PIO_config->pin_base);
    /* Use a state machine to set the same pin direction for multiple consecutive pins for the PIO instance */
    pio_sm_set_consecutive_pindirs(PIO_config->pio_instance, PIO_config->state_machine_num, PIO_config->pin_base, PIO_config->pin_count, PIO_config->set_pins_as_output);

    /* Set the 'set' pins in a state machine configuration */
    sm_config_set_set_pins(&sm_config, PIO_config->pin_base, PIO_config->pin_count);

    /* Reset and configure the state machine to a consistent state:
     * - Disables the state machine
     * - Clears the FIFOs
     * - Applies the specified configuration
     * - Resets internal state (shift counters, etc)
     * - Jumps to initial program location
     *
     * Note: State machine remains disabled after this call */
    pio_sm_init(PIO_config->pio_instance, PIO_config->state_machine_num, PIO_config->program_offset, &sm_config);

    /* Set the current clock divider for a state machine */
    pio_sm_set_clkdiv(PIO_config->pio_instance, PIO_config->state_machine_num, PIO_config->clock_div);

    /* Enable a PIO state machine (true = enable, false = disable) */
    pio_sm_set_enabled(PIO_config->pio_instance, PIO_config->state_machine_num, true);

    /* Write a word of data to a state machine's TX FIFO, blocking if the FIFO is full. Here we send the initial delay value */
    pio_sm_put_blocking(PIO_config->pio_instance, PIO_config->state_machine_num, 1000);

    return PIO_HAL_OK;
}