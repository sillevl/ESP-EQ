#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * WiFi Manager Configuration
 */
#define WIFI_SSID_MAX_LEN       32
#define WIFI_PASSWORD_MAX_LEN   64
#define WIFI_MAXIMUM_RETRY      5

/**
 * Initialize WiFi Manager
 * Loads WiFi credentials from NVS and starts connection
 * 
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_init(void);

/**
 * Set WiFi credentials and connect
 * 
 * @param ssid WiFi network SSID
 * @param password WiFi network password
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_set_credentials(const char* ssid, const char* password);

/**
 * Disconnect from WiFi
 * 
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * Check if WiFi is connected
 * 
 * @return true if connected, false otherwise
 */
bool wifi_manager_is_connected(void);

/**
 * Get current WiFi SSID
 * 
 * @param ssid Buffer to store SSID (min WIFI_SSID_MAX_LEN bytes)
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_get_ssid(char* ssid);

/**
 * Get WiFi IP address as string
 * 
 * @param ip_str Buffer to store IP address (min 16 bytes)
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_get_ip(char* ip_str);

#endif // WIFI_MANAGER_H
