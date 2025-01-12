/**
 * File: ModuriConfig.h
 * Description: Configuration file for the Moduri project. 
 */

#ifndef MODURICONFIG_H
#define MODURICONFIG_H

/* Generic macros for turning features ON or OFF */
#define ON      1
#define OFF     0

/*******************************************************************************/
/*                             COMMUNICATION                                   */
/*******************************************************************************/

/* This option decides if TCP stack on the Pico is set up as TCP server or TCP client */
#define PICO_W_AS_TCP_SERVER   OFF

/* This option decides if HTTP is enabled on the Pico, it is then included within the TCP stack */
#define HTTP_ENABLED           ON

/* Enable this option if you want to set the IP address of the Pico to a static value PICO_W_STATIC_IP_ADDRESS. Otherwise DHCP is used */
#define USE_STATIC_IP          OFF

#endif // MODURICONFIG_H
