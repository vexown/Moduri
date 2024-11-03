/**
 * File: I2C_HAL.c
 * Description: High-level HAL for easier use of I2C on Raspberry Pico W
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/
#include "FreeRTOS.h"
#include "I2C_HAL.h"
#include <string.h>
#include "Common.h"

// Helper function to get i2c instance
static i2c_inst_t* get_i2c_inst(I2C_Instance instance) {
    return (instance == I2C_INSTANCE_0) ? i2c0 : i2c1;
}

// Validate I2C pins
static bool is_valid_i2c_pins(I2C_Instance instance, uint8_t sda_pin, uint8_t scl_pin) {
    if (instance == I2C_INSTANCE_0) {
        // I2C0 valid pin pairs: GPIO 0/1, 4/5, 8/9, 12/13, 16/17, 20/21
        const uint8_t valid_sda[] = {0, 4, 8, 12, 16, 20};
        const uint8_t valid_scl[] = {1, 5, 9, 13, 17, 21};
        
        for (int i = 0; i < 6; i++) {
            if (sda_pin == valid_sda[i] && scl_pin == valid_scl[i]) {
                return true;
            }
        }
    } else {
        // I2C1 valid pin pairs: GPIO 2/3, 6/7, 10/11, 14/15, 18/19, 26/27
        const uint8_t valid_sda[] = {2, 6, 10, 14, 18, 26};
        const uint8_t valid_scl[] = {3, 7, 11, 15, 19, 27};
        
        for (int i = 0; i < 6; i++) {
            if (sda_pin == valid_sda[i] && scl_pin == valid_scl[i]) {
                return true;
            }
        }
    }
    return false;
}

I2C_Status I2C_Init(const I2C_Config* config) {
    if (config->instance != I2C_INSTANCE_0 && config->instance != I2C_INSTANCE_1) {
        return I2C_ERROR_INVALID_INSTANCE;
    }

    if (!is_valid_i2c_pins(config->instance, config->sda_pin, config->scl_pin)) {
        return I2C_ERROR_INVALID_PINS;
    }

    i2c_inst_t* i2c = get_i2c_inst(config->instance);

    // Initialize I2C peripheral
    i2c_init(i2c, config->speed_hz);
    
    // Set up GPIO pins for I2C
    gpio_set_function(config->sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(config->scl_pin, GPIO_FUNC_I2C);
    
    // Enable pull-ups
    gpio_pull_up(config->sda_pin);
    gpio_pull_up(config->scl_pin);

    return I2C_OK;
}

I2C_Status I2C_WriteByte(I2C_Instance instance, uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    i2c_inst_t* i2c = get_i2c_inst(instance);
    uint8_t buffer[2] = {reg_addr, data};
    
    if (i2c_write_blocking(i2c, dev_addr, buffer, 2, false) == PICO_ERROR_GENERIC) {
        return I2C_ERROR_WRITE_FAILED;
    }
    
    return I2C_OK;
}

I2C_Status I2C_ReadByte(I2C_Instance instance, uint8_t dev_addr, uint8_t reg_addr, uint8_t* data) {
    i2c_inst_t* i2c = get_i2c_inst(instance);
    
    if (i2c_write_blocking(i2c, dev_addr, &reg_addr, 1, true) == PICO_ERROR_GENERIC) {
        return I2C_ERROR_WRITE_FAILED;
    }
    
    if (i2c_read_blocking(i2c, dev_addr, data, 1, false) == PICO_ERROR_GENERIC) {
        return I2C_ERROR_READ_FAILED;
    }
    
    return I2C_OK;
}

I2C_Status I2C_WriteMultiple(I2C_Instance instance, uint8_t dev_addr, uint8_t reg_addr, 
                            const uint8_t* data, size_t length) {
    i2c_inst_t* i2c = get_i2c_inst(instance);
    
    // Use FreeRTOS heap allocation
    uint8_t* buffer = (uint8_t*)pvPortMalloc(length + 1);
    if (buffer == NULL) {
        return I2C_ERROR_WRITE_FAILED;
    }
    
    buffer[0] = reg_addr;
    memcpy(buffer + 1, data, length);
    
    I2C_Status status = I2C_OK;
    if (i2c_write_blocking(i2c, dev_addr, buffer, length + 1, false) == PICO_ERROR_GENERIC) {
        status = I2C_ERROR_WRITE_FAILED;
    }
    
    // Use FreeRTOS heap deallocation
    vPortFree(buffer);
    return status;
}

I2C_Status I2C_ReadMultiple(I2C_Instance instance, uint8_t dev_addr, uint8_t reg_addr, 
                           uint8_t* data, size_t length) {
    i2c_inst_t* i2c = get_i2c_inst(instance);
    
    if (i2c_write_blocking(i2c, dev_addr, &reg_addr, 1, true) == PICO_ERROR_GENERIC) {
        return I2C_ERROR_WRITE_FAILED;
    }
    
    if (i2c_read_blocking(i2c, dev_addr, data, length, false) == PICO_ERROR_GENERIC) {
        return I2C_ERROR_READ_FAILED;
    }
    
    return I2C_OK;
}

I2C_Status I2C_IsDeviceReady(I2C_Instance instance, uint8_t dev_addr) {
    i2c_inst_t* i2c = get_i2c_inst(instance);
    uint8_t dummy_data = 0;
    
    // Attempt to write 1 byte to check for an ACK response
    if (i2c_write_blocking(i2c, dev_addr, &dummy_data, 1, false) == PICO_ERROR_GENERIC) {
        return I2C_ERROR_NO_DEVICE;
    }
    
    return I2C_OK;
}

I2C_Status I2C_ScanBus(I2C_Instance instance, uint8_t* devices, size_t* count) {
    *count = 0;
    
    for (uint8_t addr = 0x08; addr < 0x77; addr++) {
        if (I2C_IsDeviceReady(instance, addr) == I2C_OK) {
            devices[*count] = addr;
            (*count)++;

            LOG("Device found at address: 0x%02X\n", addr);
        }
    }
    
    return I2C_OK;
}
