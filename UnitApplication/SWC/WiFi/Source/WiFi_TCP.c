/**
 * File: WiFi_TCP.c
 * Description: High level implementation for lwIP-based TCP communication between Unit and Central Application.
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "semphr.h"

/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
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
#include "lwip/err.h"

/* WiFi includes */
#include "WiFi_Common.h"
#include "WiFi_HTTP.h"
#include "WiFi_DHCP_server.h"
#include "WiFi_DNS_server.h"

/* Misc includes */
#include "Common.h"

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
    uint8_t receive_buffer[TCP_RECV_BUFFER_SIZE]; // Buffer for incoming data
    bool complete;
    ip_addr_t gw;
} tcpServerType;

/* Basic TCP client structure */
typedef struct {
    struct tcp_pcb *pcb;                      // The TCP protocol control block - lwIP's main structure for a TCP connection
    uint8_t receive_buffer[TCP_RECV_BUFFER_SIZE]; // Buffer for incoming data
    uint16_t receive_length;                  // Length of received data
    bool is_connected;                        // Connection status
} TCP_Client_t;

/*******************************************************************************/
/*                               STATIC VARIABLES                              */
/*******************************************************************************/
static tcpServerType *tcpServerGlobal;
static TCP_Client_t *clientGlobal;
static SemaphoreHandle_t bufferMutex;

/*******************************************************************************/
/*                         STATIC FUNCTION DECLARATIONS                        */
/*******************************************************************************/
#if (PICO_W_AS_TCP_SERVER == ON)
static bool tcp_server_open(tcpServerType *tcpServer);
static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err);
static err_t tcp_server_recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *buffer, err_t err);
static void tcp_server_close(tcpServerType *tcpServer);
#else
static err_t tcp_client_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_client_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err);
TCP_Client_t* tcp_client_init(void);
#endif

/*******************************************************************************/
/*                          GLOBAL FUNCTION DEFINITIONS                        */
/*******************************************************************************/

#if (PICO_W_AS_TCP_SERVER == ON)
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

	tcpServerGlobal = (tcpServerType*)pvPortCalloc(1, sizeof(tcpServerType)); /* Warning - the server is running forever once started so this memory is not freed */
    if (tcpServerGlobal == NULL)
	{
		LOG("Failed to allocate memory for the TCP server \n");
        status = false;
    }
	else
	{
        ip4_addr_t mask;
        IP4_ADDR(ip_2_ip4(&tcpServerGlobal->gw), 192, 168, 4, 1);
        IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

        // Start the dhcp server TODO - figure out DHCP failure when attempting to connect
        dhcp_server_t dhcp_server;
        dhcp_server_init(&dhcp_server, &tcpServerGlobal->gw, &mask);

        // Start the dns server
        dns_server_t dns_server;
        dns_server_init(&dns_server, &tcpServerGlobal->gw);

		serverOpenedAndListening = tcp_server_open(tcpServerGlobal);
		if (serverOpenedAndListening == false) 
		{
			LOG("TCP did not successfully open, closing the server and freeing memory... \n");
			tcp_server_close(tcpServerGlobal);
			vPortFree(tcpServerGlobal);
            status = false;
		}
		else
		{
			LOG("TCP server started and listening for incoming connections... \n");
			status = true;
		}
	}

    bufferMutex = xSemaphoreCreateMutex();
    if (bufferMutex == NULL) 
    {
        LOG("Failed to create mutex\n");
        status = false;
    }

    return status;
}

/* 
 * Function: tcp_server_process_recv_message
 * 
 * Description: 
 *      Prints the received buffer and analyzes its content.
 *      Checks if the buffer starts with "cmd:".
 *      If it does, it extracts the numeric value following "cmd:"
 *      and stores it as the received command.
 * 
 * Parameters:
 *  - received_command: Pointer to store the extracted command value
 * 
 * Returns: 
 *  - void
 */
void tcp_server_process_recv_message(uint8_t *received_command) 
{
    // Wait for the mutex before accessing the buffer
    if (xSemaphoreTake(bufferMutex, NO_TIMEOUT) == pdTRUE) 
    {
        // Access the receive buffer
        uint8_t *buffer = tcpServerGlobal->receive_buffer;

        // Print the entire received buffer
        LOG("Received message: %s\n", buffer);

        // Check if the buffer starts with "cmd:"
        if (strncmp((const char *)buffer, "cmd:", 4) == 0) 
        {
            // Extract the number after "cmd:"
            // Explanation of atoi return value:
            // - If the string after "cmd:" is non-numeric (e.g., "cmd:abc"), atoi will return 0.
            // - If there is nothing after "cmd:" (e.g., "cmd:"), atoi will return 0.
            // - If the string after "cmd:" is explicitly "0", atoi will return 0 as a valid value.
            int command_value = atoi((const char *)&buffer[4]);

            // Ensure the command value is within 0-255 range
            if (command_value >= 0 && command_value <= 255) 
            {
                *received_command = (uint8_t)command_value;
                LOG("Received command: %u\n", *received_command);
            } 
            else 
            {
                LOG("Command value out of range (0-255).\n");
            }
        } 
        else 
        {
            LOG("No command found in received message.\n");
        }

        // Clear the buffer after processing
        memset(tcpServerGlobal->receive_buffer, 0, sizeof(tcpServerGlobal->receive_buffer));

        // Release the mutex after access
        xSemaphoreGive(bufferMutex); 
    } 
    else 
    {
        LOG("Failed to acquire the pointer to receive buffer.\n");
    }
}

/* 
 * Function: tcp_server_send
 * 
 * Description: 
 *      Checks if the TCP server has an active client connection. If connected, 
 *      it uses tcp_write() to queue the data for sending and then uses 
 *      tcp_output() to actually send the queued data over the network.
 * 
 * Parameters:
 *  - const char *data: Pointer to the data buffer to be sent.
 *  - uint16_t length: Length of the data to be sent.
 * 
 * Returns: 
 *  - err_t: An error code indicating the success (ERR_OK) or failure 
 *            (e.g., ERR_CONN if no client is connected or other 
 *            error codes from tcp_write() or tcp_output()) of the send operation.
 */
err_t tcp_server_send(const char *data, uint16_t length) 
{
    err_t err = ERR_OK;
    
    /* Check if tcpServerGlobal exists and has an active client connection */
    if (tcpServerGlobal == NULL || tcpServerGlobal->client_pcb == NULL) 
    {
        LOG("Cannot send - no client connected\n");
        return ERR_CONN;
    }
    
    /* Send data */
    err = tcp_write(tcpServerGlobal->client_pcb, data, length, TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK) 
    {
        /* Actually send the data */
        err = tcp_output(tcpServerGlobal->client_pcb);
        if (err != ERR_OK) 
        {
            LOG("tcp_output failed: %d\n", err);
        }
    } 
    else 
    {
        LOG("tcp_write failed: %d\n", err);
    }
    
    return err;
}

#else
/* 
 * Function: start_TCP_client
 * 
 * Description: Starts the TCP client and reports the status
 * 
 * Returns: bool indicating the success (true) or failure (false) of starting the TCP client.
 */
bool start_TCP_client(void) 
{
    clientGlobal = tcp_client_init();
    bool status = false;

    if (clientGlobal != NULL) {
        LOG("TCP client initialized successfully\n");
        status = true;
    } else {
        LOG("TCP client initialization failed\n");
    }

    bufferMutex = xSemaphoreCreateMutex();
    if (bufferMutex == NULL) 
    {
        LOG("Failed to create mutex\n");
        status = false;
    }
    
    return status;
}

/* 
 * Function: tcp_client_send
 * 
 * Description: 
 *      Checks if the TCP client is connected. If connected, it uses tcp_write() 
 *      to queue the data for sending and then uses tcp_output() to actually send 
 *      the queued data over the network.
 * 
 * Parameters:
 *  - const char *data: Pointer to the data buffer to be sent.
 *  - uint16_t length: Length of the data to be sent.
 * 
 * Returns: 
 *  - err_t: An error code indicating the success (ERR_OK) or failure 
 *            (e.g., ERR_CONN if the client is not connected or other 
 *            error codes from tcp_write() or tcp_output()) of the send operation.
 */
err_t tcp_client_send(const char *data, uint16_t length) {
    err_t err = ERR_OK;
    
    // Check if clientGlobal exists and is connected
    if (clientGlobal == NULL || !clientGlobal->is_connected || clientGlobal->pcb == NULL) {
        LOG("Cannot send - client not connected\n");
        return ERR_CONN;
    }
    
    // Send data
    err = tcp_write(clientGlobal->pcb, data, length, TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK) {
        // Actually send the data
        err = tcp_output(clientGlobal->pcb);
        if (err != ERR_OK) {
            LOG("tcp_output failed: %d\n", err);
        }
    } else {
        LOG("tcp_write failed: %d\n", err);
    }
    
    return err;
}

/* 
 * Function: tcp_client_process_recv_message
 * 
 * Description: 
 *      Prints the received buffer and analyzes its content.
 *      Checks if the buffer starts with "cmd:".
 *      If it does, it extracts the numeric value following "cmd:"
 *      and stores it as the received command.
 * 
 * Parameters:
 *  - received_command: Pointer to store the extracted command value
 * 
 * Returns: 
 *  - void
 */
void tcp_client_process_recv_message(uint8_t *received_command) 
{
    // Wait for the mutex before accessing the buffer
    if (xSemaphoreTake(bufferMutex, NO_TIMEOUT) == pdTRUE) 
    {
        // Access the receive buffer
        uint8_t *buffer = clientGlobal->receive_buffer;

        // Print the entire received buffer
        LOG("Received message: %s\n", buffer);

        // Check if the buffer starts with "cmd:"
        if (strncmp((const char *)buffer, "cmd:", 4) == 0) 
        {
            // Extract the number after "cmd:"
            // Explanation of atoi return value:
            // - If the string after "cmd:" is non-numeric (e.g., "cmd:abc"), atoi will return 0.
            // - If there is nothing after "cmd:" (e.g., "cmd:"), atoi will return 0.
            // - If the string after "cmd:" is explicitly "0", atoi will return 0 as a valid value.
            int command_value = atoi((const char *)&buffer[4]);

            // Ensure the command value is within 0-255 range
            if (command_value >= 0 && command_value <= 255) 
            {
                *received_command = (uint8_t)command_value;
                LOG("Received command: %u\n", *received_command);
            } 
            else 
            {
                LOG("Command value out of range (0-255).\n");
            }
        } 
        else 
        {
            LOG("No command found in received message.\n");
        }

        // Clear the buffer after processing
        memset(clientGlobal->receive_buffer, 0, sizeof(clientGlobal->receive_buffer));

        // Release the mutex after access
        xSemaphoreGive(bufferMutex); 
    } 
    else 
    {
        LOG("Failed to acquire the pointer to receive buffer.\n");
    }
}

#endif

/*******************************************************************************/
/*                          STATIC FUNCTION DEFINITIONS                        */
/*******************************************************************************/

#if (PICO_W_AS_TCP_SERVER == ON)
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
 * Function: tcp_server_recv_callback
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
static err_t tcp_server_recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *buffer, err_t err) 
{
    (void)arg; // Unused parameter

    /* Check if the received buffer is NULL. */
    if (!buffer) 
    {
        /* Log a message and return if the buffer is NULL. */
        LOG("buffer is NULL, returning... \n");
		return ERR_BUF;
    }

    if (err != ERR_OK) 
    {
        pbuf_free(buffer);
        return err;
    }

    /* Check if received data fits our buffer and the buffer is not empty */
    if ((buffer->tot_len > TCP_RECV_BUFFER_SIZE) || (buffer->tot_len == 0)) 
    {
        pbuf_free(buffer);
        return ERR_MEM;
    }

    /* Check the lwIP architecture for any state that needs to be updated. */
    cyw43_arch_lwip_check();

    /* Wait for the mutex before accessing the buffer */
    if (xSemaphoreTake(bufferMutex, NO_TIMEOUT) == pdTRUE) 
    {
        /* Allocate a buffer to hold the received data plus one byte for the null terminator. */
        /* Copy the received data from the pbuf to the newly allocated buffer. */
        pbuf_copy_partial(buffer, tcpServerGlobal->receive_buffer, buffer->tot_len, 0);

        /* Null-terminate the received string to ensure it's a valid C string. */
        tcpServerGlobal->receive_buffer[buffer->tot_len] = '\0'; 

        /* Print the received message to the console. */
        //LOG("Received message in callback: %s\n", tcpServerGlobal->receive_buffer);

#if (HTTP_ENABLED == ON)
        /* Process the HTTP response */
        process_HTTP_response(tcpServerGlobal->receive_buffer, sizeof(tcpServerGlobal->receive_buffer), pcb);
#endif
        /* Inform lwIP that the received data has been processed. */
        tcp_recved(pcb, buffer->tot_len);

        /* Release the mutex after finishing with the buffer */
        xSemaphoreGive(bufferMutex);
    } 
    else 
    {
        LOG("Failed to take mutex\n");
    }
    
    /* Free the pbuf to release memory used for the received data. */
    pbuf_free(buffer);

    return ERR_OK;
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
        LOG("Failure in accept\n");
        status = ERR_VAL; // Return an error value to indicate failure
    }
	else
	{
		/* Log a message indicating that a client has connected successfully. */
		LOG("Client connected\n");

		/* Store the client PCB in the tcpServer structure for later use. */
		tcpServer->client_pcb = client_pcb;
        
		/* Associate the tcpServer context with the client PCB for later reference. */
		tcp_arg(client_pcb, tcpServer);

		/* Set the receive callback function for the client PCB to handle incoming data. */
		tcp_recv(client_pcb, tcp_server_recv_callback);
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
    LOG("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);

    /* Check if the PCB was successfully created. */
    if (!pcb) 
    {
        /* Log an error message if PCB creation failed. */
        LOG("failed to create pcb\n");
        return false;
    }

    /* Bind the PCB to the specified port. Use NULL to bind to all available interfaces. */
#if (HTTP_ENABLED == ON)
    err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_HTTP_PORT);
#else
    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
#endif
    if (err) 
    {
        /* Log an error message if binding to the port failed. */
#if (HTTP_ENABLED == ON)
        LOG("failed to bind to port %u\n", TCP_HTTP_PORT);
#else
        LOG("failed to bind to port %u\n", TCP_PORT);
#endif
        return false;
    }

    /* Set the PCB to listen for incoming connections with a backlog of 1. */
    tcpServer->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!tcpServer->server_pcb) 
    {
        /* Log an error message if setting the server to listen failed. */
        LOG("failed to listen\n");
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

#else

/* 
 * Function: tcp_client_recv_callback
 * 
 * Description: 
 *      This callback function is called automatically when data arrives 
 *      at the TCP client. It handles the incoming data by storing it in 
 *      the client's buffer, prints the received data (for testing), and 
 *      sends an acknowledgment back to the server.
 * 
 * Parameters:
 *  - void *arg: A pointer to the TCP client structure (TCP_Client_t) that 
 *                is passed when the callback is registered.
 *  - struct tcp_pcb *tpcb: Pointer to the TCP protocol control block 
 *                            associated with the connection.
 *  - struct pbuf *p: Pointer to the pbuf structure containing the received data.
 *  - err_t err: An error code indicating the result of the receive operation.
 * 
 * Returns: 
 *  - err_t: An error code indicating the success (ERR_OK) or failure 
 *            (e.g., ERR_MEM if the received data exceeds the buffer size 
 *            or other error codes related to handling the incoming data).
 */
static err_t tcp_client_recv_callback(void *arg, struct tcp_pcb *tpcb,
                                    struct pbuf *p, err_t err) {
    TCP_Client_t *client = (TCP_Client_t*)arg;
    
    // Check if connection closed by remote host
    if (p == NULL) {
        client->is_connected = false;
        return ERR_OK;
    }
    
    // Handle receive error
    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }

    // Check if received data fits our buffer
    if (p->tot_len > TCP_RECV_BUFFER_SIZE) {
        pbuf_free(p);
        return ERR_MEM;
    }

    // Wait for the mutex before accessing the buffer
    if (xSemaphoreTake(bufferMutex, NO_TIMEOUT) == pdTRUE) 
    {
        // Copy received data to our buffer
        memcpy(client->receive_buffer, p->payload, p->tot_len);
        client->receive_length = p->tot_len;
        client->receive_buffer[p->tot_len] = '\0';  // Null terminate, assuming the received data will be used as string

        // Print received data (for testing)
        //LOG("Received: %s\n", client->receive_buffer);

#if (HTTP_ENABLED == ON)
        // Process the HTTP response
        process_HTTP_response(client->receive_buffer, client->receive_length);
#endif
        // Release the mutex after finishing with the buffer
        xSemaphoreGive(bufferMutex);
    } 
    else 
    {
        LOG("Failed to take mutex\n");
    }

    // Free the pbuf
    pbuf_free(p);

    // Send acknowledgment to tcp
    tcp_recved(tpcb, p->tot_len);

    return ERR_OK;
}

/* 
 * Function: tcp_client_connected_callback
 * 
 * Description: 
 *      This callback function is called when a connection to the server is 
 *      established. It sets up the receive callback for handling incoming 
 *      data and updates the connection status of the TCP client.
 * 
 * Parameters:
 *  - void *arg: A pointer to the TCP client structure (TCP_Client_t) that 
 *                is passed when the callback is registered.
 *  - struct tcp_pcb *tpcb: Pointer to the TCP protocol control block 
 *                            associated with the connection.
 *  - err_t err: An error code indicating the result of the connection attempt.
 * 
 * Returns: 
 *  - err_t: An error code indicating the success (ERR_OK) or failure 
 *            (e.g., other error codes if the connection attempt failed).
 */
static err_t tcp_client_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    TCP_Client_t *client = (TCP_Client_t*)arg;
    
    if (err != ERR_OK) {
        return err;
    }

    client->is_connected = true;
    
    // Set up receive callback
    tcp_recv(tpcb, tcp_client_recv_callback);
    
    LOG("TCP client connected to server\n");
    return ERR_OK;
}

/* 
 * Function: tcp_client_init
 * 
 * Description: 
 *      Allocates and initializes the TCP client structure, creates a new 
 *      TCP protocol control block (PCB), and initiates a connection to the 
 *      specified server. This function prepares the client for communication 
 *      by setting up necessary callbacks and connecting to the server.
 * 
 * Parameters:
 *  - None
 * 
 * Returns: 
 *  - TCP_Client_t*: A pointer to the initialized TCP client structure if 
 *                   successful; NULL if initialization or connection 
 *                   fails.
 */
TCP_Client_t* tcp_client_init(void) {
    ip_addr_t server_ip;
    
    // Allocate client structure
    TCP_Client_t *client = (TCP_Client_t*)pvPortMalloc(sizeof(TCP_Client_t));
    if (client == NULL) {
        LOG("Failed to allocate client structure\n");
        return NULL;
    }

    // Initialize client structure
    client->pcb = NULL;
    client->receive_length = 0;
    client->is_connected = false;

    // Create new TCP PCB
    client->pcb = tcp_new();
    if (client->pcb == NULL) {
        LOG("Failed to create TCP PCB\n");
        vPortFree(client);
        return NULL;
    }

    // Set client structure as argument for callbacks
    tcp_arg(client->pcb, client);

    // Convert server IP string to IP address structure
    ipaddr_aton(EXTERNAL_SERVER_IP_ADDRESS, &server_ip);

    // Connect to server
#if (HTTP_ENABLED == ON)
    err_t err = tcp_connect(client->pcb, &server_ip, TCP_HTTP_PORT, tcp_client_connected_callback);
#else
    err_t err = tcp_connect(client->pcb, &server_ip, TCP_PORT, tcp_client_connected_callback);
#endif
    if (err != ERR_OK) {
        LOG("TCP connect failed: %d\n", err);
        tcp_close(client->pcb);
        vPortFree(client);
        return NULL;
    }

    return client;
}

#endif

