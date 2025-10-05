#include "pregain.h"
#include <string.h>
#include <math.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

// NVS storage keys
#define NVS_NAMESPACE "pregain_settings"
#define NVS_KEY_ENABLED "enabled"
#define NVS_KEY_GAIN "gain"

static const char *TAG = "PREGAIN";

void pregain_init(pregain_t *pregain)
{
    memset(pregain, 0, sizeof(pregain_t));
    
    // Set default gain to 0dB (unity gain)
    pregain->gain_db = PREGAIN_DEFAULT_DB;
    pregain->gain_linear = 1.0f;  // 0dB = 10^(0/20) = 1.0
    pregain->enabled = true;
}

bool pregain_set_gain(pregain_t *pregain, float gain_db)
{
    // Clamp gain to reasonable range
    if (gain_db < PREGAIN_MIN_DB) gain_db = PREGAIN_MIN_DB;
    if (gain_db > PREGAIN_MAX_DB) gain_db = PREGAIN_MAX_DB;
    
    pregain->gain_db = gain_db;
    
    // Convert dB to linear gain: linear = 10^(dB/20)
    pregain->gain_linear = powf(10.0f, gain_db / 20.0f);
    
    return true;
}

float pregain_get_gain(pregain_t *pregain)
{
    return pregain->gain_db;
}

void pregain_process(pregain_t *pregain, int32_t *buffer, int num_samples)
{
    if (!pregain->enabled) {
        return;  // Bypass
    }
    
    // If gain is 0dB (unity gain), skip processing
    if (pregain->gain_db == 0.0f) {
        return;
    }
    
    // Apply gain to all samples
    for (int i = 0; i < num_samples; i++) {
        int64_t temp = (int64_t)(buffer[i] * pregain->gain_linear);
        
        // Clamp to prevent overflow
        if (temp > 2147483647LL) temp = 2147483647LL;
        if (temp < -2147483648LL) temp = -2147483648LL;
        
        buffer[i] = (int32_t)temp;
    }
}

void pregain_set_enabled(pregain_t *pregain, bool enabled)
{
    pregain->enabled = enabled;
}

bool pregain_is_enabled(pregain_t *pregain)
{
    return pregain->enabled;
}

esp_err_t pregain_save_settings(pregain_t *pregain)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    // Save enabled state
    err = nvs_set_u8(nvs_handle, NVS_KEY_ENABLED, pregain->enabled ? 1 : 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving enabled state: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    
    // Save gain (convert float to int32 to store in NVS)
    int32_t gain_fixed = (int32_t)(pregain->gain_db * 100.0f);
    err = nvs_set_i32(nvs_handle, NVS_KEY_GAIN, gain_fixed);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving gain: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    
    // Commit changes to flash
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing to NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Pre-gain settings saved to flash");
    }
    
    nvs_close(nvs_handle);
    return err;
}

esp_err_t pregain_load_settings(pregain_t *pregain)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No saved pre-gain settings found, using defaults");
        } else {
            ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        }
        return err;
    }
    
    // Load enabled state
    uint8_t enabled_u8 = 1;
    err = nvs_get_u8(nvs_handle, NVS_KEY_ENABLED, &enabled_u8);
    if (err == ESP_OK) {
        pregain->enabled = (enabled_u8 != 0);
    }
    
    bool settings_loaded = false;
    
    // Load gain
    int32_t gain_fixed = 0;
    err = nvs_get_i32(nvs_handle, NVS_KEY_GAIN, &gain_fixed);
    if (err == ESP_OK) {
        float gain_db = (float)gain_fixed / 100.0f;
        pregain_set_gain(pregain, gain_db);
        settings_loaded = true;
    }
    
    nvs_close(nvs_handle);
    
    if (settings_loaded) {
        ESP_LOGI(TAG, "Pre-gain settings loaded from flash:");
        ESP_LOGI(TAG, "  Status: %s", pregain->enabled ? "ENABLED" : "DISABLED");
        ESP_LOGI(TAG, "  Gain: %.1f dB (%.3fx)", pregain->gain_db, pregain->gain_linear);
        return ESP_OK;
    }
    
    return ESP_ERR_NVS_NOT_FOUND;
}
