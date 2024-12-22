/**
 * File: main.c
 * Description: First executed user code after crt0.S assembly startup code defined
 * in the pico-sdk (located in pico-sdk/src/rp2_common/pico_crt0/crt0.S)
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/

/* Library includes. */
#include <stdio.h>
#include "pico/stdlib.h"

/* HAL includes */
#include "GPIO_HAL.h"

/* Common includes */
#include "WiFi_Common.h"

/*******************************************************************************/
/*                        GLOBAL FUNCTION DECLARATIONS                         */
/*******************************************************************************/
extern void OS_start( void );

/*******************************************************************************/
/*                        STATIC FUNCTION DECLARATIONS                         */
/*******************************************************************************/

/* Hardware setup function */
static void setupHardware( void );

/* Function to open the lid */
static void open_lid(void);

/* Function to close the lid */
static void close_lid(void);

/* Function to flash the LED */
static void flash_led(int times, int delay_ms);

/*******************************************************************************/
/*                          GLOBAL FUNCTION DEFINITIONS                        */
/*******************************************************************************/

/* 
 * Function: main
 * 
 * Description: First user code function, does initial HW setup and calls the OS
 *              start function
 * 
 * Parameters:
 *   - none
 * 
 * Returns: technically 'int' value but the program is a loop we will never return
 */
int main(void)
{
    setupHardware();
    
    OS_start();
}

/*******************************************************************************/
/*                         STATIC FUNCTION DEFINITIONS                         */
/*******************************************************************************/

/* 
 * Function: setupHardware
 * 
 * Description: Configure and initialize Raspberry Pico W hardware 
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 */
static void setupHardware(void)
{
    stdio_init_all();

    // Initialize end switch pin as input with pull-up
    GPIO_Init(GPIO_END_SWTICH, GPIO_INPUT, GPIO_PULL_UP);
    GPIO_Init(GPIO_LED, GPIO_OUTPUT, GPIO_PULL_DOWN);
    GPIO_Init(GPIO_MOTOR_UP, GPIO_OUTPUT, GPIO_PULL_DOWN);
    GPIO_Init(GPIO_MOTOR_DOWN, GPIO_OUTPUT, GPIO_PULL_DOWN);

    //Set initial states of the pins
    GPIO_Clear(GPIO_LED);
    GPIO_Clear(GPIO_MOTOR_UP);
    GPIO_Clear(GPIO_MOTOR_DOWN);

    GPIO_State button_state;
    GPIO_Read(GPIO_END_SWTICH, &button_state);

    /* During startup, let's check if the lid is closed or open and test the mechanism
       This way we know the opening and closing works and we end up in a known starting state */
    if (button_state == GPIO_LOW) 
    {  // End switch pressed - lid is currently closed
        open_lid();
        flash_led(1, 1000); // Flash the LEDs with lid open to make sure LEDs are working
        close_lid();
    } 
    else 
    { // End switch released - lid is currently open
        close_lid();
        open_lid();
        flash_led(1, 1000); // Flash the LEDs with lid open to make sure LEDs are working
        close_lid();
    }

}

/* 
 * Function: open_lid
 * 
 * Description: Opens the lid by setting the motor up pin and waiting until the end switch is released
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 */
static void open_lid(void)
{
    GPIO_State button_state;
    GPIO_Read(GPIO_END_SWTICH, &button_state);

    GPIO_Set(GPIO_MOTOR_UP);
    while(button_state == GPIO_LOW) // Move the lid up until the end switch is released
    {
        GPIO_Read(GPIO_END_SWTICH, &button_state);
    }
    sleep_ms(OPENING_TIME); // Open the lid fully
    GPIO_Clear(GPIO_MOTOR_UP);
}

/* 
 * Function: close_lid
 * 
 * Description: Closes the lid by setting the motor down pin and waiting until the end switch is pressed
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 */
static void close_lid(void)
{
    GPIO_State button_state;
    GPIO_Read(GPIO_END_SWTICH, &button_state);

    GPIO_Set(GPIO_MOTOR_DOWN);
    while(button_state == GPIO_HIGH) // Move the lid down until the end switch is pressed
    {
        GPIO_Read(GPIO_END_SWTICH, &button_state);
    }
    sleep_ms(CLOSING_TIME); // Wait for the lid to fully close (experimentally determined)
    GPIO_Clear(GPIO_MOTOR_DOWN);
}

/* 
 * Function: flash_led
 * 
 * Description: Flashes the LED a specified number of times with a specified delay
 * 
 * Parameters:
 *   - times: Number of times to flash the LED
 *   - delay_ms: Delay in milliseconds between flashes
 * 
 * Returns: void
 */
static void flash_led(int times, int delay_ms)
{
    for (int i = 0; i < times; i++)
    {
        GPIO_Set(GPIO_LED);
        sleep_ms(delay_ms);
        GPIO_Clear(GPIO_LED);
        sleep_ms(delay_ms);
    }
}