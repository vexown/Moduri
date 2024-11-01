#ifndef WIFI_TCP_H
#define WIFI_TCP_H

#include "lwip/err.h"

bool start_TCP_server(void);
bool start_TCP_client(void);
err_t tcp_client_send(const char *data, uint16_t length);
void tcp_client_process_recv_message(uint8_t *received_command);

#endif