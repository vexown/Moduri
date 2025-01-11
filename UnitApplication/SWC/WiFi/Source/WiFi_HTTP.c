/**
 * File: WiFi_HTTP.c
 * Description: High level implementation for HTTP communication over TCP.
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/

/* WiFi includes */
#include "WiFi_HTTP.h"
#include "WiFi_TCP.h"

/* Misc includes */
#include "Common.h"

/* Standard includes */
#include <stdio.h>
#include <string.h>

/*******************************************************************************/
/*                      GLOBAL FUNCTION DEFINITIONS                            */
/*******************************************************************************/


/**
 * Function: send_http_get_request
 * 
 * Description: Sends an HTTP GET request to the specified host and path.
 * 
 * Parameters:
 *  - host: The host to send the request to.
 *  - path: The path to send the request to.
 * 
 * Returns: bool indicating the success (true) or failure (false) of sending the HTTP GET request.
 * 
 * Example Usage: 
 * 
 *       const char *server_host = PICO_W_AP_TCP_SERVER_ADDRESS;
 *       const char *pathON = "/ledtest?led=1";
 * 
 *       send_http_get_request(server_host, pathON);
 * 
 */
bool send_http_get_request(const char *host, const char *path) 
{
    // Formulate the HTTP GET request
    char http_request[256];
    int len = snprintf(http_request, sizeof(http_request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n\r\n",
        path, host);

    if (len < 0 || len >= sizeof(http_request)) 
    {
        LOG("HTTP request string too long or failed to format.\n");
        return false;
    }

    // Send the HTTP GET request
    err_t result = tcp_client_send(http_request, len);
    if (result != ERR_OK) 
    {
        LOG("Failed to send HTTP request: %d\n", result);
        return false;
    }

    LOG("HTTP request sent successfully:\n%s\n", http_request);

    return true;
}

/**
 * Function: process_HTTP_response
 * 
 * Description: Processes the received HTTP response, extracting the status code and body.
 * 
 * Parameters:
 *  - buffer: Pointer to the buffer containing the received HTTP response.
 *  - length: Length of the received HTTP response.
 * 
 * Returns: void
 * 
 * Example Usage:
 *  
 *       process_HTTP_response(client->receive_buffer, client->receive_length);
 * 
 */
void process_HTTP_response(const uint8_t *buffer, uint16_t length) 
{
    LOG("HTTP Response:\n%.*s\n", length, buffer);

    // Example: Extract HTTP status code
    const char *status_line = (const char *)buffer;
    if (strncmp(status_line, "HTTP/1.1", 8) == 0) 
    {
        int status_code;
        if (sscanf(status_line, "HTTP/1.1 %d", &status_code) == 1) 
        {
            LOG("HTTP Status Code: %d\n", status_code);
        } 
        else 
        {
            LOG("Failed to parse HTTP status code\n");
        }
    }

    // Optionally: Parse the response body if needed
    const char *body = strstr((const char *)buffer, "\r\n\r\n");
    if (body) 
    {
        body += 4; // Skip the "\r\n\r\n"
        LOG("HTTP Body:\n%s\n", body);
    }
}
