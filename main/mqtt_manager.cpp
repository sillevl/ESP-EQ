#include "mqtt_manager.h"
#include "wifi_manager.h"
#include "subsonic.h"
#include "pregain.h"
#include "equalizer.h"
#include "limiter.h"
#include "audio_config.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include <nvs.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "MQTT";

// External references to DSP processors
extern subsonic_t subsonic;
extern pregain_t pregain;
extern equalizer_t equalizer;
extern limiter_t limiter;

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_is_connected = false;
static char s_broker_uri[MQTT_BROKER_MAX_LEN] = {0};

// NVS keys for MQTT configuration
#define NVS_NAMESPACE   "mqtt_config"
#define NVS_KEY_BROKER  "broker_uri"

/**
 * Load MQTT broker from NVS
 */
static esp_err_t load_mqtt_broker(char* broker_uri)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No MQTT broker found in NVS");
        return err;
    }
    
    size_t len = MQTT_BROKER_MAX_LEN;
    err = nvs_get_str(handle, NVS_KEY_BROKER, broker_uri, &len);
    nvs_close(handle);
    
    return err;
}

/**
 * Save MQTT broker to NVS
 */
static esp_err_t save_mqtt_broker(const char* broker_uri)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    err = nvs_set_str(handle, NVS_KEY_BROKER, broker_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save broker URI: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }
    
    err = nvs_commit(handle);
    nvs_close(handle);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "MQTT broker saved to NVS");
    }
    
    return err;
}

/**
 * Process MQTT command messages
 */
static void process_mqtt_command(const char* topic, const char* data, int data_len)
{
    char value[128] = {0};
    if (data_len > 0 && data_len < sizeof(value)) {
        strncpy(value, data, data_len);
        value[data_len] = '\0';
    }
    
    ESP_LOGI(TAG, "Command: topic=%s, value=%s", topic, value);
    
    // Subsonic filter commands
    if (strcmp(topic, MQTT_TOPIC_SUB_FREQ) == 0) {
        float freq = atof(value);
        if (subsonic_set_frequency(&subsonic, freq, SAMPLE_RATE)) {
            subsonic_save_settings(&subsonic);
            mqtt_manager_publish_subsonic_state();
            ESP_LOGI(TAG, "Subsonic frequency set to %.1f Hz", freq);
        }
    }
    else if (strcmp(topic, MQTT_TOPIC_SUB_ENABLE) == 0) {
        bool enable = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        subsonic_set_enabled(&subsonic, enable);
        subsonic_save_settings(&subsonic);
        mqtt_manager_publish_subsonic_state();
        ESP_LOGI(TAG, "Subsonic filter %s", enable ? "enabled" : "disabled");
    }
    
    // Pre-gain commands
    else if (strcmp(topic, MQTT_TOPIC_GAIN_SET) == 0) {
        float gain = atof(value);
        if (pregain_set_gain(&pregain, gain)) {
            pregain_save_settings(&pregain);
            mqtt_manager_publish_pregain_state();
            ESP_LOGI(TAG, "Pre-gain set to %.1f dB", gain);
        }
    }
    else if (strcmp(topic, MQTT_TOPIC_GAIN_ENABLE) == 0) {
        bool enable = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        pregain_set_enabled(&pregain, enable);
        pregain_save_settings(&pregain);
        mqtt_manager_publish_pregain_state();
        ESP_LOGI(TAG, "Pre-gain %s", enable ? "enabled" : "disabled");
    }
    
    // Equalizer commands
    else if (strncmp(topic, MQTT_TOPIC_EQ_BAND, strlen(MQTT_TOPIC_EQ_BAND)) == 0) {
        // Topic format: esp-dsp/eq/band/<band_num>
        const char* band_str = topic + strlen(MQTT_TOPIC_EQ_BAND);
        if (*band_str == '/') {
            int band = atoi(band_str + 1);
            float gain = atof(value);
            if (band >= 0 && band < 5) {
                if (equalizer_set_band_gain(&equalizer, band, gain, SAMPLE_RATE)) {
                    equalizer_save_settings(&equalizer);
                    mqtt_manager_publish_eq_state();
                    ESP_LOGI(TAG, "EQ band %d set to %.1f dB", band, gain);
                }
            }
        }
    }
    else if (strcmp(topic, MQTT_TOPIC_EQ_ENABLE) == 0) {
        bool enable = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        equalizer_set_enabled(&equalizer, enable);
        equalizer_save_settings(&equalizer);
        mqtt_manager_publish_eq_state();
        ESP_LOGI(TAG, "Equalizer %s", enable ? "enabled" : "disabled");
    }
    else if (strcmp(topic, MQTT_TOPIC_EQ_PRESET) == 0) {
        // Apply preset
        if (strcmp(value, "flat") == 0) {
            for (int i = 0; i < 5; i++) {
                equalizer_set_band_gain(&equalizer, i, 0.0f, SAMPLE_RATE);
            }
        }
        else if (strcmp(value, "bass") == 0) {
            equalizer_set_band_gain(&equalizer, 0, 6.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 1, 4.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 2, 0.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 3, 0.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 4, 0.0f, SAMPLE_RATE);
        }
        else if (strcmp(value, "vocal") == 0) {
            equalizer_set_band_gain(&equalizer, 0, -2.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 1, 0.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 2, 4.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 3, 3.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 4, -1.0f, SAMPLE_RATE);
        }
        else if (strcmp(value, "rock") == 0) {
            equalizer_set_band_gain(&equalizer, 0, 5.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 1, 3.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 2, -2.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 3, 2.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 4, 4.0f, SAMPLE_RATE);
        }
        else if (strcmp(value, "jazz") == 0) {
            equalizer_set_band_gain(&equalizer, 0, 3.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 1, 2.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 2, 0.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 3, 2.0f, SAMPLE_RATE);
            equalizer_set_band_gain(&equalizer, 4, 3.0f, SAMPLE_RATE);
        }
        equalizer_save_settings(&equalizer);
        mqtt_manager_publish_eq_state();
        ESP_LOGI(TAG, "EQ preset '%s' applied", value);
    }
    
    // Limiter commands
    else if (strcmp(topic, MQTT_TOPIC_LIM_THRESHOLD) == 0) {
        float threshold = atof(value);
        if (limiter_set_threshold(&limiter, threshold)) {
            limiter_save_settings(&limiter);
            mqtt_manager_publish_limiter_state();
            ESP_LOGI(TAG, "Limiter threshold set to %.1f dB", threshold);
        }
    }
    else if (strcmp(topic, MQTT_TOPIC_LIM_ENABLE) == 0) {
        bool enable = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        limiter_set_enabled(&limiter, enable);
        limiter_save_settings(&limiter);
        mqtt_manager_publish_limiter_state();
        ESP_LOGI(TAG, "Limiter %s", enable ? "enabled" : "disabled");
    }
}

/**
 * MQTT event handler
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to MQTT broker");
            s_is_connected = true;
            
            // Subscribe to all command topics
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_SUB_FREQ, 1);
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_SUB_ENABLE, 1);
            
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_GAIN_SET, 1);
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_GAIN_ENABLE, 1);
            
            // Subscribe to individual band topics (0-4)
            for (int i = 0; i < 5; i++) {
                char topic[64];
                snprintf(topic, sizeof(topic), "%s/%d", MQTT_TOPIC_EQ_BAND, i);
                esp_mqtt_client_subscribe(s_mqtt_client, topic, 1);
            }
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_EQ_ENABLE, 1);
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_EQ_PRESET, 1);
            
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_LIM_THRESHOLD, 1);
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_LIM_ENABLE, 1);
            
            // Publish initial states
            mqtt_manager_publish_all_states();
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected from MQTT broker");
            s_is_connected = false;
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Subscribed to topic, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_DATA: {
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            char topic[128] = {0};
            if (event->topic_len < sizeof(topic)) {
                strncpy(topic, event->topic, event->topic_len);
                topic[event->topic_len] = '\0';
                process_mqtt_command(topic, event->data, event->data_len);
            }
            break;
        }
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;

        case MQTT_EVENT_ANY:
        case MQTT_EVENT_UNSUBSCRIBED:
        case MQTT_EVENT_PUBLISHED:
        case MQTT_EVENT_BEFORE_CONNECT:
        case MQTT_EVENT_DELETED:
        case MQTT_USER_EVENT:
        default:
            break;
    }
}

/**
 * Start MQTT client
 */
static esp_err_t start_mqtt_client(const char* broker_uri)
{
    if (s_mqtt_client != NULL) {
        ESP_LOGI(TAG, "Stopping existing MQTT client...");
        esp_mqtt_client_stop(s_mqtt_client);
        esp_mqtt_client_destroy(s_mqtt_client);
        s_mqtt_client = NULL;
    }
    
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = broker_uri;
    mqtt_cfg.session.keepalive = 60;
    mqtt_cfg.network.timeout_ms = 5000;
    mqtt_cfg.credentials.client_id = MQTT_CLIENT_ID;
    
    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }
    
    esp_mqtt_client_register_event(s_mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    
    esp_err_t err = esp_mqtt_client_start(s_mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
        return err;
    }
    
    strncpy(s_broker_uri, broker_uri, MQTT_BROKER_MAX_LEN - 1);
    ESP_LOGI(TAG, "MQTT client started, connecting to: %s", broker_uri);
    
    return ESP_OK;
}

esp_err_t mqtt_manager_init(void)
{
    char broker_uri[MQTT_BROKER_MAX_LEN] = {0};
    
    ESP_LOGI(TAG, "Initializing MQTT Manager...");
    
    // Check if WiFi is connected
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected. MQTT will start when WiFi is available.");
        return ESP_OK;
    }
    
    // Try to load broker from NVS
    esp_err_t err = load_mqtt_broker(broker_uri);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No saved MQTT broker found. Use 'mqtt set <broker>' to configure.");
        return ESP_OK; // Not an error, just not configured yet
    }
    
    // Start MQTT client
    return start_mqtt_client(broker_uri);
}

esp_err_t mqtt_manager_set_broker(const char* broker_uri)
{
    if (broker_uri == NULL || strlen(broker_uri) == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check if WiFi is connected
    if (!wifi_manager_is_connected()) {
        ESP_LOGE(TAG, "WiFi not connected. Connect to WiFi first.");
        return ESP_FAIL;
    }
    
    // Save broker URI
    esp_err_t err = save_mqtt_broker(broker_uri);
    if (err != ESP_OK) {
        return err;
    }
    
    // Start or restart MQTT client
    return start_mqtt_client(broker_uri);
}

esp_err_t mqtt_manager_disconnect(void)
{
    if (s_mqtt_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t err = esp_mqtt_client_stop(s_mqtt_client);
    if (err == ESP_OK) {
        s_is_connected = false;
        ESP_LOGI(TAG, "Disconnected from MQTT broker");
    }
    
    return err;
}

bool mqtt_manager_is_connected(void)
{
    return s_is_connected;
}

esp_err_t mqtt_manager_publish(const char* topic, const char* data, int qos, bool retain)
{
    if (!s_is_connected || s_mqtt_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic, data, 0, qos, retain ? 1 : 0);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish to topic: %s", topic);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t mqtt_manager_publish_status(void)
{
    char status[512];
    snprintf(status, sizeof(status),
             "{\"sample_rate\":%d,\"channels\":%d,\"subsonic\":%s,\"pregain\":%s,\"eq\":%s,\"limiter\":%s}",
             SAMPLE_RATE,
             I2S_NUM_CHANNELS,
             subsonic_get_enabled(&subsonic) ? "true" : "false",
             pregain_is_enabled(&pregain) ? "true" : "false",
             equalizer.enabled ? "true" : "false",
             limiter.enabled ? "true" : "false");
    
    return mqtt_manager_publish(MQTT_TOPIC_STATUS, status, 0, true);
}

esp_err_t mqtt_manager_publish_subsonic_state(void)
{
    char state[128];
    snprintf(state, sizeof(state),
             "{\"enabled\":%s,\"freq\":%.1f}",
             subsonic_get_enabled(&subsonic) ? "true" : "false",
             subsonic_get_frequency(&subsonic));
    
    return mqtt_manager_publish(MQTT_TOPIC_SUB_STATE, state, 0, true);
}

esp_err_t mqtt_manager_publish_pregain_state(void)
{
    char state[128];
    snprintf(state, sizeof(state),
             "{\"enabled\":%s,\"gain\":%.1f}",
             pregain_is_enabled(&pregain) ? "true" : "false",
             pregain_get_gain(&pregain));
    
    return mqtt_manager_publish(MQTT_TOPIC_GAIN_STATE, state, 0, true);
}

esp_err_t mqtt_manager_publish_eq_state(void)
{
    char state[256];
    snprintf(state, sizeof(state),
             "{\"enabled\":%s,\"bands\":[%.1f,%.1f,%.1f,%.1f,%.1f]}",
             equalizer.enabled ? "true" : "false",
             equalizer.gain_db[0],
             equalizer.gain_db[1],
             equalizer.gain_db[2],
             equalizer.gain_db[3],
             equalizer.gain_db[4]);
    
    return mqtt_manager_publish(MQTT_TOPIC_EQ_STATE, state, 0, true);
}

esp_err_t mqtt_manager_publish_limiter_state(void)
{
    char state[128];
    snprintf(state, sizeof(state),
             "{\"enabled\":%s,\"threshold\":%.1f}",
             limiter.enabled ? "true" : "false",
             limiter_get_threshold(&limiter));
    
    return mqtt_manager_publish(MQTT_TOPIC_LIM_STATE, state, 0, true);
}

esp_err_t mqtt_manager_publish_all_states(void)
{
    mqtt_manager_publish_status();
    mqtt_manager_publish_subsonic_state();
    mqtt_manager_publish_pregain_state();
    mqtt_manager_publish_eq_state();
    mqtt_manager_publish_limiter_state();
    
    return ESP_OK;
}
