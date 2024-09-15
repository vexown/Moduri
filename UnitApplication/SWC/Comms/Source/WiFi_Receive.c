#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>
#include "WiFi_Common.h"
#include <errno.h>

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
    if (inet_pton(AF_INET, PICO_W_IP_ADDRESS, &server_addr.sin_addr) <= E_OK) // Convert the IP address from string to binary form
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

    printf("Server is listening on %s:%d\n", PICO_W_IP_ADDRESS, SERVER_PORT);

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












