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
#include "ADC_HAL.h"

/*******************************************************************************/
/*                        GLOBAL FUNCTION DECLARATIONS                         */
/*******************************************************************************/
extern void OS_start( void );

/*******************************************************************************/
/*                        STATIC FUNCTION DECLARATIONS                         */
/*******************************************************************************/

/* Hardware setup function */
static void setupHardware( void );

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
 * Function: main
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

    /* Initialize all 3 ADC0-2 channels */
    ADC_Init(0);
    ADC_Init(1);
    ADC_Init(2);
}

