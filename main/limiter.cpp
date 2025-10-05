#include "limiter.h"
#include <string.h>
#include <math.h>
#include <nvs.h>
#include "esp_log.h"

// NVS storage keys
#define NVS_NAMESPACE "limiter_set"
#define NVS_KEY_ENABLED "enabled"
#define NVS_KEY_THRESHOLD "threshold"

static const char *TAG = "LIMITER";

// Convert dB to linear
static inline float db_to_linear(float db) {
    return powf(10.0f, db / 20.0f);
}

// Convert linear to dB
static inline float linear_to_db(float linear) {
    return 20.0f * log10f(linear);
}

// Fast approximation of maximum absolute value
static inline float fast_abs_max(int32_t a, int32_t b) {
    float abs_a = fabsf((float)a);
    float abs_b = fabsf((float)b);
    return (abs_a > abs_b) ? abs_a : abs_b;
}

void limiter_init(limiter_t *limiter, uint32_t sample_rate)
{
    memset(limiter, 0, sizeof(limiter_t));
    
    // Set default threshold
    limiter->threshold_db = LIMITER_THRESHOLD_DB;
    limiter->threshold = db_to_linear(LIMITER_THRESHOLD_DB);
    
    // Calculate lookahead buffer size (in samples, stereo)
    // lookahead_ms * sample_rate * 2 channels / 1000
    limiter->lookahead_samples = (int)((LIMITER_LOOKAHEAD_MS * sample_rate * 2.0f) / 1000.0f);
    if (limiter->lookahead_samples > MAX_LOOKAHEAD_SAMPLES) {
        limiter->lookahead_samples = MAX_LOOKAHEAD_SAMPLES;
    }
    
    // Calculate attack coefficient
    // attack_coeff = exp(-1 / (attack_time_sec * sample_rate))
    float attack_time_sec = LIMITER_ATTACK_MS / 1000.0f;
    limiter->attack_coeff = expf(-1.0f / (attack_time_sec * sample_rate));
    
    // Calculate release coefficient
    float release_time_sec = LIMITER_RELEASE_MS / 1000.0f;
    limiter->release_coeff = expf(-1.0f / (release_time_sec * sample_rate));
    
    // Initialize envelope to 1.0 (no gain reduction)
    limiter->envelope = 1.0f;
    limiter->write_index = 0;
    
    limiter->enabled = true;
    limiter->peak_reduction_db = 0.0f;
    limiter->clip_prevented_count = 0;
    
    ESP_LOGI(TAG, "Limiter initialized:");
    ESP_LOGI(TAG, "  Threshold: %.1f dB", limiter->threshold_db);
    ESP_LOGI(TAG, "  Lookahead: %.1f ms (%d samples)", LIMITER_LOOKAHEAD_MS, limiter->lookahead_samples);
    ESP_LOGI(TAG, "  Attack: %.1f ms (coeff: %.6f)", LIMITER_ATTACK_MS, limiter->attack_coeff);
    ESP_LOGI(TAG, "  Release: %.1f ms (coeff: %.6f)", LIMITER_RELEASE_MS, limiter->release_coeff);
}

void limiter_process(limiter_t *limiter, int32_t *buffer, int num_samples)
{
    if (!limiter->enabled) {
        return;  // Bypass
    }
    
    // Maximum value for 24-bit audio (shifted to 32-bit, so full scale is 2^31)
    const float FULL_SCALE = 2147483648.0f;  // 2^31
    const float THRESHOLD_LINEAR = limiter->threshold * FULL_SCALE;
    
    // Process samples
    for (int i = 0; i < num_samples; i += 2) {
        // Read from lookahead buffer (this is our delayed output)
        int32_t delayed_left = limiter->lookahead_buffer[limiter->write_index];
        int32_t delayed_right = limiter->lookahead_buffer[limiter->write_index + 1];
        
        // Store current input in lookahead buffer
        limiter->lookahead_buffer[limiter->write_index] = buffer[i];
        limiter->lookahead_buffer[limiter->write_index + 1] = buffer[i + 1];
        
        // Advance write index (circular buffer)
        limiter->write_index += 2;
        if (limiter->write_index >= limiter->lookahead_samples) {
            limiter->write_index = 0;
        }
        
        // Detect peak of current input (before delay)
        float peak = fast_abs_max(buffer[i], buffer[i + 1]);
        
        // Calculate desired gain
        float desired_gain = 1.0f;
        if (peak > THRESHOLD_LINEAR) {
            desired_gain = THRESHOLD_LINEAR / peak;
            limiter->clip_prevented_count++;
        }
        
        // Smooth envelope follower
        if (desired_gain < limiter->envelope) {
            // Attack: Fast reduction
            limiter->envelope = limiter->attack_coeff * limiter->envelope + 
                               (1.0f - limiter->attack_coeff) * desired_gain;
        } else {
            // Release: Slow recovery
            limiter->envelope = limiter->release_coeff * limiter->envelope + 
                               (1.0f - limiter->release_coeff) * desired_gain;
        }
        
        // Track peak reduction for monitoring
        float reduction_db = linear_to_db(limiter->envelope);
        if (reduction_db < limiter->peak_reduction_db) {
            limiter->peak_reduction_db = reduction_db;
        }
        
        // Apply gain to delayed signal
        int64_t output_left = ((int64_t)delayed_left * (int64_t)(limiter->envelope * 65536.0f)) >> 16;
        int64_t output_right = ((int64_t)delayed_right * (int64_t)(limiter->envelope * 65536.0f)) >> 16;
        
        // Clamp to prevent overflow (safety)
        if (output_left > 2147483647LL) output_left = 2147483647LL;
        if (output_left < -2147483648LL) output_left = -2147483648LL;
        if (output_right > 2147483647LL) output_right = 2147483647LL;
        if (output_right < -2147483648LL) output_right = -2147483648LL;
        
        buffer[i] = (int32_t)output_left;
        buffer[i + 1] = (int32_t)output_right;
    }
}

void limiter_set_enabled(limiter_t *limiter, bool enabled)
{
    limiter->enabled = enabled;
    if (enabled) {
        ESP_LOGI(TAG, "Limiter enabled");
    } else {
        ESP_LOGI(TAG, "Limiter bypassed");
    }
}

bool limiter_set_threshold(limiter_t *limiter, float threshold_db)
{
    // Clamp threshold to reasonable range
    if (threshold_db < -12.0f) threshold_db = -12.0f;
    if (threshold_db > 0.0f) threshold_db = 0.0f;
    
    limiter->threshold_db = threshold_db;
    limiter->threshold = db_to_linear(threshold_db);
    
    ESP_LOGI(TAG, "Threshold set to %.1f dB (linear: %.4f)", threshold_db, limiter->threshold);
    return true;
}

float limiter_get_threshold(limiter_t *limiter)
{
    return limiter->threshold_db;
}

void limiter_reset(limiter_t *limiter)
{
    // Clear lookahead buffer
    memset(limiter->lookahead_buffer, 0, sizeof(limiter->lookahead_buffer));
    limiter->write_index = 0;
    limiter->envelope = 1.0f;
    
    ESP_LOGI(TAG, "Limiter state reset");
}

float limiter_get_peak_reduction(limiter_t *limiter)
{
    return limiter->peak_reduction_db;
}

uint32_t limiter_get_clips_prevented(limiter_t *limiter)
{
    return limiter->clip_prevented_count;
}

void limiter_reset_stats(limiter_t *limiter)
{
    limiter->peak_reduction_db = 0.0f;
    limiter->clip_prevented_count = 0;
    ESP_LOGI(TAG, "Statistics reset");
}

esp_err_t limiter_save_settings(limiter_t *limiter)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    // Save enabled state
    err = nvs_set_u8(nvs_handle, NVS_KEY_ENABLED, limiter->enabled ? 1 : 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save enabled state: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    
    // Save threshold
    err = nvs_set_i32(nvs_handle, NVS_KEY_THRESHOLD, (int32_t)(limiter->threshold_db * 100.0f));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save threshold: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    
    // Commit changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Settings saved to NVS");
    }
    
    nvs_close(nvs_handle);
    return err;
}

esp_err_t limiter_load_settings(limiter_t *limiter, uint32_t sample_rate)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS for reading: %s", esp_err_to_name(err));
        return err;
    }
    
    // Load enabled state
    uint8_t enabled = 1;
    err = nvs_get_u8(nvs_handle, NVS_KEY_ENABLED, &enabled);
    if (err == ESP_OK) {
        limiter->enabled = (enabled != 0);
    }
    
    // Load threshold
    int32_t threshold_int = 0;
    err = nvs_get_i32(nvs_handle, NVS_KEY_THRESHOLD, &threshold_int);
    if (err == ESP_OK) {
        float threshold_db = (float)threshold_int / 100.0f;
        limiter_set_threshold(limiter, threshold_db);
    }
    
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "Settings loaded from NVS");
    ESP_LOGI(TAG, "  Enabled: %s", limiter->enabled ? "yes" : "no");
    ESP_LOGI(TAG, "  Threshold: %.1f dB", limiter->threshold_db);
    
    return ESP_OK;
}
