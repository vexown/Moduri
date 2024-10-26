#ifndef WIFI_TCP_H
#define WIFI_TCP_H

bool start_TCP_server(void);
bool start_TCP_client(void);
err_t tcp_client_send(const char *data, uint16_t length);

#endif