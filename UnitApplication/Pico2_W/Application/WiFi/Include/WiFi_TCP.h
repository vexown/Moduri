#ifndef WIFI_TCP_H
#define WIFI_TCP_H

#include "lwip/err.h"
#include "lwip/tcp.h"
#include "mbedtls/ssl.h"
#include "Common.h"
#include "WiFi_Common.h"

/*******************************************************************************/
/*                                 DEFINES                                     */
/*******************************************************************************/
#define MBEDTLS_ERR_NET_SEND_FAILED -0x004C  /**< Failed to send data. */
#define MBEDTLS_ERR_NET_CONN_RESET -0x004E  /**< Connection was reset by peer. */
#define MBEDTLS_ERR_SSL_WANT_READ  -0x6900  /**< No data available, try again. */

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
    ip_addr_t gateway;
} tcpServerType;

/* Basic TCP client structure */
typedef struct {
    struct tcp_pcb *pcb;                      // The TCP protocol control block - lwIP's main structure for a TCP connection
    uint8_t receive_buffer[TCP_RECV_BUFFER_SIZE]; // Buffer for incoming data
    uint16_t receive_length;                  // Length of received data
    bool is_closing;                         // Connection closing status
} TCP_Client_t;

/*******************************************************************************/
/*                               GLOBAL VARIABLES                              */
/*******************************************************************************/
#if (PICO_W_AS_TCP_SERVER == ON)
/* Global server instance */
extern tcpServerType *tcpServerGlobal;
#else
/* Global client instance */
extern TCP_Client_t *clientGlobal;
#endif

/*******************************************************************************/
/*                        GLOBAL FUNCTION DELCARATION                          */
/*******************************************************************************/
err_t tcp_send(const char *data, uint16_t length);
void tcp_receive_cmd(uint8_t *received_command);

#ifdef DEBUG_BUILD
err_t tcp_send_debug(const char* format, ...);
#endif

#if (PICO_W_AS_TCP_SERVER == ON)
/* Global server instance */
extern tcpServerType *tcpServerGlobal;

/* Server initialization */
bool start_TCP_server(void);
/* Connection management */
void tcp_close_client_connection(struct tcp_pcb *client_pcb);
#else
/* Global client instance */
extern TCP_Client_t *clientGlobal;
/* Client initialization */
bool start_TCP_client(void);
/* Connection management */
bool tcp_client_connect(const char *host, uint16_t port);
void tcp_client_disconnect(void);
bool tcp_client_is_connected(void);
/* OTA specific functions */
int tcp_client_send_ssl_callback(void *ctx, const unsigned char *buf, size_t len);
int tcp_client_recv_ssl_callback(void *ctx, unsigned char *buf, size_t len);
#endif

#endif /* WIFI_TCP_H */