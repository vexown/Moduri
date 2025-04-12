/**
 * File: WiFi_OTA_download.c
 * Description: Implementation of the firmware download process over WiFi using mbedTLS and our lwIP-based TCP stack.
 */

/*******************************************************************************/
/*                                 INCLUDES                                    */
/*******************************************************************************/

/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* SDK includes */
#include "pico/cyw43_arch.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"

/* MBEDTLS includes */
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

/* Project includes */
#include "flash_operations.h"
#include "WiFi_OTA_download.h"
#include "WiFi_TCP.h"
#include "WiFi_Common.h"
#include "Common.h"

/* Disable this module if OTA is not enabled */
#if (OTA_ENABLED == ON)

/**********************************************************************/
/*                          DEFINES                                   */
/**********************************************************************/
#define FIRMWARE_PATH           "/firmware.bin"
#define CHUNK_SIZE              65536       /* 32KB read chunks */
#define MAX_FIRMWARE_SIZE       1048576     /* 1MB maximum size */
#define FLASH_TARGET_OFFSET     0x00220000  /* Flash write address */
#define HTTP_HEADER_MAX_SIZE    2048        /* Max HTTP header size */
#define HANDSHAKE_TIMEOUT_MS    15000       /* 15 seconds for TLS handshake */
#define READ_TIMEOUT_MS         30000       /* 30 seconds for reads */
#define RECONNECT_MAX_ATTEMPTS  3           /* Maximum reconnection attempts */

/**********************************************************************/
/*                   STATIC VARIABLE DECLARATIONS                     */
/**********************************************************************/
/* Self-signed certificate for the Apache server */
static const unsigned char *ca_cert = 
    "-----BEGIN CERTIFICATE-----\n"
    "MIIECTCCAvGgAwIBAgIUB3E9K+/DmBkIOefd2ZOLzTZncuUwDQYJKoZIhvcNAQEL\n"
    "BQAwgZMxCzAJBgNVBAYTAlBMMRUwEwYDVQQIDAxXaWVsa29wb2xza2ExFTATBgNV\n"
    "BAcMDFNpZXJvc3pld2ljZTEPMA0GA1UECgwGTW9kdXJpMQwwCgYDVQQLDANPVEEx\n"
    "FjAUBgNVBAMMDTE5Mi4xNjguMS4xOTQxHzAdBgkqhkiG9w0BCQEWEHZleG93bkBn\n"
    "bWFpbC5jb20wHhcNMjUwMjIzMTAzODUzWhcNMjYwMjIzMTAzODUzWjCBkzELMAkG\n"
    "A1UEBhMCUEwxFTATBgNVBAgMDFdpZWxrb3BvbHNrYTEVMBMGA1UEBwwMU2llcm9z\n"
    "emV3aWNlMQ8wDQYDVQQKDAZNb2R1cmkxDDAKBgNVBAsMA09UQTEWMBQGA1UEAwwN\n"
    "MTkyLjE2OC4xLjE5NDEfMB0GCSqGSIb3DQEJARYQdmV4b3duQGdtYWlsLmNvbTCC\n"
    "ASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANfxG3inpJCsvN2ESzvwH21P\n"
    "c64ZCUgZvp8Q/HRwuB/pdK4+qmdDvmUV52n+p9szjptNMONewBby+QYvMLjO0lbi\n"
    "9aKz1Ll8f/+7KIAEZLynctFzdXEAlApF78yX0t5yMdVMzlv7gPYrt8W6L8zd5nxK\n"
    "1Uy17NeXIbPQUcQTU47xOm7W0SOxbamcr4jmxHDz3tAguy1a+DpDNTzHOgRo0+7W\n"
    "S3yQ/cuSUmBH3ItfAJDTVxmc1Dl4Djjjyw1CEBhdPyjBfkt2PNmHmiCsTYN2lW2V\n"
    "pq888+9WlFFvIOapR/yC30GR7KlUsRzjdeXqyNf2J0dTq11dJAAqaJB9BytMPuUC\n"
    "AwEAAaNTMFEwHQYDVR0OBBYEFGmYQyvPVIY651YH+DPEDNV51YF7MB8GA1UdIwQY\n"
    "MBaAFGmYQyvPVIY651YH+DPEDNV51YF7MA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZI\n"
    "hvcNAQELBQADggEBAJ87nkPJHMa7wqi8SIraQfM+wo3yzWh6oxuznz8pwgD+sjpt\n"
    "bm/jrTugyWzEF0nkTDT9CLDgdnZwz5joAfGKZgaegXsJzpv02VxMgpm6rLBFVeeK\n"
    "wJUGulodjVRxKQxIJM6oVKOplK7LE2MyXNRtt7ccsbqHtXTYxsqhTzELadYNShou\n"
    "RY17M1pM5BI8k20+lP58ckEnuaNlPV6Gm0r1LV8ckCR9sJKDU9bNzVfO77lj2OXf\n"
    "pYdGDG1mkVFhF6Ej7KtOPOeWO29fVNkwdSGjWMIbxIQhNaHN1T4T2Q6W4y4EiXKk\n"
    "KktqIH7GPGgMmtHo5uofNt2EUrzPMHQwDz9SxN4=\n"
    "-----END CERTIFICATE-----\n";


/**********************************************************************/
/*                  STATIC FUNCTION DECLARATIONS                      */
/**********************************************************************/

/**
 * Find the end of HTTP headers in a buffer
 * 
 * This function scans through an HTTP response buffer to locate the boundary
 * between HTTP headers and the message body. In HTTP/1.1, headers are separated
 * from the body by a specific sequence: CRLF CRLF (carriage return + line feed twice).
 * 
 * Specifically, it searches for the "\r\n\r\n" pattern which indicates:
 * 1. The end of the last header field
 * 2. An empty line (\r\n)
 * 3. The beginning of the actual content (firmware binary data in our case)
 * 
 * @param buf Buffer containing HTTP response (headers + possibly some body data)
 * @param len Buffer length
 * @return Pointer to the first byte after headers (start of body), or NULL if header end not found
 */
static const char* find_header_end(const uint8_t *buf, size_t len);

/**********************************************************************/
/*                  GLOBAL FUNCTION DEFINITIONS                       */
/**********************************************************************/

int download_firmware(void) {
    int result = -1;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    static char request[512];
    static uint8_t flash_buffer[CHUNK_SIZE];
    size_t flash_buf_pos = 0;
    size_t total_received = 0;
    size_t content_length = 0;
    size_t expected_size = 0; 
    bool headers_processed = false;
    static uint8_t header_buffer[HTTP_HEADER_MAX_SIZE];
    size_t header_buf_pos = 0;
    TickType_t timeout_time;
    int reconnect_attempts = 0;
    
    LOG("=== Starting firmware download ===\n");
    LOG("Target server: https://%s%s\n", OTA_HTTPS_SERVER_IP_ADDRESS, FIRMWARE_PATH);

    watchdog_disable(); // Disable watchdog during OTA process

    // Initialize TLS components
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    // Seed the random number generator
    LOG("Seeding random number generator...\n");
    if(mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, 
                             (const unsigned char *)"pico_w_ota", 9) != 0) {
        LOG("Failed to seed RNG\n");
        goto cleanup;
    }

    // Load CA certificate
    LOG("Loading CA certificate...\n");
    if (mbedtls_x509_crt_parse(&cacert, (const unsigned char *)ca_cert, strlen(ca_cert) + 1)) 
    {
        LOG("Failed to parse CA certificate\n");
        goto cleanup;
    }

    // Make sure TCP client is initialized and connected
    if (!clientGlobal) {
        LOG("TCP client not initialized\n");
        goto cleanup;
    }
    
    // Setup TLS configuration
    LOG("Setting up TLS configuration...\n");
    if(mbedtls_ssl_config_defaults(&conf,
                                   MBEDTLS_SSL_IS_CLIENT,
                                   MBEDTLS_SSL_TRANSPORT_STREAM,
                                   MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
        LOG("Failed to set TLS configuration defaults\n");
        goto cleanup;
    }

    // Setup CA certificate
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    // Apply configuration to SSL context
    if(mbedtls_ssl_setup(&ssl, &conf) != 0) {
        LOG("Failed to setup SSL\n");
        goto cleanup;
    }

    // Set hostname for verification
    if(mbedtls_ssl_set_hostname(&ssl, OTA_HTTPS_SERVER_IP_ADDRESS) != 0) {
        LOG("Failed to set hostname\n");
        goto cleanup;
    }

    /**
     * Configure the SSL I/O callbacks for TLS communication
     * 
     * This function links the SSL/TLS layer with our TCP client's network functions.
     * It sets up how encrypted data will be sent and received over the network:
     * - ssl: The SSL context that will use these callbacks
     * - clientGlobal: Connection context passed to callbacks (contains socket info)
     * - tcp_send_mbedtls_callback: Function called when SSL needs to send data
     * - tcp_receive_mbedtls_callback: Function called when SSL needs to receive data
     * - NULL: blocking read callback with timeout, not used here (non-blocking)
     * 
     * This is a critical part of the TLS setup that allows mbedTLS to handle the 
     * encryption/decryption while delegating actual network I/O to our TCP layer.
     */
    mbedtls_ssl_set_bio(&ssl, clientGlobal, 
                        (mbedtls_ssl_send_t*)tcp_send_mbedtls_callback,
                        (mbedtls_ssl_recv_t*)tcp_receive_mbedtls_callback,
                        NULL);

    // Perform TLS handshake
    LOG("Performing TLS handshake...\n");
    timeout_time = xTaskGetTickCount() + pdMS_TO_TICKS(HANDSHAKE_TIMEOUT_MS);
    int ret;
    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            LOG("TLS handshake failed: %08x\n", -ret);
            goto cleanup;
        }
        
        // Check for timeout
        if (xTaskGetTickCount() >= timeout_time) {
            LOG("TLS handshake timeout\n");
            goto cleanup;
        }
        
        // Give other tasks a chance to run
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Send HTTP request
    snprintf(request, sizeof(request), 
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: keep-alive\r\n"
        "Cache-Control: no-cache\r\n"
        "User-Agent: Pico-W-OTA/1.0\r\n"
        "\r\n", 
        FIRMWARE_PATH, OTA_HTTPS_SERVER_IP_ADDRESS);
    
    if (mbedtls_ssl_write(&ssl, (const unsigned char *)request, strlen(request)) <= 0) {
        LOG("Failed to send HTTP request\n");
        goto cleanup;
    }

    // Main download loop
    timeout_time = xTaskGetTickCount() + pdMS_TO_TICKS(READ_TIMEOUT_MS);
    while (xTaskGetTickCount() < timeout_time) 
    {
        // Read data from TLS connection
        uint8_t recv_buffer[1024];
        ret = mbedtls_ssl_read(&ssl, recv_buffer, sizeof(recv_buffer));
        
        if (ret > 0) {
            // Reset timeout on successful read
            timeout_time = xTaskGetTickCount() + pdMS_TO_TICKS(READ_TIMEOUT_MS);
            
            // Process the received data
            size_t data_pos = 0;
            
            // Process HTTP headers if not done yet
            if (!headers_processed) {
                // Copy data to header buffer
                while (data_pos < ret && header_buf_pos < HTTP_HEADER_MAX_SIZE) {
                    header_buffer[header_buf_pos++] = recv_buffer[data_pos++];
                    
                    // Check if we've reached the end of the headers
                    if (header_buf_pos >= 4 && 
                        header_buffer[header_buf_pos-4] == '\r' && 
                        header_buffer[header_buf_pos-3] == '\n' && 
                        header_buffer[header_buf_pos-2] == '\r' && 
                        header_buffer[header_buf_pos-1] == '\n') {
                        
                        // Null-terminate the header buffer
                        header_buffer[header_buf_pos] = 0;
                        
                        // Parse status code
                        if (strncmp((char*)header_buffer, "HTTP/1.1 200", 12) != 0) {
                            LOG("HTTP error: %.*s\n", 32, header_buffer);
                            result = -1;
                            goto cleanup;
                        }
                        
                        // Parse content length
                        char *cl_str = strstr((char*)header_buffer, "Content-Length:");
                        if (cl_str) {
                            content_length = strtoul(cl_str + 15, NULL, 10);
                            expected_size = content_length;
                            LOG("Firmware size from Content-Length: %zu bytes\n", expected_size);
                        } else {
                            LOG("Warning: Content-Length header not found\n");
                            result = -1;
                            goto cleanup;
                        }

                        if (content_length > MAX_FIRMWARE_SIZE) {
                            LOG("Error: Content-Length (%zu) exceeds maximum firmware size (%d)\n", 
                                content_length, MAX_FIRMWARE_SIZE);
                            result = -1;
                            goto cleanup;
                        }
                        
                        LOG("HTTP headers processed, starting firmware download...\n");
                        headers_processed = true;
                        break;
                    }
                }
            }
            
            // If headers are processed, we can save firmware data
            if (headers_processed && data_pos < ret) {
                // Process firmware data
                size_t bytes_left = ret - data_pos;
                
                // Copy data to flash buffer
                while (bytes_left > 0) 
                {
                    size_t copy_size = CHUNK_SIZE - flash_buf_pos;
                    if (copy_size > bytes_left) copy_size = bytes_left;
                    
                    memcpy(flash_buffer + flash_buf_pos, recv_buffer + data_pos, copy_size);
                    flash_buf_pos += copy_size;
                    data_pos += copy_size;
                    bytes_left -= copy_size;
                    
                    // If flash buffer is full, write it to flash
                    if (flash_buf_pos == CHUNK_SIZE) 
                    {
                        uint32_t write_offset = FLASH_TARGET_OFFSET + total_received;
                        LOG("Writing %d bytes to flash at offset 0x%x\n", CHUNK_SIZE, write_offset);
                        
                        if (!write_to_flash(write_offset, flash_buffer, CHUNK_SIZE)) {
                            LOG("Flash write failed\n");
                            result = -1;
                            goto cleanup;
                        }
                        
                        total_received += CHUNK_SIZE;
                        flash_buf_pos = 0;
                        
                        // Progress reporting
                        if (total_received % (CHUNK_SIZE * 10) == 0) {
                            LOG("Download progress: %zu/%d bytes (%.1f%%)\n", 
                                total_received, expected_size, 
                                (float)total_received * 100 / expected_size);
                        }
                    }
                }
                if (headers_processed && total_received >= expected_size) 
                {
                    LOG("Download complete, received %zu of %zu bytes\n", total_received, expected_size);
                    break;
                }
            }
        } else if (ret == 0) {
            // Connection closed by server
            LOG("Connection closed by server\n");
            break;
        } else if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            // Non-blocking operation in progress, try again later
            vTaskDelay(pdMS_TO_TICKS(10));
        } else if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
            // Connection reset - implement reconnection logic
            LOG("Connection reset, attempting to reconnect (%d/%d)...\n", 
                reconnect_attempts + 1, RECONNECT_MAX_ATTEMPTS);
                
            if (++reconnect_attempts >= RECONNECT_MAX_ATTEMPTS) {
                LOG("Maximum reconnection attempts reached\n");
                break;
            }
            
            // Reset TLS session
            mbedtls_ssl_session_reset(&ssl);
            
            // Attempt to reconnect TCP client
            if (tcp_client_is_connected()) {
                tcp_client_disconnect();
            }
            
            // Wait before reconnecting
            vTaskDelay(pdMS_TO_TICKS(1000));
            
            if (!tcp_client_connect(OTA_HTTPS_SERVER_IP_ADDRESS, OTA_HTTPS_SERVER_PORT)) {
                LOG("Failed to reconnect TCP client\n");
                break;
            }
            
            // Try handshake again
            LOG("Performing handshake after reconnect...\n");
            timeout_time = xTaskGetTickCount() + pdMS_TO_TICKS(HANDSHAKE_TIMEOUT_MS);
            
            while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
                if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                    LOG("TLS handshake failed after reconnect: %08x\n", -ret);
                    break;
                }
                
                if (xTaskGetTickCount() >= timeout_time) {
                    LOG("TLS handshake timeout after reconnect\n");
                    break;
                }
                
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            
            if (ret != 0) {
                break;
            }
            
            // Send HTTP request with Range header to resume download
            snprintf(request, sizeof(request), 
                "GET %s HTTP/1.1\r\n"
                "Host: %s\r\n"
                "Connection: close\r\n"
                "Range: bytes=%zu-%zu\r\n"
                "Cache-Control: no-cache\r\n"
                "User-Agent: Pico-W-OTA/1.0\r\n"
                "\r\n", 
                FIRMWARE_PATH, OTA_HTTPS_SERVER_IP_ADDRESS,
                total_received, expected_size - 1);
            
            if (mbedtls_ssl_write(&ssl, (const unsigned char *)request, strlen(request)) <= 0) {
                LOG("Failed to send HTTP resume request\n");
                break;
            }
            
            // Reset headers for new response
            headers_processed = false;
            header_buf_pos = 0;
            
            // Reset timeout
            timeout_time = xTaskGetTickCount() + pdMS_TO_TICKS(READ_TIMEOUT_MS);
        } else {
            LOG("SSL read error: %d\n", ret);
            break;
        }
    }
    
    // Write any remaining data in the flash buffer
    if (flash_buf_pos > 0) {
        uint32_t write_offset = FLASH_TARGET_OFFSET + total_received;
        LOG("Writing final %zu bytes to flash at offset 0x%x\n", flash_buf_pos, write_offset);
        
        if (!write_to_flash(write_offset, flash_buffer, flash_buf_pos)) {
            LOG("Final flash write failed\n");
            result = -1;
            goto cleanup;
        }
        
        total_received += flash_buf_pos;
    }
    
    // Check if we received all the expected data
    if (total_received == expected_size) {
        LOG("Firmware download completed successfully: %zu bytes\n", total_received);
        result = 0;  // Success
    } else if (total_received > 0) {
        LOG("Firmware download partial: %zu/%d bytes\n", total_received, expected_size);
        result = total_received;  // Partial success
    } else {
        LOG("Firmware download failed\n");
        result = -1;  // Failure
    }
    
cleanup:
    // Try to close the SSL connection gracefully
    if (clientGlobal && tcp_client_is_connected()) {
        LOG("Closing TLS connection...\n");
        mbedtls_ssl_close_notify(&ssl);
    }
    
    // Free TLS resources
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_x509_crt_free(&cacert);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    
    LOG("=== Firmware download complete ===\n");
    watchdog_enable(((uint32_t)2000), true);

    return result;
}


/**********************************************************************/
/*                  STATIC FUNCTION DEFINITIONS                       */
/**********************************************************************/

static const char* find_header_end(const uint8_t *buf, size_t len) 
{
    /* Check each position in the buffer for the header termination sequence
     * We need at least 4 bytes to check for \r\n\r\n, so stop 3 bytes before the end */
    for (size_t i = 0; i < len - 3; i++) 
    {
        /* Look for the sequence: CR LF CR LF (\r\n\r\n) */
        if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n') 
        {
            /* Found! Return pointer to the first byte AFTER the sequence.
             * This is where the actual HTTP body (firmware binary) begins */
            return (const char *)(buf + i + 4);  // Point to start of body
        }
    }

    /* If we've searched the entire buffer without finding the pattern,
     * it means we don't have the complete headers yet - more data needed */
    return NULL; // Headers end not found in this buffer
}

#endif // OTA_ENABLED