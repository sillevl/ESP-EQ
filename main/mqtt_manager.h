#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * MQTT Manager Configuration
 */
#define MQTT_BROKER_MAX_LEN     128
#define MQTT_CLIENT_ID          "esp-dsp"
#define MQTT_BASE_TOPIC         "esp-dsp"

// MQTT Topics
#define MQTT_TOPIC_STATUS       MQTT_BASE_TOPIC"/status"
#define MQTT_TOPIC_COMMAND      MQTT_BASE_TOPIC"/command"

// Subsonic topics
#define MQTT_TOPIC_SUB_FREQ     MQTT_BASE_TOPIC"/subsonic/freq"
#define MQTT_TOPIC_SUB_ENABLE   MQTT_BASE_TOPIC"/subsonic/enable"
#define MQTT_TOPIC_SUB_STATE    MQTT_BASE_TOPIC"/subsonic/state"

// Pre-gain topics
#define MQTT_TOPIC_GAIN_SET     MQTT_BASE_TOPIC"/pregain/set"
#define MQTT_TOPIC_GAIN_ENABLE  MQTT_BASE_TOPIC"/pregain/enable"
#define MQTT_TOPIC_GAIN_STATE   MQTT_BASE_TOPIC"/pregain/state"

// Equalizer topics
#define MQTT_TOPIC_EQ_BAND      MQTT_BASE_TOPIC"/eq/band"
#define MQTT_TOPIC_EQ_ENABLE    MQTT_BASE_TOPIC"/eq/enable"
#define MQTT_TOPIC_EQ_PRESET    MQTT_BASE_TOPIC"/eq/preset"
#define MQTT_TOPIC_EQ_STATE     MQTT_BASE_TOPIC"/eq/state"

// Limiter topics
#define MQTT_TOPIC_LIM_THRESHOLD MQTT_BASE_TOPIC"/limiter/threshold"
#define MQTT_TOPIC_LIM_ENABLE    MQTT_BASE_TOPIC"/limiter/enable"
#define MQTT_TOPIC_LIM_STATE     MQTT_BASE_TOPIC"/limiter/state"

/**
 * Initialize MQTT Manager
 * Loads MQTT broker configuration from NVS and connects
 * 
 * @return ESP_OK on success
 */
esp_err_t mqtt_manager_init(void);

/**
 * Set MQTT broker and connect
 * 
 * @param broker_uri MQTT broker URI (e.g., "mqtt://192.168.1.100:1883")
 * @return ESP_OK on success
 */
esp_err_t mqtt_manager_set_broker(const char* broker_uri);

/**
 * Disconnect from MQTT broker
 * 
 * @return ESP_OK on success
 */
esp_err_t mqtt_manager_disconnect(void);

/**
 * Check if MQTT is connected
 * 
 * @return true if connected, false otherwise
 */
bool mqtt_manager_is_connected(void);

/**
 * Publish a message to a topic
 * 
 * @param topic MQTT topic
 * @param data Message payload
 * @param qos Quality of Service (0, 1, or 2)
 * @param retain Retain message flag
 * @return ESP_OK on success
 */
esp_err_t mqtt_manager_publish(const char* topic, const char* data, int qos, bool retain);

/**
 * Publish current system status
 * 
 * @return ESP_OK on success
 */
esp_err_t mqtt_manager_publish_status(void);

/**
 * Publish subsonic filter state
 * 
 * @return ESP_OK on success
 */
esp_err_t mqtt_manager_publish_subsonic_state(void);

/**
 * Publish pre-gain state
 * 
 * @return ESP_OK on success
 */
esp_err_t mqtt_manager_publish_pregain_state(void);

/**
 * Publish equalizer state
 * 
 * @return ESP_OK on success
 */
esp_err_t mqtt_manager_publish_eq_state(void);

/**
 * Publish limiter state
 * 
 * @return ESP_OK on success
 */
esp_err_t mqtt_manager_publish_limiter_state(void);

/**
 * Publish all states
 * 
 * @return ESP_OK on success
 */
esp_err_t mqtt_manager_publish_all_states(void);

#endif // MQTT_MANAGER_H
