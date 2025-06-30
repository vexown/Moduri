/**
 * ****************************************************************************
 * WiFi_AP.c
 *
 * ****************************************************************************
 */

/*******************************************************************************/
/*                                INCLUDES                                     */
/*******************************************************************************/
#include "WiFi_AP.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "Common.h"

/*******************************************************************************/
/*                                 MACROS                                      */
/*******************************************************************************/
#define TAG "WiFi_AP"

/*******************************************************************************/
/*                               DATA TYPES                                    */
/*******************************************************************************/

/*******************************************************************************/
/*                            STATIC VARIABLES                                 */
/*******************************************************************************/
static httpd_handle_t server = NULL;

/*******************************************************************************/
/*                        STATIC FUNCTION DECLARATIONS                         */
/*******************************************************************************/
static esp_err_t root_handler(httpd_req_t *req);
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data);

/*******************************************************************************/
/*                            STATIC VARIABLES                                 */
/*******************************************************************************/
static httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_handler,
    .user_ctx  = NULL
};

/*******************************************************************************/
/*                        STATIC FUNCTION DEFINITIONS                          */
/*******************************************************************************/
/**
 * @brief HTTP GET handler for the root URI ('/')
 *
 * This function sends a simple HTML page as a response to HTTP GET requests
 * to the root URI. It demonstrates a basic web page served from the ESP32 AP.
 *
 * @param req Pointer to the HTTP request structure
 * @return ESP_OK on success
 */
static esp_err_t root_handler(httpd_req_t *req)
{
    const char* html = "<html><head><title>ESP32 AP</title></head>"
                      "<body><h1>Hello World from ESP32!</h1>"
                      "<p>This is a simple HTTP server running on an ESP32 in Access Point mode.</p>"
                      "</body></html>";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    
    return ESP_OK;
}

/**
 * @brief WiFi event handler for AP events
 *
 * Handles station connect and disconnect events for the WiFi AP.
 * Logs when a station connects or disconnects from the AP.
 *
 * @param arg User-defined argument (unused)
 * @param event_base Event base (should be WIFI_EVENT)
 * @param event_id Event ID (WIFI_EVENT_AP_STACONNECTED or WIFI_EVENT_AP_STADISCONNECTED)
 * @param event_data Event-specific data
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        LOG("Station connected, AID=%d\n", event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        LOG("Station disconnected, AID=%d\n", event->aid);
    }
}

/*******************************************************************************/
/*                        GLOBAL FUNCTION DEFINITIONS                          */
/*******************************************************************************/
/**
 * @brief Initialize the ESP32 WiFi Access Point (AP)
 *
 * This function initializes the NVS, TCP/IP stack, event loop, and WiFi driver.
 * It configures the AP with the provided SSID, password, channel, and max connections.
 * Registers event handlers for WiFi events. Starts the WiFi AP.
 *
 * @param config Pointer to a wifi_ap_custom_config_t structure containing AP settings
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_ap_init(wifi_ap_custom_config_t *config)
{
    esp_err_t ret = ESP_OK;
    
    /* Initialize NVS (Non-Volatile Storage) */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    /* Initialize TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_init());
    
    /* Create default event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    /* Create default WiFi AP network interface */
    esp_netif_create_default_wifi_ap();
    
    /* Configure WiFi driver with default settings */
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    
    /* Register WiFi event handler for all WiFi events */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                       ESP_EVENT_ANY_ID,
                                                       &wifi_event_handler,
                                                       NULL,
                                                       NULL));
    
    /* Configure WiFi AP settings */
    wifi_config_t wifi_config = {0};
    
    /* Set SSID and password */
    strncpy((char*)wifi_config.ap.ssid, config->ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = strlen(config->ssid);
    
    if (config->password != NULL && strlen(config->password) > 0) {
        strncpy((char*)wifi_config.ap.password, config->password, sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    wifi_config.ap.max_connection = config->max_connections;
    wifi_config.ap.channel = config->channel;
    
    /* Set WiFi mode to AP and apply configuration */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    
    /* Start WiFi AP */
    ESP_ERROR_CHECK(esp_wifi_start());
    
    LOG("WiFi AP started with SSID: %s, channel: %d\n", config->ssid, config->channel);
    
    return ESP_OK;
}

/**
 * @brief Start the HTTP server
 *
 * Initializes and starts the HTTP server on the default port. Registers URI handlers.
 *
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    LOG("Starting HTTP server on port: %d\n", config.server_port);
    
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &root);
        return ESP_OK;
    }
    
    LOG("Error starting HTTP server!\n");
    return ESP_FAIL;
}

/**
 * @brief Stop the HTTP server
 *
 * Stops the HTTP server if it is running and releases resources.
 *
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if server was not running
 */
esp_err_t http_server_stop(void)
{
    if (server != NULL) {
        httpd_stop(server);
        server = NULL;
        return ESP_OK;
    }
    
    return ESP_ERR_INVALID_STATE;
}