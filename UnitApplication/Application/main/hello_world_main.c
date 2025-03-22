/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

/**
 * ****************************************************************************
 * hello_world_main.c
 *
 * Description:
 * This is the main application file for the ESP32 hello world example.
 * It demonstrates basic ESP32 functionality and displays detailed chip information.
 * 
 * The application performs the following tasks:
 * 1. Prints a hello world message
 * 2. Displays detailed chip information (CPU cores, features, revision)
 * 3. Shows flash memory size and type
 * 4. Reports minimum free heap size
 * 5. Performs a countdown and restarts the ESP32
 *
 * This serves as a basic template and testing application for new ESP32 projects.
 * ****************************************************************************
 */

/*******************************************************************************/
/*                                INCLUDES                                     */
/*******************************************************************************/
 #include <stdio.h>
 #include <inttypes.h>
 #include "sdkconfig.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "esp_chip_info.h"
 #include "esp_flash.h"
 #include "esp_system.h"

/*******************************************************************************/
/*                                 MACROS                                      */
/*******************************************************************************/

/*******************************************************************************/
/*                               DATA TYPES                                    */
/*******************************************************************************/

/*******************************************************************************/
/*                        GLOBAL FUNCTION DECLARATIONS                         */
/*******************************************************************************/

/*******************************************************************************/
/*                        STATIC FUNCTION DECLARATIONS                         */
/*******************************************************************************/

/*******************************************************************************/
/*                            STATIC VARIABLES                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                            GLOBAL VARIABLES                                 */
/*******************************************************************************/

/*******************************************************************************/
/*                        STATIC FUNCTION DEFINITIONS                          */
/*******************************************************************************/

/*******************************************************************************/
/*                        GLOBAL FUNCTION DEFINITIONS                          */
/*******************************************************************************/
 
/**
 * ****************************************************************************
 * Function: app_main
 * 
 * Description: Main application entry point. This function is called by the 
 *              ESP-IDF framework after initialization
 * 
 * Parameters:
 *   - none
 * 
 * Returns: void
 * ****************************************************************************
 */
 void app_main(void)
 {
    printf("Hello world!\n");

    esp_chip_info_t chip_info;
    uint32_t flash_size;

    /* Get chip information */
    esp_chip_info(&chip_info);

    /* Print chip information */
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
            (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
            (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);

    /* Get flash size */
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) 
    {
        printf("Get flash size failed");
        return;
    }

    /* Print flash size */
    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
        (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    for (int i = 10; i >= 0; i--) 
    {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    printf("Restarting now.\n");
    fflush(stdout);
    
    esp_restart();
 }