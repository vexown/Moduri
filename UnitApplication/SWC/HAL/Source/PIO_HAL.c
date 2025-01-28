#include "PIO_HAL.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"

// Static config storage - 2 PIOs with 4 state machines each
static PIO_HAL_PinConfig_t sm_configs[2][4] = {0};

PIO_HAL_Status_t PIO_HAL_Init(PIO_HAL_Config_t *config) {
    if (!config) return PIO_HAL_ERROR_INVALID_PARAM;
    
    check_pio_param(config->pio);
    check_sm_param(config->sm);
    
    // Store pin configuration
    uint pio_idx = pio_get_index(config->pio);
    sm_configs[pio_idx][config->sm].pin_base = config->pin_base;
    sm_configs[pio_idx][config->sm].pin_count = config->pin_count;
    sm_configs[pio_idx][config->sm].in_use = true;
    
    // Claim state machine
    pio_sm_claim(config->pio, config->sm);
    
    // Get default configuration
    pio_sm_config sm_config = pio_get_default_sm_config();
    
    // Configure clock divider (16.8 fixed point)
    sm_config_set_clkdiv(&sm_config, config->clock_div);
    
    // Configure pins
    sm_config_set_out_pins(&sm_config, config->pin_base, config->pin_count);
    sm_config_set_set_pins(&sm_config, config->pin_base, config->pin_count);
    sm_config_set_in_pins(&sm_config, config->pin_base);
    
    // Configure shift registers
    sm_config_set_out_shift(&sm_config, config->out_shift_right, true, config->pull_threshold);
    sm_config_set_in_shift(&sm_config, config->in_shift_right, true, config->push_threshold);
    
    // Configure FIFOs
    if (config->fifo_join_tx && config->fifo_join_rx) {
        return PIO_HAL_ERROR_INVALID_PARAM;
    }
    
    if (config->fifo_join_tx) {
        sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_TX);
    } else if (config->fifo_join_rx) {
        sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_RX);
    }
    
    // Initialize state machine
    pio_sm_init(config->pio, config->sm, config->program_offset, &sm_config);
    
    // Initialize GPIO pins
    for (uint i = 0; i < config->pin_count; i++) {
        pio_gpio_init(config->pio, config->pin_base + i);
    }
    
    return PIO_HAL_OK;
}

void PIO_HAL_DeInit(PIO pio, uint sm) {
    check_pio_param(pio);
    check_sm_param(sm);
    
    // Disable first
    pio_sm_set_enabled(pio, sm, false);
    
    // Get stored config
    uint pio_idx = pio_get_index(pio);
    PIO_HAL_PinConfig_t *cfg = &sm_configs[pio_idx][sm];
    
    // Reset only pins used by this PIO instance
    if (cfg->in_use) {
        for (uint i = 0; i < cfg->pin_count; i++) {
            gpio_set_function(cfg->pin_base + i, GPIO_FUNC_NULL);
        }
        cfg->in_use = false;
    }
    
    // Finally unclaim state machine
    pio_sm_unclaim(pio, sm);
}


PIO_HAL_Status_t PIO_HAL_LoadProgram(PIO pio, const uint16_t *program, uint length) {
    check_pio_param(pio);
    if (!program) return PIO_HAL_ERROR_INVALID_PARAM;
    
    pio_program_t prog = {
        .instructions = program,
        .length = length,
        .origin = -1  // Relocatable
    };
    
    if (!pio_can_add_program(pio, &prog)) {
        return PIO_HAL_ERROR_PROGRAM_TOO_LARGE;
    }
    
    uint offset = pio_add_program(pio, &prog);
    return offset >= 0 ? PIO_HAL_OK : PIO_HAL_ERROR_PROGRAM_TOO_LARGE;
}

void PIO_HAL_ClearProgram(PIO pio, uint offset, uint length) {
    check_pio_param(pio);
    pio_remove_program(pio, (const pio_program_t*)offset, length);
}

void PIO_HAL_SM_Enable(PIO pio, uint sm, bool enable) {
    check_pio_param(pio);
    check_sm_param(sm);
    pio_sm_set_enabled(pio, sm, enable);
}

void PIO_HAL_SM_Restart(PIO pio, uint sm) {
    check_pio_param(pio);
    check_sm_param(sm);
    pio_sm_restart(pio, sm);
}

void PIO_HAL_SM_ClearFIFOs(PIO pio, uint sm) {
    check_pio_param(pio);
    check_sm_param(sm);
    pio_sm_clear_fifos(pio, sm);
}

void PIO_HAL_TX(PIO pio, uint sm, uint32_t data) {
    check_pio_param(pio);
    check_sm_param(sm);
    pio_sm_put_blocking(pio, sm, data);
}

uint32_t PIO_HAL_RX(PIO pio, uint sm) {
    check_pio_param(pio);
    check_sm_param(sm);
    return pio_sm_get_blocking(pio, sm);
}

bool PIO_HAL_TX_IsFull(PIO pio, uint sm) {
    check_pio_param(pio);
    check_sm_param(sm);
    return pio_sm_is_tx_fifo_full(pio, sm);
}

bool PIO_HAL_RX_IsEmpty(PIO pio, uint sm) {
    check_pio_param(pio);
    check_sm_param(sm);
    return pio_sm_is_rx_fifo_empty(pio, sm);
}

void PIO_HAL_IRQ_Enable(PIO pio, uint sm, uint irq_num, bool enable) {
    check_pio_param(pio);
    check_sm_param(sm);
    pio_set_irqn_source_enabled(pio, irq_num, pis_sm0_rx_fifo_not_empty + sm, enable);
}

void PIO_HAL_IRQ_Clear(PIO pio, uint irq_num) {
    check_pio_param(pio);
    pio_interrupt_clear(pio, irq_num);
}

bool PIO_HAL_IRQ_Get(PIO pio, uint irq_num) {
    check_pio_param(pio);
    return pio_interrupt_get(pio, irq_num);
}