/**
 * File: WiFi_UDP.c
 * Description: High level implementation for lwIP-based UDP communication between Unit and Central Application.
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "semphr.h"

/* Standard includes */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* SDK includes */
#include "lwip/udp.h"
#include "lwip/ip_addr.h"

/* WiFi includes */
#include "WiFi_Common.h"

/*******************************************************************************/
/*                                STATIC VARIABLES                             */
/*******************************************************************************/
static struct udp_pcb *udp_client_pcb;
static uint8_t recv_data[UDP_RECV_BUFFER_SIZE];
static SemaphoreHandle_t bufferMutex;

/*******************************************************************************/
/*                          STATIC FUNCTION DECLARATIONS                       */
/*******************************************************************************/
static void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static bool udp_send_message(const char *message, const ip_addr_t *dest_addr);
static bool udp_client_init(void);

/*******************************************************************************/
/*                          GLOBAL FUNCTION DEFINITIONS                        */
/*******************************************************************************/

void udp_client_process_recv_message(uint8_t *received_command) 
{
    // Wait for the mutex before accessing the buffer
    if (xSemaphoreTake(bufferMutex, NO_DELAY) == pdTRUE) 
    {
        // Access the receive buffer
        uint8_t *buffer = recv_data;

        // Print the entire received buffer
        printf("Received message: %s\n", buffer);

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
                printf("Received command: %u\n", *received_command);
            } 
            else 
            {
                printf("Command value out of range (0-255).\n");
            }
        } 
        else 
        {
            printf("No command found in received message.\n");
        }

        // Clear the buffer after processing
        memset(recv_data, 0, sizeof(recv_data));

        // Release the mutex after access
        xSemaphoreGive(bufferMutex); 
    } 
    else 
    {
        printf("Failed to acquire the pointer to receive buffer.\n");
    }
}

/* 
 * Function: start_UDP_client
 * 
 * Description: Starts the UDP client and reports the status
 * 
 * Returns: bool indicating the success (true) or failure (false) of starting the UDP client.
 */
bool start_UDP_client(void) 
{
    bool status = udp_client_init();

    if (status == true) 
    {
        printf("UDP client initialized successfully\n");
    } 
    else 
    {
        printf("UDP client initialization failed\n");
    }

    bufferMutex = xSemaphoreCreateMutex();
    if (bufferMutex == NULL) 
    {
        printf("Failed to create mutex\n");
        status = false;
    }
    
    return status;
}

bool udp_client_send(const char* message) 
{
    static ip_addr_t dest_addr;
    static bool addr_initialized = false;
    
    // Initialize the address only once
    if (!addr_initialized) {
        // Try string conversion first
        if (ipaddr_aton(PC_IP_ADDRESS, &dest_addr) != 1) {
            // If string conversion fails, use direct method
            IP4_ADDR(&dest_addr, 192, 168, 1, 194);
        }
        addr_initialized = true;
    }
    
    // Send the message
    bool result = udp_send_message(message, &dest_addr);
    if (!result) {
        printf("Failed to send UDP message to %s\n", PC_IP_ADDRESS);
    }
    
    return result;
}

/*******************************************************************************/
/*                          STATIC FUNCTION DEFINITIONS                        */
/*******************************************************************************/

// Callback function for receiving UDP packets
static void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) 
{
    if (p != NULL) 
    {
        // Wait for the mutex before accessing the buffer
        if (xSemaphoreTake(bufferMutex, NO_DELAY) == pdTRUE) 
        {
            memset(recv_data, 0, sizeof(recv_data));
        
            if (p->len < UDP_RECV_BUFFER_SIZE) 
            {
                memcpy(recv_data, p->payload, p->len);

                recv_data[p->len] = '\0';  // Null terminate, assuming the received data will be used as string

                printf("Received UDP message from %s:%d\n", ipaddr_ntoa(addr), port);
            }

            // Release the mutex after finishing with the buffer
            xSemaphoreGive(bufferMutex);
        } 
        else 
        {
            printf("Failed to take mutex\n");
        }
        
        pbuf_free(p);
    }
}

// Function to send UDP message
static bool udp_send_message(const char *message, const ip_addr_t *dest_addr) 
{
    if (!udp_client_pcb || !message || !dest_addr) {
        return false;
    }
    
    size_t message_len = strlen(message);
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, message_len, PBUF_RAM);
    
    if (!p) {
        return false;
    }
    
    // Copy message to pbuf
    memcpy(p->payload, message, message_len);
    
    // Send the packet
    err_t err = udp_sendto(udp_client_pcb, p, dest_addr, UDP_SERVER_PORT);
    
    // Free the packet buffer
    pbuf_free(p);
    
    return (err == ERR_OK);
}

// Initialize UDP client
bool udp_client_init(void) {
    // Create new UDP PCB
    udp_client_pcb = udp_new();
    if (!udp_client_pcb) {
        printf("Failed to create UDP PCB\n");
        return false;
    }
    
    // Bind to local port
    err_t err = udp_bind(udp_client_pcb, IP_ADDR_ANY, UDP_CLIENT_PORT);
    if (err != ERR_OK) {
        printf("Failed to bind UDP PCB: %d\n", err);
        udp_remove(udp_client_pcb);
        return false;
    }
    
    // Set receive callback
    udp_recv(udp_client_pcb, udp_receive_callback, NULL);
    
    // Print network information
    struct netif *netif = netif_default;
    if (netif != NULL) {
        printf("\nNetwork Information:\n");
        printf("IP Address: %s\n", ipaddr_ntoa(&netif->ip_addr));
        printf("Netmask: %s\n", ipaddr_ntoa(&netif->netmask));
        printf("Gateway: %s\n", ipaddr_ntoa(&netif->gw));
        printf("Listening on port: %d\n\n", UDP_CLIENT_PORT);
    }
    
    return true;
}
