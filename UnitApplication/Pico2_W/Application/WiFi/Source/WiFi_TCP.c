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
#include "lwip/err.h"

/* WiFi includes */
#include "WiFi_HTTP.h"
#include "WiFi_DHCP_server.h"
#include "WiFi_DNS_server.h"
#include "WiFi_OTA_download.h"

/* Misc includes */
#include "Common.h"

/*******************************************************************************/
/*                               GLOBAL VARIABLES                              */
/*******************************************************************************/
#if (PICO_W_AS_TCP_SERVER == ON)
tcpServerType *tcpServerGlobal;
#else
TCP_Client_t *clientGlobal;
#endif

/*******************************************************************************/
/*                               STATIC VARIABLES                              */
/*******************************************************************************/

/* Semaphore for notifying the TCP server that data is available */
static SemaphoreHandle_t tcp_data_available_semaphore = NULL;

/* Common mutex for accessing the TCP receive buffer */
static SemaphoreHandle_t bufferMutex;

/*******************************************************************************/
/*                         STATIC FUNCTION DECLARATIONS                        */
/*******************************************************************************/
#if (PICO_W_AS_TCP_SERVER == ON)
/* Open/close server */
static bool tcp_server_open();
static void tcp_server_close(tcpServerType *tcpServer);
/* Callback functions */
static err_t tcp_server_accept_callback(void *arg, struct tcp_pcb *client_pcb, err_t err);
static err_t tcp_server_recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *buffer, err_t err);
static void tcp_server_err_callback(void *arg, err_t err);
/* Send/receive functions */
static void tcp_server_process_recv_message(uint8_t *received_command);
static void tcp_server_send(const char *data, uint16_t length);
#else
/* Initialization functions */
static TCP_Client_t* tcp_client_init(void);
/* Callback functions */
static err_t tcp_client_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_client_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err);
static void tcp_client_err_callback(void *arg, err_t err);
/* Send/receive functions */
static void tcp_client_process_recv_message(char* output_buffer, uint16_t* output_buffer_length, uint8_t *received_command);
static err_t tcp_client_send(const char *data, uint16_t length);
#endif

/*******************************************************************************/
/*                          GLOBAL FUNCTION DEFINITIONS                        */
/*******************************************************************************/

#ifdef DEBUG_BUILD
/**
 * Function: tcp_send_debug
 * 
 * Description: Formats a debug message using printf-style arguments and sends it via TCP.
 * 
 * Parameters:
 *  - const char *format: Printf-style format string
 *  - ...: Variable arguments corresponding to format string
 * 
 * Returns: err_t indicating the result of the send operation
 */
err_t tcp_send_debug(const char* format, ...)
{
    char debug_buffer[256]; /* Fixed size buffer for debug messages */
    va_list args;
    
    /* Add [PICO] prefix to the buffer */
    strcpy(debug_buffer, "[PICO] ");
    size_t prefix_len = strlen(debug_buffer);
    
    /* Format the message with variable arguments */
    va_start(args, format);
    vsnprintf(debug_buffer + prefix_len, sizeof(debug_buffer) - prefix_len - 1, format, args);
    va_end(args);
    
    /* Ensure null termination */
    debug_buffer[sizeof(debug_buffer) - 1] = '\0';
    
    /* Send the formatted message */
    return tcp_send(debug_buffer, (uint16_t)strlen(debug_buffer)); //cast is safe because debug_buffer is fixed size and not bigger than UINT16_MAX
}
#endif

/* 
 * Function: tcp_send
 * 
 * Description: Unified function for sending data over TCP. It calls the appropriate
 *              send function based on the configuration of the Pico as a TCP server
 *              or client.
 * 
 * Parameters:
 *  - const char *data: Pointer to the data to be sent.
 *  - uint16_t length: Length of the data to be sent.
 * 
 * Returns: err_t indicating the result of the send operation.
 */
err_t tcp_send(const char* data, uint16_t length)
{
#if (PICO_W_AS_TCP_SERVER == ON)
    return tcp_server_send(data, length);
#else
    return tcp_client_send(data, length);
#endif
}

/* 
 * Function: tcp_receive_cmd
 * 
 * Description: Unified function for receiving commands over TCP. It calls the appropriate
 *              receive function based on the configuration of the Pico as a TCP server
 *              or client.
 * 
 * Parameters:
 *  - uint8_t *cmd: Pointer to the buffer where the received command will be stored.
 * 
 * Returns: void
 * 
 * Note: This function doesn't actually receive data, it just fetches the data from the buffer. 
 *       The actual data reception is done asynchronously by the lwIP stack and the receive callbacks.
 */
void tcp_receive_cmd(uint8_t* cmd)
{
#if (PICO_W_AS_TCP_SERVER == ON)
    tcp_server_process_recv_message(cmd); // Fetch data received in tcp_server_recv_callback
#else
    char received_cmd[CMD_MAX_SIZE_BYTES] = {0};
    uint16_t received_cmd_length = sizeof(received_cmd);

    tcp_client_process_recv_message(received_cmd, &received_cmd_length, cmd); // Fetch data received in tcp_client_recv_callback
#endif
}

/* 
 * Function: tcp_receive_data
 * 
 * Description: Unified function for receiving data over TCP. It calls the appropriate
 *              receive function based on the configuration of the Pico as a TCP server
 *              or client.
 * 
 * Parameters:
 *  - char* buffer: Pointer to the buffer where the received data will be stored.
 *  - uint16_t* buffer_length: Pointer to the length of the received data.
 * 
 * Returns: void
 */
void tcp_receive_data(char* buffer, uint16_t* buffer_length)
{
#if (PICO_W_AS_TCP_SERVER == ON)
    //tcp_server_process_recv_message(cmd); // Fetch data received in tcp_server_recv_callback - TODO, only receiving commands for now
#else
    uint8_t received_cmd; 

    tcp_client_process_recv_message(buffer, buffer_length, &received_cmd); // Fetch data received in tcp_client_recv_callback

    (void)received_cmd; //unused in the case of receiving data, there is a separate function for receving commands
#endif
}

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
    }
	else
	{
        ip4_addr_t mask;
        IP4_ADDR(ip_2_ip4(&tcpServerGlobal->gateway), 192, 168, 4, 1); // Set the gateway IP address of the TCP server to 192.168.4.1
        IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0); // Set the subnet mask to 255.255.255.0
        
        dhcp_server_t* dhcp_server = (dhcp_server_t*)pvPortCalloc(1, sizeof(dhcp_server_t));
        if (dhcp_server == NULL) 
        {
            LOG("Failed to allocate memory for DHCP server.\n");
            vPortFree(tcpServerGlobal);
            tcpServerGlobal = NULL;
        }
        /* Start the dhcp server */
        dhcp_server_init(dhcp_server, &tcpServerGlobal->gateway, &mask);

        
        dns_server_t* dns_server = (dns_server_t*)pvPortCalloc(1, sizeof(dns_server_t)); 
        if (dns_server == NULL) 
        {
            LOG("Failed to allocate memory for DNS server.\n");
            vPortFree(tcpServerGlobal);
            vPortFree(dhcp_server);
            tcpServerGlobal = NULL;
            dhcp_server = NULL;
        }
        /* Start the dns server */
        dns_server_init(dns_server, &tcpServerGlobal->gateway);

		serverOpenedAndListening = tcp_server_open();
		if (serverOpenedAndListening == false) 
		{
			LOG("TCP did not successfully open, closing the server and freeing memory... \n");
			tcp_server_close(tcpServerGlobal);
			vPortFree(tcpServerGlobal);
            vPortFree(dhcp_server);
            vPortFree(dns_server);
            tcpServerGlobal = NULL;
            dhcp_server = NULL;
            dns_server = NULL;
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
    }

    return status;
}

#else // PICO_W_AS_TCP_SERVER == OFF

// Custom send function for Mbed TLS
int tcp_client_send_ssl_callback(void *ctx, const unsigned char *buf, size_t len) 
{
    TCP_Client_t *client = (TCP_Client_t *)ctx;

    // Check if the client is valid and connected
    if (client == NULL || !tcp_client_is_connected || client->pcb == NULL) {
        LOG("Cannot send - client not connected\n");
        return MBEDTLS_ERR_NET_SEND_FAILED;
    }

    // Use your existing tcp_send function
    err_t err = tcp_send((const char *)buf, len);
    if (err != ERR_OK) {
        LOG("tcp_send failed: %d\n", err);
        return MBEDTLS_ERR_NET_SEND_FAILED;
    }

    // Since tcp_send doesn't return bytes sent, assume full send on success
    // Note: lwIP's tcp_write is non-blocking, but tcp_output flushes, so this is usually safe
    LOG("Sent %u bytes\n", len); 
    return (int)len;
}

// Custom receive function for Mbed TLS
int tcp_client_recv_ssl_callback(void *ctx, unsigned char *buf, size_t len) {
    TCP_Client_t *client = (TCP_Client_t*)ctx;
    size_t bytes_to_read = 0;
    TickType_t start_time = xTaskGetTickCount();
    
    // Wait for data with timeout (5 seconds)
    while (bytes_to_read == 0) {
        // Check for timeout
        if ((xTaskGetTickCount() - start_time) > pdMS_TO_TICKS(5000)) {
            return bytes_to_read > 0 ? bytes_to_read : MBEDTLS_ERR_SSL_TIMEOUT;
        }
        
        // Check if connection closed
        if (!tcp_client_is_connected || client->is_closing) {
            return MBEDTLS_ERR_NET_CONN_RESET;
        }
        
        // Check if we already have data in the buffer
        if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (client->receive_length > 0) {
                // Copy data to caller's buffer
                bytes_to_read = client->receive_length > len ? len : client->receive_length;
                memcpy(buf, client->receive_buffer, bytes_to_read);
                
                // If there's more data left, move it to the beginning of the buffer
                if (bytes_to_read < client->receive_length) {
                    memmove(client->receive_buffer, 
                            client->receive_buffer + bytes_to_read, 
                            client->receive_length - bytes_to_read);
                    client->receive_length -= bytes_to_read;
                    
                    // Signal that there's still data available
                    xSemaphoreGive(tcp_data_available_semaphore);
                } else {
                    // All data consumed - reset buffer
                    client->receive_length = 0;
                }
            }
            xSemaphoreGive(bufferMutex);
        }
        
        // If no data was read and we need to wait
        if (bytes_to_read == 0) {
            // Wait for data notification
            if (xSemaphoreTake(tcp_data_available_semaphore, pdMS_TO_TICKS(100)) != pdTRUE) {
                // No data yet, continue waiting loop
            }
        }
    }
    
    return bytes_to_read > 0 ? bytes_to_read : MBEDTLS_ERR_SSL_WANT_READ;
}


/**
 * Function: tcp_client_connect
 * 
 * Description: Connects the TCP client (Pico) to the specified IP address (host) and port of some TCP server.
 * 
 * Parameters:
 * - const char *host: IP address of the server to connect to. Example: "192.168.1.194"
 * - uint16_t port: Port number of the server to connect to. Example: 443 (HTTPS)
 * 
 * Returns: bool indicating the success (true) or failure (false) of the connection.
 */
bool tcp_client_connect(const char *host, uint16_t port) 
{
    ip_addr_t server_ip;

    if(clientGlobal == NULL || clientGlobal->pcb == NULL)
    {
        LOG("TCP client not initialized or PCB is NULL\n");
        return false;
    }
    
    /* Check if we are already connected to a host */
    if(tcp_client_is_connected())
    {
        /* Check if trying to connect to a different host/port while already connected */
        ip_addr_t requested_ip;
        if (ipaddr_aton(host, &requested_ip) == 0)
        {
            LOG("Invalid IP address: %s\n", host);
            return false;
        }
        
        /* Compare with current connection */
        ip_addr_t *current_ip = &clientGlobal->pcb->remote_ip;
        u16_t current_port = clientGlobal->pcb->remote_port;
        
        if (!ip_addr_cmp(&requested_ip, current_ip) || port != current_port)
        {
            LOG("Already connected to a different host (%s:%d). Disconnect first before connecting to %s:%d\n",
                ipaddr_ntoa(current_ip), current_port, host, port);
            return false;
        }
        
        LOG("Already connected to requested host\n");
        return true;
    }
    else
    {
        /* The lwIP TCP PCB has a few states. So even if we are not connected, we also need to make sure we are specifically
           in the tcp CLOSED state. It is the only state from which we can attempt to tcp_connect */
        if(clientGlobal->pcb->state == CLOSED)
        {
            LOG("TCP state is CLOSED. Attempting to connect... \n");
        }
        else
        {
            LOG("TCP state is not CLOSED. Current state: %d - Attempting to reinitialize the TCP client... \n", clientGlobal->pcb->state); // find enum tcp_state in pico-sdk for info about the states
            tcp_client_disconnect();
            WiFiState = INIT; // Reinitialize the TCP client in attempt to connect again
            return false;
        }
    }
    
    if (ipaddr_aton(host, &server_ip) == 0) // Convert IP address string (of the host) to numeric value
    {
        LOG("Invalid IP address: %s\n", host);
        return false;
    }
    
    /* Connects the client to the server. Connected callback will be called when the connection is established. */
    /* If connection wasn't properly established, the generic "err" callback will be called (it can be set via tcp_err() function) */
    err_t err = tcp_connect(clientGlobal->pcb, &server_ip, port, tcp_client_connected_callback); // async call, returns immediately
    if (err != ERR_OK) // This DOES NOT check if connection was successful, only if connection request was sent successfully
    {
        LOG("Failed to send connection request: %d\n", err);
        WiFiState = INIT; // Reinitialize the TCP client in attempt to connect again
        return false;
    }

    /* Wait for connection to establish */
    TickType_t xStartTime = xTaskGetTickCount();
    TickType_t xTimeoutTicks = pdMS_TO_TICKS(10000); // 10s

    while (!tcp_client_is_connected() && !clientGlobal->is_closing) 
    {
        /* Check if timeout has occurred */
        if ((xTaskGetTickCount() - xStartTime) > xTimeoutTicks) 
        {
            LOG("Connection timeout\n");
            WiFiState = INIT; // Reinitialize the TCP client in attempt to connect again
            return false;
        }
        
        /* Block this task for 10ms allowing other tasks to run */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    /* Client successfully connected (otherwise the while loop would have returned after timeout) */
    return tcp_client_is_connected();
}

/**
 * Function: tcp_client_disconnect
 * 
 * Description: Disconnects the TCP client from the server and cleans up resources.
 * 
 * Parameters:
 * - void *client: Pointer to the TCP client structure.
 * 
 * Returns: void
 */
void tcp_client_disconnect(void) 
{
    if (clientGlobal && clientGlobal->pcb) // double-check if client and its pcb are not NULL
    {
        clientGlobal->is_closing = true; // indicate that the client is closing the connection

        /* Clear the callbacks for the client PCB to release the associated resources */
        tcp_arg(clientGlobal->pcb, NULL);
        tcp_recv(clientGlobal->pcb, NULL);
        tcp_err(clientGlobal->pcb, NULL);
        tcp_sent(clientGlobal->pcb, NULL);

        /* Close the connection held by the client PCB and free the resources */
        err_t err = tcp_close(clientGlobal->pcb);

        /* According to lwIP documentation in pico-sdk, if tcp_close() returns ERR_OK, the connection was closed successfully
           and if it returns ERR_MEM, the connection was not closed and the application should wait and try again */
        if(err != ERR_OK)
        {
            /* So let's wait for the connection to close if needed - but not forever */
            TickType_t xStartTime = xTaskGetTickCount();
            TickType_t xTimeoutTicks = pdMS_TO_TICKS(2000); // 2s

            while (clientGlobal->is_closing) 
            {
                /* Check if timeout has occurred */
                if ((xTaskGetTickCount() - xStartTime) > xTimeoutTicks) 
                {
                    LOG("Connection close timeout\n");
                    break;
                }
                else if(tcp_close(clientGlobal->pcb) == ERR_OK) /* Try again to close the connection */
                {
                    LOG("Connection closed\n");
                    clientGlobal->is_closing = false;
                    clientGlobal->pcb = NULL;
                    break;
                }
                else /* Block this task for 10ms allowing other tasks to run and try again after */
                {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }
        }

        /* As a last resort, if after the timeout the connection is still open, 
           force the abort which sends a RST segment to remote host. Ugly but never fails */
        if (tcp_client_is_connected() == true)
        {
            LOG("Error closing TCP connection - forcing abort... \n");
            tcp_abort(clientGlobal->pcb);
        }
    }
}

/* 
 * Function: start_TCP_client
 * 
 * Description: Starts the TCP client and reports the status
 * 
 * Returns: bool indicating the success (true) or failure (false) of starting the TCP client.
 */
bool start_TCP_client(void) 
{
    bool status = false;

    /* Check the initialization and connection status of the TCP client */
    if(clientGlobal != NULL && tcp_client_is_connected())
    {
        LOG("TCP client already started and connected \n");
        status = true;
    }
    else if (clientGlobal != NULL && !tcp_client_is_connected())
    {
        LOG("TCP client is initialized but the connection is not established. Freeing resources and reinitializing... \n");
        
        tcp_client_disconnect();
        
        vPortFree(clientGlobal);
        clientGlobal = NULL;
    }
    else
    {
        LOG("Initializing the TCP client... \n");
        clientGlobal = tcp_client_init();

        if (clientGlobal != NULL) 
        {
            LOG("TCP client initialized successfully\n");
            status = true;
        } 
        else 
        {
            LOG("TCP client initialization failed\n");
        }
    }

    /* If the client was successfully initialized, create a mutex for the buffer */
    if(status == true)
    {
        bufferMutex = xSemaphoreCreateMutex();
        if (bufferMutex == NULL) 
        {
            LOG("Failed to create mutex\n");
            status = false;
        }
    }

    /* If the client was successfully initialized, but not connected yet, wait for the connection to establish */
    if((status == true) && (!tcp_client_is_connected()))
    {
        LOG("Giving the network a few seconds to establish a connection... \n");

        /* Wait for connection to establish */
        TickType_t xStartTime = xTaskGetTickCount();
        TickType_t xTimeoutTicks = pdMS_TO_TICKS(2500); // 2.5s

        while (!tcp_client_is_connected()) 
        {
            /* Check if timeout has occurred */
            if ((xTaskGetTickCount() - xStartTime) > xTimeoutTicks) 
            {
                LOG("Connection timeout\n");
                status = false;
                break;
            }
            
            /* Block this task for 10ms allowing other tasks to run */
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
        
    return status;
}

/**
 * Function: tcp_client_is_connected
 * 
 * Description: Safely checks if TCP client is connected
 * 
 * Returns: bool indicating connection state
 */
bool tcp_client_is_connected(void) 
{
    if (clientGlobal == NULL || clientGlobal->pcb == NULL) 
    {
        return false;
    }
    
    return (clientGlobal->pcb->state == ESTABLISHED);
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
    /* Check if the received buffer is NULL. */
    if (!buffer) 
    {
        /* When the client sends a FIN (indicating it wants to close the connection), the tcp_recv callback will be invoked with buffer == NULL. */
        if (err == ERR_OK)
        {
            LOG("Connection closed gracefully\n");
            tcp_close_client_connection(pcb);
            /* err value you return from the tcp_recv callback should guide lwIP in how to proceed with the connection. In this case we tell lwIP all is good */
            return ERR_OK; 
        } 
        else /* In other cases - NULL buffer means something went wrong */
        {
            LOG("Error receiving data: %d\n", err); 
            tcp_close_client_connection(pcb);
            return ERR_ABRT; // Tell lwIP to abort the connection
        }
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
    if (xSemaphoreTake(bufferMutex, NON_BLOCKING) == pdTRUE) 
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
        process_HTTP_response(tcpServerGlobal, pcb);
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
 * Function: tcp_close_client_connection
 * 
 * Description: Closes the client connection by freeing the resources associated 
 *              with the client PCB and setting the client PCB pointer to NULL.
 * 
 * Parameters:
 *  - client_pcb: Pointer to the tcp_pcb structure representing the client connection.
 * 
 * Returns: void
 */
void tcp_close_client_connection(struct tcp_pcb *client_pcb) 
{
    if (client_pcb) 
    {
        /* Log the client's IP address and port before closing the connection. */
        ip_addr_t *client_ip = &client_pcb->remote_ip;
        u16_t client_port = client_pcb->remote_port;
        LOG("Closing connection for client: IP=%s, Port=%d\n", ipaddr_ntoa(client_ip), client_port);

        /* Clear the callbacks for the client PCB to release the associated resources. */
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);

        /* Close the client PCB to terminate the connection. */
        err_t err = tcp_close(client_pcb);
        if (err != ERR_OK) 
        {
            LOG("close failed %d, calling abort\n", err);
            tcp_abort(client_pcb);
        }
    } 
    else 
    {
        LOG("tcp_close_client_connection called with NULL client_pcb\n");
    }
}

/* 
 * Function: tcp_server_err_callback
 * 
 * Description: Callback function that is called when an error occurs during 
 *              the TCP server operation. It logs the error and closes the client 
 *              connection if the error is not an abort error.
 * 
 * Parameters:
 *  - arg: User-defined argument passed to the error callback, typically containing server context.
 *  - err: Error status indicating the type of error that occurred.
 * 
 * Returns: void
 */
static void tcp_server_err_callback(void *arg, err_t err) 
{
    (void)arg; // Unused parameter

    if (err != ERR_ABRT) // See list of possible error codes in lwip/err.h in pico-sdk
    {
        LOG("tcp_client_err_fn %d\n", err);
        tcp_close_client_connection(tcpServerGlobal->client_pcb);
    }
}

/* 
 * Function: tcp_server_accept_callback
 * 
 * Description: Callback function that is called when there is a new connection request by a client 
 *              to the TCP server. It sets up the client PCB and prepares to receive data from the client.
 * 
 * Parameters:
 *  - arg: User-defined argument passed to the accept callback, typically containing server context.
 *  - client_pcb: Pointer to the tcp_pcb structure representing the new client connection.
 *  - err: Error status indicating if there was an error during the accept process.
 * 
 * Returns: ERR_OK always - we either accept the connection or abort it.
 */
static err_t tcp_server_accept_callback(void *arg, struct tcp_pcb *client_pcb, err_t err) 
{
    (void)arg; // Unused parameter
    
    /* Check for errors in the accept callback or if the client PCB is NULL. */
    if (err != ERR_OK || client_pcb == NULL) 
    {
        /* Log a failure message if the connection was not successful. */
        if (err != ERR_OK) 
        { 
            LOG("Failure in accept: Error code %d\n", err); 
        } 
        else 
        { 
            LOG("Failure in accept: client_pcb is NULL\n"); 
        }

        if (client_pcb != NULL) 
        {
            tcp_abort(client_pcb); // Clean up the client PCB
        }
    }
	else
	{
		/* Log a message indicating that a client has connected successfully. */
        ip_addr_t *client_ip = &client_pcb->remote_ip;
        u16_t client_port = client_pcb->remote_port;
        LOG("Client connected from IP: %s, Port: %d\n", ipaddr_ntoa(client_ip), client_port);

		/* Store the client PCB in the tcpServer structure for later use. */
		tcpServerGlobal->client_pcb = client_pcb;

		/* Set the receive callback function for the client PCB to handle incoming data. */
		tcp_recv(client_pcb, tcp_server_recv_callback);

        /* Set the error callback function for the client PCB to handle connection errors. */
        tcp_err(client_pcb, tcp_server_err_callback);
	}
    
    return ERR_OK; // Always return ERR_OK to lwIP (we either accept the connection or abort it)
}

/* 
 * Function: tcp_server_open
 * 
 * Description: Initializes and opens the TCP server, binding it to a specified port 
 *              and setting up a listening state for incoming connections.
 * 
 * Parameters: void (uses the global tcpServer structure)
 * 
 * Returns: bool indicating the success (true) or failure (false) of the server opening.
 */
static bool tcp_server_open(void) 
{
    /* Create a new TCP protocol control block (PCB) for the server,
       allowing any IP address type to be used (IPv4 or IPv6). */
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);

    /* Log the starting message with the server's IP address and port. */
#if (HTTP_ENABLED == ON)
    LOG("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_HTTP_PORT);
#else
    LOG("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);
#endif

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
    tcpServerGlobal->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!tcpServerGlobal->server_pcb) 
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

    /* Set the accept callback function for incoming connections. */
    tcp_accept(tcpServerGlobal->server_pcb, tcp_server_accept_callback);

    /* Return true to indicate the server has been successfully opened and is listening for incoming connections */
    return true;
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
static void tcp_server_process_recv_message(uint8_t *received_command) 
{
    // Wait for the mutex before accessing the buffer
    if (xSemaphoreTake(bufferMutex, NON_BLOCKING) == pdTRUE) 
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
static err_t tcp_server_send(const char *data, uint16_t length) 
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
static TCP_Client_t* tcp_client_init(void) 
{
    ip_addr_t server_ip;
    
    // Allocate client structure
    TCP_Client_t *client = (TCP_Client_t*)pvPortMalloc(sizeof(TCP_Client_t));
    if (client == NULL) {
        LOG("Failed to allocate client structure\n");
        return NULL;
    }

    tcp_data_available_semaphore = xSemaphoreCreateBinary();
    if (tcp_data_available_semaphore == NULL) 
    {
        LOG("Failed to create semaphore\n");
        vPortFree(client);
        client = NULL;
        return NULL;
    }

    // Initialize client structure
    client->pcb = NULL;
    client->receive_length = 0;
    client->is_closing = false;

    // Create new TCP PCB
    client->pcb = tcp_new();
    if (client->pcb == NULL) {
        LOG("Failed to create TCP PCB\n");
        vPortFree(client);
        return NULL;
    }

    // Set client structure as argument for callbacks
    tcp_arg(client->pcb, client);

    // Set up error callback
    tcp_err(client->pcb, tcp_client_err_callback);

#if (OTA_ENABLED == ON)
    // Set up the TCP client to connect to the external server
    ipaddr_aton(OTA_HTTPS_SERVER_IP_ADDRESS, &server_ip);
    // Connect to server
    err_t err = tcp_connect(client->pcb, &server_ip, OTA_HTTPS_SERVER_PORT, tcp_client_connected_callback);
#else
    // Convert server IP string to IP address structure
    ipaddr_aton(REMOTE_TCP_SERVER_IP_ADDRESS, &server_ip);
    // Connect to server
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
static err_t tcp_client_send(const char *data, uint16_t length) {
    err_t err = ERR_OK;
    
    // Check if clientGlobal exists and is connected
    if (clientGlobal == NULL || !tcp_client_is_connected() || clientGlobal->pcb == NULL) {
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
 *  - char* output_buffer: Pointer to the buffer where the received data will be stored.
 *  - uint16_t* output_buffer_length: Pointer to the length of the received data.
 *  - uint8_t* received_command: Pointer to store the extracted command value.
 * 
 * Returns: 
 *  - void
 */
static void tcp_client_process_recv_message(char* output_buffer, uint16_t* output_buffer_length, uint8_t *received_command)
{
    /* Initialize output parameters to default values */
    *received_command = 0;
    *output_buffer = '\0';

    /* Make sure we have a valid client and receive buffer */
    if (clientGlobal == NULL || clientGlobal->receive_buffer == NULL || output_buffer == NULL || output_buffer_length == NULL || received_command == NULL) 
    {
        LOG("Invalid client or receive buffer\n");
        return;
    }

    /* Make sure client is connected so we can tell the difference between no connection and no data */
    if (!tcp_client_is_connected()) 
    {
        LOG("Attempted to receive data while not connected. Trying to reconnect...\n");
#if (OTA_ENABLED == ON)
        tcp_client_connect(OTA_HTTPS_SERVER_IP_ADDRESS, OTA_HTTPS_SERVER_PORT);
#else
        tcp_client_connect(REMOTE_TCP_SERVER_IP_ADDRESS, TCP_PORT);
#endif
        return;
    }

    /* Wait for the mutex before accessing the buffer */
    if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(1000)) == pdTRUE) 
    {
        /* Check if the output buffer is large enough to store the received data */
        if(clientGlobal->receive_length > *output_buffer_length)
        {
            LOG("Output buffer is too small to store the received data. Data will be truncated.\n");
        }
        else
        {
            *output_buffer_length = clientGlobal->receive_length; // Update the output buffer length with the actual received data length
        }

        /* Copy the received data from the TCP receive_buffer to the output buffer */
        memcpy(output_buffer, clientGlobal->receive_buffer, *output_buffer_length);

        /* Check if the message starts with "cmd:" and if it's even long enough to contain a command */
        if (*output_buffer_length >= CMD_MIN_SIZE_BYTES && strncmp((const char *)output_buffer, "cmd:", 4) == 0) 
        {
            /* Extract the number after "cmd:"
             * Explanation of atoi return value:
             * - If the string after "cmd:" is non-numeric (e.g., "cmd:abc"), atoi will return 0.
             * - If there is nothing after "cmd:" (e.g., "cmd:"), atoi will return 0.
             * - If the string after "cmd:" is explicitly "0", atoi will return 0 as a valid value.
             */
            int command_value = atoi((const char *)&output_buffer[4]);

            /* Ensure the command value is within 1-255 range */
            if (command_value > 0 && command_value <= 255) 
            {
                *received_command = (uint8_t)command_value;
                LOG("Received command: %u\n", *received_command);
            } 
            else 
            {
                received_command = 0;
                LOG("Command value out of range (1-255).\n");
            }
        }
        else /* Received message is not a command */
        {
            /* Print the entire received buffer (if not empty) */
            if(strlen((const char *)output_buffer) > 0)
            {
                LOG("Received message: %s\n", output_buffer);
                LOG("Data has been stored in the output buffer\n");
            }
        }

        /* Clear the buffer after processing */
        memset(clientGlobal->receive_buffer, 0, sizeof(clientGlobal->receive_buffer));
        /* Set the receive length to 0 */
        clientGlobal->receive_length = 0;

        /* Release the mutex after access */
        xSemaphoreGive(bufferMutex); 
    } 
    else 
    {
        LOG("Failed to acquire the mutex for the receive buffer.\n");
    }
}

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
static err_t tcp_client_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_Client_t* client = (TCP_Client_t*)arg;
    
    if (p == NULL) {
        // Connection closed by remote host
        return ERR_OK;
    }

    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }

    // Handle incoming data
    if (xSemaphoreTake(bufferMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (p->tot_len + client->receive_length <= TCP_RECV_BUFFER_SIZE) {
            // Copy data from pbuf to our buffer
            pbuf_copy_partial(p, client->receive_buffer + client->receive_length, p->tot_len, 0);
            client->receive_length += p->tot_len;
            
            LOG("Appended %d bytes to receive_buffer, total %d\n", 
                p->tot_len, client->receive_length);
            
            // Signal that data is available
            xSemaphoreGive(tcp_data_available_semaphore);
        } 
        else 
        {
            LOG("Receive buffer overflow: %d + %d > %d\n", 
                client->receive_length, p->tot_len, TCP_RECV_BUFFER_SIZE);
        }
        
        xSemaphoreGive(bufferMutex);
    }
    
    // Always acknowledge the data
    tcp_recved(tpcb, p->tot_len);
    
    // Free the pbuf
    pbuf_free(p);
    
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
static err_t tcp_client_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) 
{
    TCP_Client_t *client = (TCP_Client_t*)arg;
    
    if (err != ERR_OK) // Connection failed (Host unreachable, Connection refused, etc.)
    {
        LOG("TCP client connection failed: %d\n", err);
        return err;
    }
    
    /* Set up receive callback */
    tcp_recv(tpcb, tcp_client_recv_callback);
    
    LOG("TCP client connected to server\n");
    return ERR_OK;
}

/**
 * Function: tcp_client_err_callback
 * 
 * Description: Callback function that is called when when a fatal error
 *              has occurred on the connection.
 * 
 * Parameters:
 *  - void *arg: A pointer to the TCP client structure (TCP_Client_t) that
 *               is passed when the callback is registered.
 *  - err_t err: Error status indicating the type of error that occurred.
 * 
 * Returns: void
 * 
 * Note: The corresponding pcb is already freed when this callback is called!
 */
static void tcp_client_err_callback(void *arg, err_t err) 
{
    (void)arg; // Unused parameter, we know the client is global

    if(clientGlobal != NULL)
    {
        clientGlobal->is_closing = false;
        clientGlobal->pcb = NULL;
    }

    switch (err) 
    {
        case ERR_ABRT:
            LOG("Connection aborted locally\n");
            break;
        case ERR_RST:
            LOG("Connection reset by remote host\n");
            break;
        case ERR_CLSD:
            LOG("Connection closed by remote host\n");
            break;
        case ERR_TIMEOUT:
            LOG("Connection timed out\n");
            break;
        default:
            LOG("TCP Client Error: %d\n", err);
            break;
    }

    /* Attempt to recover connection by reinitializing the client */
    WiFiState = INIT;
}

#endif

