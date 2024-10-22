/**
 * File: WiFi_Receive.c
 * Description: Implementation related to receiving messages or commands via UDP or TCP.
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/

/* FreeRTOS includes */
#include "FreeRTOS.h"

/* Standard includes */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <string.h>

/* SDK includes */
#include "pico/cyw43_arch.h"
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

/* WiFi includes */
#include "WiFi_Common.h"

/*******************************************************************************/
/*                               DATA TYPES                                    */
/*******************************************************************************/

/* Structure of the TCP server */
typedef struct
{
	/* tcp_pcb is a huge struct defined in lwIP tcp.h file. PCB stands for Protocol Control Block 
	   It is used to manage the state and behavior of TCP connections */
    struct tcp_pcb *server_pcb; 
    struct tcp_pcb *client_pcb;
    bool complete;
} tcpServerType;

/*******************************************************************************/
/*                         STATIC FUNCTION DECLARATIONS                        */
/*******************************************************************************/
static bool tcp_server_open(tcpServerType *tcpServer);
static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err);
static err_t tcp_server_recieve(void *arg, struct tcp_pcb *pcb, struct pbuf *buffer, err_t err);
static void tcp_server_close(tcpServerType *tcpServer);

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
 * Function: start_TCP_server
 * 
 * Description: Starts the TCP server by initializing the tcpServerType structure 
 *              and calling the tcp_server_open function. It handles any necessary 
 *              setup before the server begins accepting connections.
 * 
 * Returns: bool indicating the success (true) or failure (false) of starting the TCP server.
 */
bool start_TCP_server(void) 
{
	bool serverOpenedAndListening = false;
    bool status = false;

	tcpServerType *tcpServer = (tcpServerType*)pvPortCalloc(1, sizeof(tcpServerType)); /* Warning - the server is running forever once started so this memory is not freed */
    if (tcpServer == NULL)
	{
		printf("Failed to allocate memory for the TCP server \n");
        status = false;
    }
	else
	{
		serverOpenedAndListening = tcp_server_open(tcpServer);
		if (serverOpenedAndListening == false) 
		{
			printf("TCP did not successfully open, closing the server and freeing memory... \n");
			tcp_server_close(tcpServer);
			vPortFree(tcpServer);
            status = false;
		}
		else
		{
			printf("TCP server started and listening for incoming connections... \n");
			status = true;
		}
	}
    return status;
}

/*******************************************************************************/
/*                          STATIC FUNCTION DEFINITIONS                        */
/*******************************************************************************/

/* 
 * Function: tcp_server_close
 * 
 * Description: Closes the TCP server and releases any associated resources, 
 *              including the client PCB if it exists.
 * 
 * Parameters:
 *  - tcpServer: Pointer to the tcpServerType structure containing server information.
 * 
 * Returns: void
 */
static void tcp_server_close(tcpServerType *tcpServer) 
{
    /* Check if there is an active client PCB. */
    if (tcpServer->client_pcb != NULL) 
    {
        /* Clear the argument associated with the client PCB to detach the server context. */
        tcp_arg(tcpServer->client_pcb, NULL);

        /* Remove the polling function for the client PCB. */
        tcp_poll(tcpServer->client_pcb, NULL, 0);

        /* Clear the receive callback for the client PCB, stopping any data reception. */
        tcp_recv(tcpServer->client_pcb, NULL);

        /* Clear the error callback for the client PCB. */
        tcp_err(tcpServer->client_pcb, NULL);

        /* Close the client PCB to terminate the connection. */
        tcp_close(tcpServer->client_pcb);

        /* Set the client PCB pointer to NULL to indicate no active client. */
        tcpServer->client_pcb = NULL;
    }

    /* Check if there is an active server PCB. */
    if (tcpServer->server_pcb) 
    {
        /* Clear the argument associated with the server PCB to detach the server context. */
        tcp_arg(tcpServer->server_pcb, NULL);

        /* Close the server PCB to stop accepting new connections. */
        tcp_close(tcpServer->server_pcb);

        /* Set the server PCB pointer to NULL to indicate no active server. */
        tcpServer->server_pcb = NULL;
    }
}

/* 
 * Function: tcp_server_recieve
 * 
 * Description: Callback function that handles incoming data from the TCP client. 
 *              It processes the received message and notifies lwIP about the 
 *              received data.
 * 
 * Parameters:
 *  - arg: User-defined argument passed to the receive callback, typically containing server context.
 *  - pcb: Pointer to the tcp_pcb structure representing the client connection.
 *  - buffer: Pointer to the pbuf containing the received data.
 *  - err: Error status indicating if there was an error during reception.
 * 
 * Returns: err_t indicating the result of the receive operation.
 */
static err_t tcp_server_recieve(void *arg, struct tcp_pcb *pcb, struct pbuf *buffer, err_t err) 
{
    /* Suppress compiler warnings for unused parameters. */
    (void)arg; 
    (void)err;

	err_t status = E_OK;

    /* Check if the received buffer is NULL. */
    if (!buffer) 
    {
        /* Log a message and return if the buffer is NULL. */
        printf("buffer is NULL, returning... \n");
		status = ERR_BUF;
    }
    else
    {
        /* Check the lwIP architecture for any state that needs to be updated. */
        cyw43_arch_lwip_check();

        /* Verify if there is data in the received buffer. */
        if (buffer->tot_len > 0) 
        {
            /* Allocate a buffer to hold the received data plus one byte for the null terminator. */
            char *recv_data = pvPortMalloc(buffer->tot_len + 1);
            if (recv_data) 
            {
                /* Copy the received data from the pbuf to the newly allocated buffer. */
                pbuf_copy_partial(buffer, recv_data, buffer->tot_len, 0);
                
                /* Null-terminate the received string to ensure it's a valid C string. */
                recv_data[buffer->tot_len] = '\0'; 

                /* Print the received message to the console. */
                printf("Received message: %s\n", recv_data);

                /* Free the allocated buffer to prevent memory leaks. */
                vPortFree(recv_data);
            }
            
            /* Inform lwIP that the received data has been processed. */
            tcp_recved(pcb, buffer->tot_len);
        }
        
        /* Free the pbuf to release memory used for the received data. */
        pbuf_free(buffer);
    }

    return status;
}

/* 
 * Function: tcp_server_accept
 * 
 * Description: Callback function that is called when a new client connects 
 *              to the TCP server. It sets up the client PCB and prepares to 
 *              receive data from the client.
 * 
 * Parameters:
 *  - arg: User-defined argument passed to the accept callback, typically containing server context.
 *  - client_pcb: Pointer to the tcp_pcb structure representing the new client connection.
 *  - err: Error status indicating if there was an error during the accept process.
 * 
 * Returns: err_t indicating the result of the accept operation (ERR_OK or ERR_VAL).
 */
static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) 
{
    /* Cast the argument to the expected tcpServerType structure. */
    tcpServerType *tcpServer = (tcpServerType*)arg;
	err_t status = ERR_OK;

    /* Check for errors in the accept callback or if the client PCB is NULL. */
    if (err != ERR_OK || client_pcb == NULL) 
    {
        /* Log a failure message if the connection was not successful. */
        printf("Failure in accept\n");
        status = ERR_VAL; // Return an error value to indicate failure
    }
	else
	{
		/* Log a message indicating that a client has connected successfully. */
		printf("Client connected\n");

		/* Store the client PCB in the tcpServer structure for later use. */
		tcpServer->client_pcb = client_pcb;

		/* Associate the tcpServer context with the client PCB for later reference. */
		tcp_arg(client_pcb, tcpServer);

		/* Set the receive callback function for the client PCB to handle incoming data. */
		tcp_recv(client_pcb, tcp_server_recieve);
	}
    
    /* ERR_OK to indicate successful acceptance of the client connection or ERR_VAL if failed */
    return status;
}

/* 
 * Function: tcp_server_open
 * 
 * Description: Initializes and opens the TCP server, binding it to a specified port 
 *              and setting up a listening state for incoming connections.
 * 
 * Parameters:
 *  - tcpServer: Pointer to the tcpServerType structure that will hold the server PCB.
 * 
 * Returns: bool indicating the success (true) or failure (false) of the server opening.
 */
static bool tcp_server_open(tcpServerType *tcpServer) 
{
    /* Create a new TCP protocol control block (PCB) for the server,
       allowing any IP address type to be used (IPv4 or IPv6). */
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);

    /* Log the starting message with the server's IP address and port. */
    printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);

    /* Check if the PCB was successfully created. */
    if (!pcb) 
    {
        /* Log an error message if PCB creation failed. */
        printf("failed to create pcb\n");
        return false;
    }

    /* Bind the PCB to the specified port. Use NULL to bind to all available interfaces. */
    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err) 
    {
        /* Log an error message if binding to the port failed. */
        printf("failed to bind to port %u\n", TCP_PORT);
        return false;
    }

    /* Set the PCB to listen for incoming connections with a backlog of 1. */
    tcpServer->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!tcpServer->server_pcb) 
    {
        /* Log an error message if setting the server to listen failed. */
        printf("failed to listen\n");
        if (pcb) 
        {
            /* Close the PCB if it was successfully created before failing to listen. */
            tcp_close(pcb);
        }
        return false; 
    }

    /* Associate the tcpServer context with the listening PCB. */
    tcp_arg(tcpServer->server_pcb, tcpServer);

    /* Set the accept callback function for incoming connections. */
    tcp_accept(tcpServer->server_pcb, tcp_server_accept);

    /* Return true to indicate the server has been successfully opened and is listening for incoming connections */
    return true;
}


