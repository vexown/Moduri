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
#define HTTP_REQUEST_MAX_SIZE   512         /* Max HTTP request size */
#define HANDSHAKE_TIMEOUT_MS    15000       /* 15 seconds for TLS handshake */
#define READ_TIMEOUT_MS         30000       /* 30 seconds for reads */
#define RECONNECT_MAX_ATTEMPTS  3           /* Maximum reconnection attempts */

/**********************************************************************/
/*                   STATIC VARIABLE DECLARATIONS                     */
/**********************************************************************/
/* Self-signed certificate for the Apache server */
static const unsigned char *CA_cert_OTA_server_raw = 
    (const unsigned char *) // cast to unsigned char since mbedtls_x509_crt_parse expects it
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
 * Initializes the mbedTLS context, configuration, certificates, and RNG.
 * Sets up the SSL context for the OTA download connection.
 *
 * @param p_ssl_context Pointer to the mbedTLS SSL context structure.
 * @param p_ssl_config Pointer to the mbedTLS SSL configuration structure.
 * @param p_CA_cert_OTA_server Pointer to the mbedTLS X.509 certificate structure for the CA.
 * @param p_ctr_drbg_context Pointer to the mbedTLS CTR_DRBG context structure.
 * @param p_entropy_context Pointer to the mbedTLS entropy context structure.
 * @return 0 on success, -1 on failure.
 */
static int initialize_mbedtls_context(mbedtls_ssl_context *p_ssl_context,
                                      mbedtls_ssl_config *p_ssl_config,
                                      mbedtls_x509_crt *p_CA_cert_OTA_server,
                                      mbedtls_ctr_drbg_context *p_ctr_drbg_context,
                                      mbedtls_entropy_context *p_entropy_context);

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

int download_firmware(void) 
{
    /******************************* Variable declarations/definitions *******************************/
    /* Status variable */
    int result = -1;
    
    /* MbedTLS structures */
    /* SSL context is the main structure for an SSL/TLS connection,
     * storing the current session state, negotiated parameters, I/O buffers,
     * cryptographic keys, and connection settings. It maintains the entire 
     * lifecycle of an SSL/TLS connection, including handshake phases, record 
     * processing, and connection termination. This context is initialized with
     * mbedtls_ssl_init() and configured via mbedtls_ssl_setup() before use. */
    mbedtls_ssl_context ssl_context;

    /* SSL configuration that holds settings for the TLS connection including security
     * parameters, certificate verification options, and allowed cipher suites. This serves
     * as a template of settings applied to the ssl_context during setup. It allows
     * configuring the TLS/SSL connection behavior before the handshake begins. */
    mbedtls_ssl_config ssl_config;

    /* X.509 certificate structure that stores and manages the parsed CA (Certificate Authority)
     * certificate used to verify the server's identity. For TLS clients, this holds the trusted
     * root certificates used to validate the server's certificate, establishing a chain of trust. */
    mbedtls_x509_crt CA_cert_OTA_server;

    /* CTR_DRBG (Counter-mode Deterministic Random Bit Generator) is a NIST-standardized 
     * cryptographically secure pseudorandom number generator that uses a block cipher
     * in counter mode operation. As defined in NIST SP 800-90A, it provides strong
     * cryptographic randomness for security-critical applications. Here, it generates
     * random values for the TLS handshake and secure communications. The implementation
     * uses AES as the underlying block cipher. */
    mbedtls_ctr_drbg_context ctr_drbg_context;

    /* Entropy context that accumulates randomness from multiple sources (hardware or system)
     * to provide seed material for the DRBG. This structure is critical for initializing
     * cryptographically secure random number generation by collecting true entropy from
     * the system, which is then used to seed the deterministic random bit generator. */
    mbedtls_entropy_context entropy_context;

    /* HTTP variables */
    static char http_request[HTTP_REQUEST_MAX_SIZE];
    static uint8_t http_header_buffer[HTTP_HEADER_MAX_SIZE];
    bool http_headers_processed = false;
    size_t header_buf_pos = 0;

    /* Flash variables */
    static uint8_t flash_buffer[CHUNK_SIZE];
    size_t flash_buf_pos = 0;

    /* Download variables */
    size_t total_received = 0;
    size_t content_length = 0;
    size_t expected_size = 0; 
    TickType_t timeout_value;
    int reconnect_attempts = 0;
    
    /******************************* Initialization *******************************/
    
    LOG("=== Starting firmware download ===\n");
    LOG("Target server: https://%s%s\n", OTA_HTTPS_SERVER_IP_ADDRESS, FIRMWARE_PATH);

    watchdog_disable(); // Disable watchdog during OTA process

    /* Initialize mbedTLS */
    if (initialize_mbedtls_context(&ssl_context, &ssl_config, &CA_cert_OTA_server, &ctr_drbg_context, &entropy_context) != 0)
    {
        LOG("Failed to initialize mbedTLS context\n");
        goto cleanup;
    }

    /****************************** Download Process ******************************/

    /* Perform the TLS handshake. 
     * This process might require multiple steps when using non-blocking I/O, as configured with mbedtls_ssl_set_bio.
     * The mbedtls_ssl_handshake function will return specific error codes MBEDTLS_ERR_SSL_WANT_READ or MBEDTLS_ERR_SSL_WANT_WRITE if it needs
     * to wait for network data to be received or sent, respectively. This loop handles these non-fatal "want" conditions by calling the function again
     * after short delay, allowing other tasks (like the network stack) to run and potentially satisfy the read/write requirement.
     * The loop continues until the handshake completes successfully (ret == 0), a fatal error occurs (ret is neither 0, WANT_READ, nor WANT_WRITE)
     * or the timeout expires. */
    LOG("Performing TLS handshake...\n");
    timeout_value = xTaskGetTickCount() + pdMS_TO_TICKS(HANDSHAKE_TIMEOUT_MS);
    int handshake_result = mbedtls_ssl_handshake(&ssl_context);
    while (handshake_result != 0) 
    {
        /* Check for fatal errors, non-fatal "want" conditions or timeout */
        if ((handshake_result != MBEDTLS_ERR_SSL_WANT_READ) && (handshake_result != MBEDTLS_ERR_SSL_WANT_WRITE)) 
        {
            LOG("TLS handshake failed: %08x\n", -handshake_result);
            goto cleanup;
        }
        else if (xTaskGetTickCount() >= timeout_value)
        {
            LOG("TLS handshake timeout\n");
            goto cleanup;
        }
        else
        {
            /* Continue the handshake process */
            handshake_result = mbedtls_ssl_handshake(&ssl_context);
        }
        
        /* Give other tasks a chance to run while we wait for handshake data to be available */
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Send HTTP http_request
    snprintf(http_request, sizeof(http_request), 
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: keep-alive\r\n"
        "Cache-Control: no-cache\r\n"
        "User-Agent: Pico-W-OTA/1.0\r\n"
        "\r\n", 
        FIRMWARE_PATH, OTA_HTTPS_SERVER_IP_ADDRESS);
    
    if (mbedtls_ssl_write(&ssl_context, (const unsigned char *)http_request, strlen(http_request)) <= 0) {
        LOG("Failed to send HTTP http_request\n");
        goto cleanup;
    }

    // Main download loop
    timeout_value = xTaskGetTickCount() + pdMS_TO_TICKS(READ_TIMEOUT_MS);
    int ret;
    while (xTaskGetTickCount() < timeout_value) 
    {
        // Read data from TLS connection
        uint8_t recv_buffer[1024];
        ret = mbedtls_ssl_read(&ssl_context, recv_buffer, sizeof(recv_buffer));
        
        if (ret > 0) {
            // Reset timeout on successful read
            timeout_value = xTaskGetTickCount() + pdMS_TO_TICKS(READ_TIMEOUT_MS);
            
            // Process the received data
            size_t data_pos = 0;
            
            // Process HTTP headers if not done yet
            if (!http_headers_processed) {
                // Copy data to header buffer
                while (data_pos < ret && header_buf_pos < HTTP_HEADER_MAX_SIZE) {
                    http_header_buffer[header_buf_pos++] = recv_buffer[data_pos++];
                    
                    // Check if we've reached the end of the headers
                    if (header_buf_pos >= 4 && 
                        http_header_buffer[header_buf_pos-4] == '\r' && 
                        http_header_buffer[header_buf_pos-3] == '\n' && 
                        http_header_buffer[header_buf_pos-2] == '\r' && 
                        http_header_buffer[header_buf_pos-1] == '\n') {
                        
                        // Null-terminate the header buffer
                        http_header_buffer[header_buf_pos] = 0;
                        
                        // Parse status code
                        if (strncmp((char*)http_header_buffer, "HTTP/1.1 200", 12) != 0) {
                            LOG("HTTP error: %.*s\n", 32, http_header_buffer);
                            result = -1;
                            goto cleanup;
                        }
                        
                        // Parse content length
                        char *cl_str = strstr((char*)http_header_buffer, "Content-Length:");
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
                        http_headers_processed = true;
                        break;
                    }
                }
            }
            
            // If headers are processed, we can save firmware data
            if (http_headers_processed && data_pos < ret) {
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
                if (http_headers_processed && total_received >= expected_size) 
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
            mbedtls_ssl_session_reset(&ssl_context);
            
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
            timeout_value = xTaskGetTickCount() + pdMS_TO_TICKS(HANDSHAKE_TIMEOUT_MS);
            
            while ((ret = mbedtls_ssl_handshake(&ssl_context)) != 0) {
                if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                    LOG("TLS handshake failed after reconnect: %08x\n", -ret);
                    break;
                }
                
                if (xTaskGetTickCount() >= timeout_value) {
                    LOG("TLS handshake timeout after reconnect\n");
                    break;
                }
                
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            
            if (ret != 0) {
                break;
            }
            
            // Send HTTP http_request with Range header to resume download
            snprintf(http_request, sizeof(http_request), 
                "GET %s HTTP/1.1\r\n"
                "Host: %s\r\n"
                "Connection: close\r\n"
                "Range: bytes=%zu-%zu\r\n"
                "Cache-Control: no-cache\r\n"
                "User-Agent: Pico-W-OTA/1.0\r\n"
                "\r\n", 
                FIRMWARE_PATH, OTA_HTTPS_SERVER_IP_ADDRESS,
                total_received, expected_size - 1);
            
            if (mbedtls_ssl_write(&ssl_context, (const unsigned char *)http_request, strlen(http_request)) <= 0) {
                LOG("Failed to send HTTP resume http_request\n");
                break;
            }
            
            // Reset headers for new response
            http_headers_processed = false;
            header_buf_pos = 0;
            
            // Reset timeout
            timeout_value = xTaskGetTickCount() + pdMS_TO_TICKS(READ_TIMEOUT_MS);
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
        mbedtls_ssl_close_notify(&ssl_context);
    }
    
    // Free TLS resources
    mbedtls_ssl_free(&ssl_context);
    mbedtls_ssl_config_free(&ssl_config);
    mbedtls_x509_crt_free(&CA_cert_OTA_server);
    mbedtls_ctr_drbg_free(&ctr_drbg_context);
    mbedtls_entropy_free(&entropy_context);
    
    LOG("=== Firmware download complete ===\n");
    watchdog_enable(((uint32_t)2000), true);

    return result;
}


/**********************************************************************/
/*                  STATIC FUNCTION DEFINITIONS                       */
/**********************************************************************/

static int initialize_mbedtls_context(mbedtls_ssl_context *p_ssl_context,
                                      mbedtls_ssl_config *p_ssl_config,
                                      mbedtls_x509_crt *p_CA_cert_OTA_server,
                                      mbedtls_ctr_drbg_context *p_ctr_drbg_context,
                                      mbedtls_entropy_context *p_entropy_context)
{
    /* Initialize mbedTLS structures to a clean, defined state */
    mbedtls_ssl_init(p_ssl_context);
    mbedtls_ssl_config_init(p_ssl_config);
    mbedtls_x509_crt_init(p_CA_cert_OTA_server);
    mbedtls_ctr_drbg_init(p_ctr_drbg_context);
    mbedtls_entropy_init(p_entropy_context);

    /* Seed the CTR_DRBG (Counter-mode Deterministic Random Bit Generator).
     * This function initializes the internal state of the DRBG using entropy
     * gathered from the system, making its output cryptographically secure.
     *
     * Explanation of the crucial parameters:
     * - mbedtls_entropy_func: The callback function used to retrieve entropy
     *                         from the registered sources within the entropy_context.
     * - p_entropy_context: The entropy context holding the accumulated randomness
     *                     sources (e.g., hardware RNG, timing variations).
     * - "pico_w_ota": A personalization string. This is application-specific data
     *                 mixed with the entropy during seeding. Its purpose, as defined
     *                 in NIST SP 800-90A, is to make this specific DRBG instance unique.
     *                 Using "pico_w_ota" helps differentiate this RNG instance used for
     *                 the OTA download process from any other potential RNG usage on
     *                 the device.
     *
     * A successful seeding is critical for generating unpredictable random numbers
     * required for the TLS handshake (e.g., nonces, key generation). */
    LOG("Seeding random number generator...\n");
    const unsigned char *personalization_string = (const unsigned char *)"pico_w_ota";
    const size_t personalization_string_len = strlen((const char *)personalization_string);
    if(mbedtls_ctr_drbg_seed(p_ctr_drbg_context, mbedtls_entropy_func, p_entropy_context, personalization_string, personalization_string_len) != 0) 
    {
        LOG("Failed to seed RNG\n");
        return -1;
    }

    /**
     * Parse the CA certificate for the OTA server.
     * 
     * This function decodes one or more X.509 certificates from a given buffer (CA_cert_OTA_server_raw) and populates an `mbedtls_x509_crt` 
     * (p_CA_cert_OTA_server) structure. Input buffer can contain a cert in either PEM (Privacy-Enhanced Mail, typically Base64 encoded 
     * with -----BEGIN/END----- markers) or DER (Distinguished Encoding Rules, usually a hex string) format. The function auto detects the format.
     * 
     * p_CA_cert_OTA_server structure with the parsed cert will later be passed to `mbedtls_ssl_conf_ca_chain` to configure the
     * SSL/TLS context (`p_ssl_config`). This tells the TLS client (our Pico W) which CAs it should trust when verifying the certificate 
     * presented by the OTA server during the TLS handshake. This step is essential for authenticating the server and ensuring
     * we are connecting to the legitimate OTA server, preventing man-in-the-middle attacks. */
    LOG("Loading CA certificate...\n");
    size_t CA_cert_OTA_server_raw_len = strlen((const char *)CA_cert_OTA_server_raw) + 1; // +1 for null terminator
    if (mbedtls_x509_crt_parse(p_CA_cert_OTA_server, CA_cert_OTA_server_raw, CA_cert_OTA_server_raw_len) != 0) 
    {
        LOG("Failed to parse CA certificate\n");
        return -1;
    }
    
    /**
     * Load default SSL/TLS configuration values into the p_ssl_config structure.
     * 
     * This function initializes the `mbedtls_ssl_config` structure (`p_ssl_config`) with a set of standard default values appropriate 
     * for the specified role (client/server), transport type (stream/datagram), and a chosen preset profile. These defaults typically 
     * include enabling secure protocol versions (like TLS 1.2 or higher), selecting a list of strong cipher suites, and setting reasonable 
     * buffer sizes. It serves as a convenient starting point, ensuring the configuration is in a known, sensible state before applying 
     * specific customizations. */
    LOG("Setting up TLS configuration...\n");
    if(mbedtls_ssl_config_defaults(p_ssl_config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0) 
    {
        LOG("Failed to set TLS configuration defaults\n");
        return -1;
    }

    /**
     * Set the certificate verification mode for the SSL/TLS configuration.
     * 
     * This function configures how the peer's certificate chain should be verified during the TLS handshake. It determines the 
     * level of authentication required.
     * 
     * Authentication modes:
     *              - `MBEDTLS_SSL_VERIFY_NONE`: Peer certificate is not checked (Insecure, vulnerable to MITM attacks).
     *              - `MBEDTLS_SSL_VERIFY_OPTIONAL`: Peer certificate is checked if presented, but the handshake continues even 
     *                 if verification fails or no certificate is sent. The verification result is available via `mbedtls_ssl_get_verify_result`.
     *              - `MBEDTLS_SSL_VERIFY_REQUIRED`: Peer must present a certificate, and it must be validly verified against the configured 
     *                 trusted CAs. Handshake fails if verification fails. (Most common for clients). */
    mbedtls_ssl_conf_authmode(p_ssl_config, MBEDTLS_SSL_VERIFY_OPTIONAL);

    /**
     * Set the trusted Certificate Authority (CA) chain for certificate verification.
     * 
     * This function provides the SSL/TLS configuration with the set of CA certificates that should be considered trustworthy for verifying 
     * the peer's certificate chain.
     * 
     * p_CA_cert_OTA_server is the parsed CA certificate structure that contains the trusted root CA certificate(s) used to validate the server's
     * certificate during the TLS handshake. This is crucial for establishing a secure connection and ensuring that the server's identity is
     * authentic.
     * 
     * Last parameter ca_crl is a pointer to an `mbedtls_x509_crl` structure that contains Certificate Revocation Lists (CRLs) associated with the CAs.
     * It can be `NULL` if CRL checking is not required or not available. */
    mbedtls_ssl_conf_ca_chain(p_ssl_config, p_CA_cert_OTA_server, NULL);

    /**
     * Set the random number generator (RNG) callback function for the SSL/TLS configuration.
     * 
     * This function, `mbedtls_ssl_conf_rng`, configures the source of randomness that the SSL/TLS stack will use for cryptographic operations, 
     * such as generating session keys, nonces, and other security-critical values during the handshake and subsequent communication.
     * 
     * f_rng argument is a pointer to the RNG function. This function will be called by mbedTLS whenever it needs random bytes. 
     * We use `mbedtls_ctr_drbg_random`, which is the standard function to get random bytes from a CTR-DRBG context. */
    mbedtls_ssl_conf_rng(p_ssl_config, mbedtls_ctr_drbg_random, p_ctr_drbg_context);

    /**
     * Apply the configured settings to the SSL context.
     * 
     * This function takes the settings defined in the `mbedtls_ssl_config` structure (`p_ssl_config`) and applies them to the 
     * `mbedtls_ssl_context` structure (`p_ssl_context`). This effectively prepares the SSL context for initiating a connection using 
     * the specified configuration, including the trusted CA certificates, authentication mode, RNG functions, etc. */
    if(mbedtls_ssl_setup(p_ssl_context, p_ssl_config) != 0) 
    {
        LOG("Failed to apply SSL configuration\n");
        return -1;
    }

    /**
     * Set the hostname to check against the received server certificate. 
     * 
     * It sets the ServerName TLS extension too, if that extension is enabled. (client-side only)
     * 
     * This function informs the SSL context about the hostname (or IP address string) of the server it intends to connect to. 
     * This serves two primary purposes:
     * 
     * 1. Server Name Indication (SNI): During the TLS handshake (ClientHello message), the client sends this hostname to the server. 
     *    This allows servers hosting multiple secure websites on a single IP address to identify which site the client wants 
     *    and present the corresponding correct certificate.
     * 
     * 2. Hostname Verification: After receiving the server's certificate, the TLS client (if verification is enabled via 
     *    `mbedtls_ssl_conf_authmode`) checks if the hostname set here matches the identity presented in the certificate 
     *    (usually in the Subject Alternative Name (SAN) extension or, as a fallback, the Common Name (CN) field).
     *    This is a crucial security step to prevent Man-in-the-Middle (MitM) attacks, ensuring that the certificate is valid 
     *    not just cryptographically, but specifically for the server we intended to connect to. */
    if(mbedtls_ssl_set_hostname(p_ssl_context, OTA_HTTPS_SERVER_IP_ADDRESS) != 0) 
    {
        LOG("Failed to set hostname\n");
        return -1; // Cleanup will be handled by the caller
    }

    /**
     * Configure the SSL I/O callbacks for TLS communication
     * 
     * This function links the SSL/TLS layer with our TCP client's network functions.
     * It sets up how encrypted data will be sent and received over the network:
     * - p_ssl_context: The SSL context that will use these callbacks
     * - clientGlobal: Connection context passed to callbacks (contains socket info)
     * - tcp_send_mbedtls_callback: Function called when SSL needs to send data
     * - tcp_receive_mbedtls_callback: Function called when SSL needs to receive data
     * - NULL: blocking read callback with timeout, not used here (non-blocking)
     * 
     * This is a critical part of the TLS setup that allows mbedTLS to handle the 
     * encryption/decryption while delegating actual network I/O to our TCP layer.
     */
    mbedtls_ssl_set_bio(p_ssl_context, clientGlobal, 
                        (mbedtls_ssl_send_t*)tcp_send_mbedtls_callback,
                        (mbedtls_ssl_recv_t*)tcp_receive_mbedtls_callback,
                        NULL);

    LOG("mbedTLS context initialized successfully.\n");
    return 0; // Success
}


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