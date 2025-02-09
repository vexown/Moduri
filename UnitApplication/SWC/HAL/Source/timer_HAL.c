#include "timer_HAL.h"
#include "hardware/timer.h"
#include <string.h>

// Internal state tracking
static uint8_t next_timer_id = 0;
#define MAX_SW_TIMERS 8

static repeating_timer_t sw_timer_check;
static bool sw_timer_system_initialized = false;

// Software timer pool
static timer_handle_t* sw_timers[MAX_SW_TIMERS] = {NULL};

// Internal callback for hardware timers
static bool hw_timer_callback(repeating_timer_t* rt) {
    timer_handle_t* handle = (timer_handle_t*)rt->user_data;
    if (handle && handle->config.callback) {
        handle->config.callback();
    }
    return handle->config.repeat;
}

// Internal function to handle software timer updates
static void update_sw_timers(void) {
    uint64_t current_time = time_us_64();
    
    for (int i = 0; i < MAX_SW_TIMERS; i++) {
        if (sw_timers[i] && sw_timers[i]->is_running) {
            // Calculate number of periods that should have elapsed
            uint64_t elapsed = current_time - sw_timers[i]->start_time;
            uint32_t periods = elapsed / sw_timers[i]->config.period_us;
            
            if (periods > 0) {
                // Call callback for each missed period
                for (uint32_t j = 0; j < periods; j++) {
                    if (sw_timers[i]->config.callback) {
                        sw_timers[i]->config.callback();
                    }
                }
                
                // Update start time precisely
                sw_timers[i]->start_time += periods * sw_timers[i]->config.period_us;
                
                if (!sw_timers[i]->config.repeat) {
                    sw_timers[i]->is_running = false;
                    break;
                }
            }
        }
    }
}

static bool sw_timer_check_callback(repeating_timer_t *rt) {
    update_sw_timers();
    return true;  // Keep repeating
}

timer_status_t timer_init(const timer_config_t* config, timer_handle_t* handle) {
    if (!config || !handle) {
        return TIMER_ERROR_INVALID_TIMER;
    }

    // Initialize software timer system if not done yet
    if (!sw_timer_system_initialized && !config->hw_timer) 
    {
        if (!add_repeating_timer_us(100,  // Check every 100µs
                                    sw_timer_check_callback,
                                    NULL,
                                    &sw_timer_check)) 
        {
            return TIMER_ERROR_INVALID_TIMER;
        }
        sw_timer_system_initialized = true;
    }

    if (config->period_us == 0) {
        return TIMER_ERROR_INVALID_PERIOD;
    }
    
    if (!config->callback) {
        return TIMER_ERROR_INVALID_CALLBACK;
    }
    
    // Initialize handle
    memset(handle, 0, sizeof(timer_handle_t));
    handle->timer_id = next_timer_id++;
    handle->config = *config;
    
    // Register software timer if needed
    if (!config->hw_timer) {
        for (int i = 0; i < MAX_SW_TIMERS; i++) {
            if (!sw_timers[i]) {
                sw_timers[i] = handle;
                break;
            }
        }
    }
    
    return TIMER_OK;
}

timer_status_t timer_start(timer_handle_t* handle) {
    if (!handle) {
        return TIMER_ERROR_INVALID_TIMER;
    }
    
    if (handle->is_running) {
        return TIMER_ERROR_ALREADY_RUNNING;
    }
    
    if (handle->config.hw_timer) {
        // Setup hardware timer
        if (!add_repeating_timer_us(
                handle->config.period_us,
                hw_timer_callback,
                handle,
                &handle->hw_timer)) {
            return TIMER_ERROR_INVALID_TIMER;
        }
    } else {
        // Setup software timer
        handle->start_time = time_us_64();
    }
    
    handle->is_running = true;
    return TIMER_OK;
}

timer_status_t timer_stop(timer_handle_t* handle) {
    if (!handle) {
        return TIMER_ERROR_INVALID_TIMER;
    }
    
    if (!handle->is_running) {
        return TIMER_ERROR_NOT_RUNNING;
    }
    
    if (handle->config.hw_timer) {
        cancel_repeating_timer(&handle->hw_timer);
    }
    
    handle->is_running = false;
    return TIMER_OK;
}

timer_status_t timer_reset(timer_handle_t* handle) {
    if (!handle) {
        return TIMER_ERROR_INVALID_TIMER;
    }
    
    timer_status_t status = timer_stop(handle);
    if (status != TIMER_OK) {
        return status;
    }
    
    return timer_start(handle);
}

timer_status_t timer_get_remaining(timer_handle_t* handle, uint32_t* remaining_us) {
    if (!handle || !remaining_us) {
        return TIMER_ERROR_INVALID_TIMER;
    }
    
    if (!handle->is_running) {
        return TIMER_ERROR_NOT_RUNNING;
    }
    
    if (handle->config.hw_timer) {
        // For hardware timers, we can only approximate
        *remaining_us = handle->config.period_us;
    } else {
        uint64_t current_time = time_us_64();
        uint64_t elapsed = current_time - handle->start_time;
        
        if (elapsed >= handle->config.period_us) {
            *remaining_us = 0;
        } else {
            *remaining_us = handle->config.period_us - elapsed;
        }
    }
    
    return TIMER_OK;
}

timer_status_t timer_update_period(timer_handle_t* handle, uint32_t new_period_us) {
    if (!handle) {
        return TIMER_ERROR_INVALID_TIMER;
    }
    
    if (new_period_us == 0) {
        return TIMER_ERROR_INVALID_PERIOD;
    }
    
    bool was_running = handle->is_running;
    
    if (was_running) {
        timer_status_t status = timer_stop(handle);
        if (status != TIMER_OK) {
            return status;
        }
    }
    
    handle->config.period_us = new_period_us;
    
    if (was_running) {
        return timer_start(handle);
    }
    
    return TIMER_OK;
}