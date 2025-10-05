#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include <nvs.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "WiFi";

// Event group for WiFi connection
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;
static bool s_is_connected = false;
static char s_current_ssid[WIFI_SSID_MAX_LEN] = {0};
static char s_ip_address[16] = {0};

// NVS keys for WiFi credentials
#define NVS_NAMESPACE   "wifi_config"
#define NVS_KEY_SSID    "ssid"
#define NVS_KEY_PASS    "password"

/**
 * Event handler for WiFi and IP events
 */
static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi station started, attempting to connect...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP (attempt %d/%d)", s_retry_num, WIFI_MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG, "Failed to connect to AP after %d attempts", WIFI_MAXIMUM_RETRY);
        }
        s_is_connected = false;
        memset(s_ip_address, 0, sizeof(s_ip_address));
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        snprintf(s_ip_address, sizeof(s_ip_address), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Got IP address: %s", s_ip_address);
        s_retry_num = 0;
        s_is_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * Load WiFi credentials from NVS
 */
static esp_err_t load_wifi_credentials(char* ssid, char* password)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No WiFi credentials found in NVS");
        return err;
    }
    
    size_t ssid_len = WIFI_SSID_MAX_LEN;
    err = nvs_get_str(handle, NVS_KEY_SSID, ssid, &ssid_len);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }
    
    size_t pass_len = WIFI_PASSWORD_MAX_LEN;
    err = nvs_get_str(handle, NVS_KEY_PASS, password, &pass_len);
    nvs_close(handle);
    
    return err;
}

/**
 * Save WiFi credentials to NVS
 */
static esp_err_t save_wifi_credentials(const char* ssid, const char* password)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_set_str(handle, NVS_KEY_SSID, ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save SSID: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }
    
    err = nvs_set_str(handle, NVS_KEY_PASS, password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save password: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }
    
    err = nvs_commit(handle);
    nvs_close(handle);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "WiFi credentials saved to NVS");
    }
    
    return err;
}

/**
 * Initialize WiFi in Station mode
 */
static esp_err_t init_wifi_sta(const char* ssid, const char* password)
{
    s_wifi_event_group = xEventGroupCreate();
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    strncpy(s_current_ssid, ssid, WIFI_SSID_MAX_LEN - 1);
    
    ESP_LOGI(TAG, "WiFi initialization finished. Connecting to SSID: %s", ssid);
    
    // Wait for connection
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to SSID: %s", ssid);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID: %s", ssid);
        return ESP_FAIL;
    }
    
    return ESP_ERR_TIMEOUT;
}

esp_err_t wifi_manager_init(void)
{
    char ssid[WIFI_SSID_MAX_LEN] = {0};
    char password[WIFI_PASSWORD_MAX_LEN] = {0};
    
    ESP_LOGI(TAG, "Initializing WiFi Manager...");
    
    // Try to load credentials from NVS
    esp_err_t err = load_wifi_credentials(ssid, password);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No saved WiFi credentials found. Use 'wifi set <ssid> <password>' to configure.");
        return ESP_OK; // Not an error, just not configured yet
    }
    
    // Initialize and connect
    return init_wifi_sta(ssid, password);
}

esp_err_t wifi_manager_set_credentials(const char* ssid, const char* password)
{
    if (ssid == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (strlen(ssid) == 0 || strlen(ssid) >= WIFI_SSID_MAX_LEN) {
        ESP_LOGE(TAG, "Invalid SSID length");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (strlen(password) < 8 || strlen(password) >= WIFI_PASSWORD_MAX_LEN) {
        ESP_LOGE(TAG, "Invalid password length (must be 8-63 characters)");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Save credentials
    esp_err_t err = save_wifi_credentials(ssid, password);
    if (err != ESP_OK) {
        return err;
    }
    
    // Disconnect if already connected
    if (s_is_connected) {
        ESP_LOGI(TAG, "Disconnecting from current network...");
        esp_wifi_disconnect();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Initialize or reconfigure WiFi
    s_retry_num = 0;
    return init_wifi_sta(ssid, password);
}

esp_err_t wifi_manager_disconnect(void)
{
    if (!s_is_connected) {
        ESP_LOGW(TAG, "WiFi is not connected");
        return ESP_ERR_WIFI_NOT_CONNECT;
    }
    
    esp_err_t err = esp_wifi_disconnect();
    if (err == ESP_OK) {
        s_is_connected = false;
        memset(s_ip_address, 0, sizeof(s_ip_address));
        ESP_LOGI(TAG, "Disconnected from WiFi");
    }
    
    return err;
}

bool wifi_manager_is_connected(void)
{
    return s_is_connected;
}

esp_err_t wifi_manager_get_ssid(char* ssid)
{
    if (ssid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!s_is_connected) {
        ssid[0] = '\0';
        return ESP_ERR_WIFI_NOT_CONNECT;
    }
    
    strncpy(ssid, s_current_ssid, WIFI_SSID_MAX_LEN - 1);
    ssid[WIFI_SSID_MAX_LEN - 1] = '\0';
    
    return ESP_OK;
}

esp_err_t wifi_manager_get_ip(char* ip_str)
{
    if (ip_str == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!s_is_connected) {
        strcpy(ip_str, "0.0.0.0");
        return ESP_ERR_WIFI_NOT_CONNECT;
    }
    
    strncpy(ip_str, s_ip_address, 16);
    ip_str[15] = '\0';
    
    return ESP_OK;
}
