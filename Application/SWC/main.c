
/* Library includes. */
#include <stdio.h>
#include "pico/stdlib.h"

/*-----------------------------------------------------------*/
/* Hardware setup function */
static void prvSetupHardware( void );
/* OS function which does some set up and starts the scheduler */
extern void OS_start( void );

/*-----------------------------------------------------------*/

int main( void )
{
    /* Configure and initialize Raspberry Pico W hardware */
    prvSetupHardware();

    /* Go to the main C file which does some setup and starts the scheduler */
    OS_start();
}

/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
    stdio_init_all();
}

