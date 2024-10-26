/**
 * File: WiFi_UDP.c
 * Description: High level implementation for lwIP-based UDP communication between Unit and Central Application.
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
 * Function: receive_message_UDP
 * 
 * Description: Receives a message over a UDP socket (from the PC central app)
 * 
 * Parameters:
 *  - buffer: stores the received message
 *  - buffer_size: size of the message buffer
 * 
 * Returns: void
 */
void receive_message_UDP(char* buffer, int buffer_size) 
{
    int bytes_received;
    int server_socket;
    struct sockaddr_in client_addr, server_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct timeval timeout;

    /* Clear out the message buffer */
    memset(buffer, 0, sizeof(buffer));

    /* Create a UDP socket for the server running on Pico W */
    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0 /*unused*/)) == ERRNO_FAIL) 
    {
        printf("ERROR opening socket: %s\n", strerror(errno));
        return;
    }

    /* Set up the server address (IP and port) */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, PICO_W_STATIC_IP_ADDRESS, &server_addr.sin_addr) <= E_OK) // Convert the IP address from string to binary form
    {
        printf("Invalid IP address format\n");
        close(server_socket);
        return;
    }

    /* Bind the server socket to the specified IP address and port */
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == ERRNO_FAIL) 
    {
        printf("Binding failed: %d\n", errno);
        close(server_socket);
        return;
    }

    printf("Server is listening on %s:%d\n", PICO_W_STATIC_IP_ADDRESS, SERVER_PORT);

    /* Set the socket timeout */
    timeout.tv_sec = 0;  // Set seconds timeout
    timeout.tv_usec = 100000; // Set microseconds timeout (100 ms)
    setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    /* Receive the message if there is one available */
    if ((bytes_received = recvfrom(server_socket, buffer, buffer_size, NO_FLAG, (struct sockaddr *)&client_addr, &client_addr_len)) == ERRNO_FAIL) 
    {
        printf("Message not received (possibly timeout if errno 0): %s (errno: %d)\n", strerror(errno), errno);
    } 
    else /* all bytes received successfully */
    {
        /* Null-terminate the received message */
        if (bytes_received > 0) 
        {
            buffer[bytes_received] = '\0';
            printf("Received UDP message: %s\n", buffer);
        }
    }

    /* Close the socket */
    close(server_socket);
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
