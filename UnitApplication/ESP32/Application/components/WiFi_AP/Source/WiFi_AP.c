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
#include <inttypes.h>
#include <unistd.h>
#include "cJSON.h"

/*******************************************************************************/
/*                                 MACROS                                      */
/*******************************************************************************/
#define TAG "WiFi_AP"
#define MAX_CLIENTS 5

/*******************************************************************************/
/*                               DATA TYPES                                    */
/*******************************************************************************/
/**
 * @brief Structure to hold the state of our simple RESTful resource.
 */
typedef struct {
    char message[128];
} rest_status_t;


/*******************************************************************************/
/*                            STATIC VARIABLES                                 */
/*******************************************************************************/
static httpd_handle_t server = NULL;
static TaskHandle_t ws_server_task_handle = NULL;
static int client_fds[MAX_CLIENTS];

/**
 * @brief The state of our simple RESTful resource.
 * In a real application, this might be stored in NVS or a database.
 */
static rest_status_t rest_status = { .message = "OK" };

/*******************************************************************************/
/*                        STATIC FUNCTION DECLARATIONS                         */
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
static esp_err_t root_handler(httpd_req_t *req);

/**
 * @brief WebSocket sender task.
 * 
 * This task periodically sends a counter to all connected WebSocket clients.
 * 
 * @param pvParameters Unused.
 */
static void ws_server_task(void *pvParameters);

/**
 * @brief WebSocket handler function.
 * 
 * This function handles WebSocket frames, echoing back any text message it receives.
 * 
 * @param req Pointer to the HTTP request structure.
 * @return esp_err_t ESP_OK on success, or an error code if an error occurs.
 */
static esp_err_t ws_handler(httpd_req_t *req);


/**
 * @brief RESTful GET handler for the /api/v1/status URI.
 *
 * This function handles GET requests to retrieve the current system status.
 * It demonstrates a stateless, cacheable, read-only operation, which are
 * key principles of REST.
 *
 * @param req Pointer to the HTTP request structure.
 * @return ESP_OK on success.
 */
static esp_err_t status_get_handler(httpd_req_t *req);

/**
 * @brief RESTful PUT handler for the /api/v1/status URI.
 *
 * This function handles PUT requests to update the system status.
 * It demonstrates an idempotent operation where the client sends the full
 * new state of the resource. The request body is expected to be a JSON
 * object with a "message" key.
 *
 * @param req Pointer to the HTTP request structure.
 * @return ESP_OK on success.
 */
static esp_err_t status_put_handler(httpd_req_t *req);


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
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/*******************************************************************************/
/*                            STATIC VARIABLES                                 */
/*******************************************************************************/
static httpd_uri_t root = 
{
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t ws = {
    .uri        = "/ws",
    .method     = HTTP_GET,
    .handler    = ws_handler,
    .user_ctx   = NULL,
    .is_websocket = true
};

/**
 * @brief URI handler for getting the system status.
 * A GET request to this URI will return the current status in JSON format.
 * This is a RESTful "read" operation.
 */
static const httpd_uri_t status_get_uri = {
    .uri      = "/api/v1/status",
    .method   = HTTP_GET,
    .handler  = status_get_handler,
    .user_ctx = NULL
};

/**
 * @brief URI handler for updating the system status.
 * A PUT request to this URI with a JSON body will update the status.
 * This is a RESTful "update" operation. It's idempotent, meaning multiple
 * identical requests will have the same effect as a single one.
 */
static const httpd_uri_t status_put_uri = {
    .uri      = "/api/v1/status",
    .method   = HTTP_PUT,
    .handler  = status_put_handler,
    .user_ctx = NULL
};

/*******************************************************************************/
/*                        STATIC FUNCTION DEFINITIONS                          */
/*******************************************************************************/

static esp_err_t root_handler(httpd_req_t *req)
{
    const char* html = "<html><head><title>ESP32 AP</title>" 
                      "<script>" 
                      "var ws = new WebSocket('ws://' + window.location.host + '/ws');" 
                      "ws.onmessage = function(evt) { document.getElementById('ws-msg').innerHTML = evt.data; };" 
                      "</script>" 
                      "</head>" 
                      "<body><h1>ESP32 WebSocket Server</h1>" 
                      "<p>Server says: <span id='ws-msg'></span></p>" 
                      "</body></html>";
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, strlen(html));
    
    return ESP_OK;
}

/**
 * @brief Creates a JSON representation of the status and sends it.
 */
static esp_err_t send_status_json(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "message", rest_status.message);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t status_get_handler(httpd_req_t *req)
{
    return send_status_json(req);
}

static esp_err_t status_put_handler(httpd_req_t *req)
{
    char buf[150]; // Larger than rest_status.message to detect overflow
    int remaining = req->content_len;

    // Read the request body
    if (remaining >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Request body too large");
        return ESP_FAIL;
    }

    int received = httpd_req_recv(req, buf, remaining);
    if (received <= 0) { // 0 means connection closed, <0 means error
        if (received == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[received] = '\0'; // Null-terminate the buffer

    // Parse the JSON
    cJSON *root = cJSON_Parse(buf);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    // Extract the "message" field
    cJSON *message = cJSON_GetObjectItem(root, "message");
    if (!cJSON_IsString(message) || (message->valuestring == NULL)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "JSON must have a 'message' string");
        return ESP_FAIL;
    }

    // Update the resource state
    strncpy(rest_status.message, message->valuestring, sizeof(rest_status.message) - 1);
    rest_status.message[sizeof(rest_status.message) - 1] = '\0'; // Ensure null-termination
    cJSON_Delete(root);

    // Respond with the updated resource
    return send_status_json(req);
}


static void ws_server_task(void *pvParameters)
{
    uint32_t count = 0;
    char buf[64];

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    while (1)
    {
        snprintf(buf, sizeof(buf), "Counter: %lu", count++);
        ws_pkt.payload = (uint8_t *)buf;
        ws_pkt.len = strlen(buf);

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (client_fds[i] != -1)
            {
                esp_err_t ret = httpd_ws_send_frame_async(server, client_fds[i], &ws_pkt);
                if (ret != ESP_OK)
                {
                    LOG("Client disconnected, fd=%d", client_fds[i]);
                    client_fds[i] = -1;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Send every 100ms
    }
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        LOG("Handshake done, the new connection was opened");
        int fd = httpd_req_to_sockfd(req);
        if (fd < 0) {
            LOG("Failed to get socket descriptor");
            return ESP_FAIL;
        }

        int i;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] == -1) {
                client_fds[i] = fd;
                break;
            }
        }
        if (i == MAX_CLIENTS) {
            LOG("Too many clients!");
            close(fd);
            return ESP_FAIL;
        }

        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        LOG("httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }

    if (ws_pkt.len) {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            LOG("Failed to calloc memory for websocket frame");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            LOG("httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        LOG("Got unexpected packet with message: %s", ws_pkt.payload);
        free(buf);
    }

    return ESP_OK;
}


static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) 
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        LOG("Station connected, AID=%d\n", event->aid);
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) 
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        LOG("Station disconnected, AID=%d\n", event->aid);
    }
    else
    {
        LOG("Unhandled WiFi event: base=%s, id=%ld\n", event_base, event_id);
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
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
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
    
    if (config->password != NULL && strlen(config->password) > 0) 
    {
        strncpy((char*)wifi_config.ap.password, config->password, sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } 
    else
    {
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

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
    }
    
    if (httpd_start(&server, &config) == ESP_OK) 
    {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &ws);
        
        /* Register RESTful API handlers */
        LOG("Registering RESTful API handlers");
        httpd_register_uri_handler(server, &status_get_uri);
        httpd_register_uri_handler(server, &status_put_uri);

        xTaskCreate(ws_server_task, "ws_server", 4096, NULL, 5, &ws_server_task_handle);
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
    if (server != NULL) 
    {
        if (ws_server_task_handle) {
            vTaskDelete(ws_server_task_handle);
            ws_server_task_handle = NULL;
        }
        httpd_stop(server);
        server = NULL;
        return ESP_OK;
    }
    
    return ESP_ERR_INVALID_STATE;
}