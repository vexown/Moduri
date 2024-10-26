#ifndef WIFI_UDP_H
#define WIFI_UDP_H

void receive_message_UDP(char* buffer, int buffer_size);
void send_message_UDP(const char* message);

#endif