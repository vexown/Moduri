#ifndef WIFI_RECEIVE_H
#define WIFI_RECEIVE_H

void receive_message_UDP(char* buffer, int buffer_size);
bool start_TCP_server(void);

#endif