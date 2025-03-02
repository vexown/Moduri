#ifndef WIFI_OTA_DOWNLOAD_H
#define WIFI_OTA_DOWNLOAD_H

/**
 * To start the HTTPS server on the PC: sudo systemctl start apache2
 * Verify the server is running: sudo systemctl status apache2
 * 
 * To stop the HTTPS server on the PC: sudo systemctl stop apache2
 * Verify the server is stopped: sudo systemctl status apache2
 * 
 * Uploading new firmware to the server: sudo cp /home/blankmcu/Repos/Moduri/UnitApplication/output/moduri_bank_A.bin /var/www/html/firmware.bin
 * Ensure Apache can read the file (www-data is the default Apache user on Ubuntu): 
 *      sudo chmown www-data:www-data /var/www/html/firmware.bin
 *      sudo chmod 644 /var/www/html/firmware.bin
 * 
 * To verify the firmware is accessible: 
 *      sudo systemctl restart apache2
 *      open a browser and go to https://192.168.1.194/firmware.bin or use curl: curl -k https://192.1.168.194/firmware.bin -o downloaded_firmware.bin
 * 
 * /etc/ssl/certs is the location of the CA certificates on Ubuntu (which includes our self-signed certificate for the apache server: apache-selfsigned.crt)
 * 
 * Rate Limit the Apache server since Pico cannot process data as fast as the server can send it:
 * Do this:
 *     - sudo a2enmod ratelimit
 *     - sudo a2enmod headers
 *     - add the following to the /etc/apache2/sites-available/default-ssl.conf file (which applies to HTTPS connections, not HTTP):
 *        <Location /firmware.bin>
 *            SetOutputFilter RATE_LIMIT
 *            # Set the download speed in KB/s
 *            SetEnv rate-limit 75
 *            
 *            # Optional: Add headers to prevent caching
 *            Header set Cache-Control "no-cache, no-store, must-revalidate"
 *            Header set Pragma "no-cache"
 *            Header set Expires 0
 *        </Location>
 *     - sudo apache2ctl configtest
 *     - sudo systemctl restart apache2
 * 
 * Verify rate-limit with: curl -o /dev/null -s -w "Time: %{time_total}s\nSpeed: %{speed_download} bytes/sec\n" https://192.168.1.194/firmware.bin -k
 * 
 */

/**
 * @brief Downloads firmware from the server over HTTPS.
 *
 * This function establishes a secure connection to the server,
 * downloads the firmware binary, and returns the number of bytes
 * downloaded. On error, it returns a negative value corresponding
 * to the Mbed TLS error code.
 *
 * @return Number of bytes downloaded on success, negative Mbed TLS error code on failure.
 */
int download_firmware(void);


#endif /* WIFI_OTA_DOWNLOAD_H */