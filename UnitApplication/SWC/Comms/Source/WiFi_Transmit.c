/**
 * File: WiFi_Transmit.c
 * Description: Implementation related to sending messages or commands via UDP or TCP.
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/

/* Standard includes. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* SDK includes */
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>

/* WiFi includes */
#include "WiFi_Common.h"

/*******************************************************************************/
/*                          GLOBAL FUNCTION DEFINITIONS                        */
/*******************************************************************************/

/* 
 * Function: send_message_TCP
 * 
 * Description: Sends a message over a TCP socket to a specified IP address and port.
 * 
 * Parameters:
 *   - message: The message to send (string)
 * 
 * Returns: void
 */
void send_message_TCP(const char* message) 
{
    int bytes_sent;
    int client_socket;
    struct sockaddr_in server_addr;
    uint32_t ip_addr_net_order;
    
    /* Create a socket for Pico W (client) */
    /* This is a wrapper around the netconn_new function that creates a new network connection 
       and associates it with a socket descriptor. It takes three arguments: 
            the domain (usually AF_INET for IPv4), 
            the type of socket (such as SOCK_STREAM for TCP or SOCK_DGRAM for UDP), 
            and the protocol (e.g., IPPROTO_TCP or IPPROTO_UDP) - in our case when we use SOCK_STREAM this parameter is unused (TCP implied). 
    The function then creates a netconn object based on the specified parameters, allocates a socket descriptor, and associates the netconn with the socket */
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0 /*unused*/)) == ERRNO_FAIL) 
    {
        printf("ERROR opening socket: %s\n", strerror(errno));
        return;
    }

    /* Set up the server address */
    memset((char *)&server_addr, 0, sizeof(server_addr)); // start with 0'd out parameters
    server_addr.sin_family = AF_INET; //server uses IPv4
    server_addr.sin_port = htons(SERVER_PORT);
    ip_addr_net_order = inet_addr(PC_IP_ADDRESS); //converts ASCII IP address to IP address in network order
    if(ip_addr_net_order != IPADDR_NONE)
    {
        server_addr.sin_addr.s_addr = ip_addr_net_order;
    }
    else
    {
        printf("Invalid IP address \n");
    }
    
    /* Establish a connection between the client and the server TCP socket */ 
    /* It takes three arguments: the socket descriptor, the server address structure, and the length of the server address structure. 
        The function first checks if the socket is valid and if the remote address matches the socket type (IPv4 or IPv6). 
        It then extracts the remote IP address and port from the address structure and calls the netconn_connect function to establish the connection. */
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) != E_OK) 
    {
        printf("ERROR connecting \n");
    }

    /* Send the message */
    /* The lwip_send function in LwIP is a wrapper around the netconn_write_partly function that sends data over a TCP socket. 
       It takes four arguments: the socket descriptor, the data to be sent, the size of the data, and flags that specify additional options. */
    bytes_sent = send(client_socket, message, strlen(message), NO_FLAG);
    if (bytes_sent == ERRNO_FAIL) //send function returns either number of bytes sent or -1 if something went wrong
    {
        printf("ERROR writing to socket \n");
    }
    else if(bytes_sent != strlen(message))
    {
        printf("ERROR - not all bytes of the message were sent \n");
    }

    /* Close the socket */
    close(client_socket);
}

/* 
 * Function: send_message_UDP
 * 
 * Description: Sends a message over a UDP socket to a specified IP address and port.
 * 
 * Parameters:
 *   - message: The message to send (string)
 * 
 * Returns: void
 */
void send_message_UDP(const char* message) 
{
    int bytes_sent;
    int client_socket;
    struct sockaddr_in server_addr;
    uint32_t ip_addr_net_order;

    /* Create a socket for Pico W (client) */
    if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0 /*unused*/)) == ERRNO_FAIL) 
    {
        printf("ERROR opening socket: %s\n", strerror(errno));
        return;
    }

    /* Set up the server address */
    memset(&server_addr, 0, sizeof(server_addr)); // start with 0'd out parameters
    server_addr.sin_family = AF_INET; //server uses IPv4
    server_addr.sin_port = htons(SERVER_PORT);
    ip_addr_net_order = inet_addr(PC_IP_ADDRESS); //converts ASCII IP address to IP address in network order
    if(ip_addr_net_order != IPADDR_NONE)
    {
        server_addr.sin_addr.s_addr = ip_addr_net_order;
    }
    else
    {
        printf("Invalid IP address \n");
    }

    /* Send the message */
    /* The lwip_sendto function sends data to a specific address using a socket. Used for UDP communication using the LwIP stack.
       Uses netconn_send to actually send the data to the specified destination address.
       It takes 6 arguments: the socket descriptor, the data to be sent, the size of the data, flags that specify additional options, destination addr and addr length */
    bytes_sent = sendto(client_socket, message, strlen(message), NO_FLAG, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bytes_sent == ERRNO_FAIL) //send function returns either number of bytes sent or -1 if something went wrong
    {
        printf("ERROR writing to socket \n");
    }
    else if(bytes_sent != strlen(message))
    {
        printf("ERROR - not all bytes of the message were sent \n");
    }

    /* Close the socket */
    close(client_socket);
}
