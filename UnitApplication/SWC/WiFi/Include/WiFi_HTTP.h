#ifndef WIFI_HTTP_H
#define WIFI_HTTP_H

#include <stdint.h>
#include <stdbool.h>
#include "Common.h"
#include "lwip/tcp.h"
#include "WiFi_TCP.h"

#if (HTTP_ENABLED == ON)

#define HTTP_GET "GET"
#define HTTP_RESPONSE_HEADERS "HTTP/1.1 %d OK\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: keep-alive\n\n"
#define LED_TEST_BODY "<html><body><h1>Hello from Pico W.</h1><p>Led is %s</p><p><a href=\"?led=%d\">Turn led %s</a></body></html>"
#define LED_PARAM "led=%d"
#define LED_TEST "/ledtest"
#define LED_GPIO 0
#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 302 Redirect\nLocation: http://%s" LED_TEST "\n\n"

/* Sends an HTTP GET request over TCP */
bool send_http_get_request(const char *host, const char *path);

/* Processes the received HTTP response */
void process_HTTP_response(tcpServerType* tcpServer, struct tcp_pcb *pcb);

#endif /* HTTP_ENABLED */

#endif /* WIFI_HTTP_H */