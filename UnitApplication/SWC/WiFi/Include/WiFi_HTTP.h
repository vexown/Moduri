#ifndef WIFI_HTTP_H
#define WIFI_HTTP_H

#include <stdint.h>
#include <stdbool.h>
#include "Common.h"

#if (HTTP_ENABLED == ON)

/* Sends an HTTP GET request over TCP */
bool send_http_get_request(const char *host, const char *path);

/* Processes the received HTTP response */
void process_HTTP_response(const uint8_t *buffer, uint16_t length);

#endif /* HTTP_ENABLED */

#endif /* WIFI_HTTP_H */