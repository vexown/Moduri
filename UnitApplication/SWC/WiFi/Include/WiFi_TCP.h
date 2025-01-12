#ifndef WIFI_TCP_H
#define WIFI_TCP_H

#include "lwip/err.h"
#include "Common.h"

#if (PICO_W_AS_TCP_SERVER == ON)
bool start_TCP_server(void);
#else
bool start_TCP_client(void);
err_t tcp_client_send(const char *data, uint16_t length);
void tcp_client_process_recv_message(uint8_t *received_command);
#endif /* PICO_W_AS_TCP_SERVER */

#endif /* WIFI_TCP_H */