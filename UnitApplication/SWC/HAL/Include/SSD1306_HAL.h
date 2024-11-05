// ssd1306.h
#ifndef SSD1306_H
#define SSD1306_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "string.h"
#include <stdint.h>
#include <stdbool.h>

// Display settings
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGES (SSD1306_HEIGHT / 8)

// I2C address
#define SSD1306_I2C_ADDR 0x3C

// Control bytes
#define SSD1306_CONTROL_COMMAND 0x00
#define SSD1306_CONTROL_DATA 0x40

// Basic commands
#define SSD1306_DISPLAY_ON 0xAF
#define SSD1306_DISPLAY_OFF 0xAE
#define SSD1306_CONTRAST 0x81
#define SSD1306_NORMAL_DISPLAY 0xA6
#define SSD1306_INVERT_DISPLAY 0xA7

// Define I2C pins for Raspberry Pi Pico W
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

typedef struct {
    uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
    i2c_inst_t* i2c;
} SSD1306_t;

extern SSD1306_t display;

// Function declarations
void ssd1306_init(SSD1306_t* display, i2c_inst_t* i2c);
void ssd1306_clear(SSD1306_t* display);
void ssd1306_update_display(SSD1306_t* display);
void ssd1306_draw_char(SSD1306_t* display, uint8_t x, uint8_t y, char c);
void ssd1306_draw_string(SSD1306_t* display, uint8_t x, uint8_t y, const char* str);
void ssd1306_draw_pixel(SSD1306_t* display, uint8_t x, uint8_t y, bool on);

#endif // SSD1306_H