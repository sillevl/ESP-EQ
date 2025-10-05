#include "subsonic.h"
#include <string.h>
#include <math.h>
#include <nvs.h>
#include "esp_log.h"

// NVS storage keys
#define NVS_NAMESPACE "subsonic_set"
#define NVS_KEY_ENABLED "enabled"
#define NVS_KEY_FREQUENCY "frequency"

static const char *TAG = "SUBSONIC";

/**
 * Calculate 2nd-order high-pass Butterworth filter coefficients
 * This creates a high-pass filter at the specified cutoff frequency
 * with Q = 0.707 (Butterworth characteristic)
 * Coefficients are converted to Q24 fixed-point format
 */
static void calculate_highpass_filter(subsonic_biquad_coeffs_t *coeffs, float freq, 
                                      float sample_rate, float Q)
{
    float w0 = 2.0f * M_PI * freq / sample_rate;  // Normalized frequency
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.0f * Q);
    
    // High-pass filter coefficients (floating-point)
    float a0 = 1.0f + alpha;
    float b0_f = ((1.0f + cos_w0) / 2.0f) / a0;
    float b1_f = (-(1.0f + cos_w0)) / a0;
    float b2_f = ((1.0f + cos_w0) / 2.0f) / a0;
    float a1_f = (-2.0f * cos_w0) / a0;
    float a2_f = (1.0f - alpha) / a0;
    
    // Convert to Q24 fixed-point (multiply by 2^24)
    coeffs->b0 = (int32_t)(b0_f * 16777216.0f);
    coeffs->b1 = (int32_t)(b1_f * 16777216.0f);
    coeffs->b2 = (int32_t)(b2_f * 16777216.0f);
    coeffs->a1 = (int32_t)(a1_f * 16777216.0f);
    coeffs->a2 = (int32_t)(a2_f * 16777216.0f);
    
    ESP_LOGD(TAG, "Highpass filter calculated for %.1f Hz:", freq);
    ESP_LOGD(TAG, "  b0=%.6f, b1=%.6f, b2=%.6f", b0_f, b1_f, b2_f);
    ESP_LOGD(TAG, "  a1=%.6f, a2=%.6f", a1_f, a2_f);
}

void subsonic_init(subsonic_t *subsonic, uint32_t sample_rate)
{
    memset(subsonic, 0, sizeof(subsonic_t));
    
    // Set default cutoff frequency
    subsonic->cutoff_freq = SUBSONIC_FREQ_HZ;
    
    // Calculate filter coefficients
    calculate_highpass_filter(&subsonic->coeffs, SUBSONIC_FREQ_HZ, 
                             (float)sample_rate, SUBSONIC_Q);
    
    subsonic->enabled = true;
    
    ESP_LOGI(TAG, "Subsonic filter initialized:");
    ESP_LOGI(TAG, "  Type: 2nd-order high-pass Butterworth");
    ESP_LOGI(TAG, "  Cutoff frequency: %.1f Hz", SUBSONIC_FREQ_HZ);
    ESP_LOGI(TAG, "  Q factor: %.3f", SUBSONIC_Q);
    ESP_LOGI(TAG, "  Sample rate: %lu Hz", sample_rate);
}

bool subsonic_set_frequency(subsonic_t *subsonic, float freq, uint32_t sample_rate)
{
    // Validate frequency range (should be low, typically 20-35 Hz)
    if (freq < 15.0f || freq > 50.0f) {
        ESP_LOGW(TAG, "Frequency %.1f Hz out of recommended range (15-50 Hz)", freq);
        return false;
    }
    
    subsonic->cutoff_freq = freq;
    
    // Recalculate filter coefficients
    calculate_highpass_filter(&subsonic->coeffs, freq, (float)sample_rate, SUBSONIC_Q);
    
    // Reset filter state to avoid transients
    subsonic_reset(subsonic);
    
    ESP_LOGI(TAG, "Cutoff frequency set to %.1f Hz", freq);
    return true;
}

void subsonic_process(subsonic_t *subsonic, int32_t *buffer, int num_samples)
{
    if (!subsonic->enabled) {
        return;  // Bypass
    }
    
    subsonic_biquad_coeffs_t *c = &subsonic->coeffs;
    subsonic_biquad_state_t *state_l = &subsonic->state_left;
    subsonic_biquad_state_t *state_r = &subsonic->state_right;
    
    // Process stereo interleaved samples
    for (int i = 0; i < num_samples; i += 2) {
        // Left channel
        int32_t input_l = buffer[i];
        
        // Biquad direct form II (transposed)
        // Coefficients are Q24, shift right by 24 after multiplication
        int64_t temp_l = ((int64_t)c->b0 * input_l) >> 24;
        temp_l += ((int64_t)c->b1 * state_l->x1) >> 24;
        temp_l += ((int64_t)c->b2 * state_l->x2) >> 24;
        temp_l -= ((int64_t)c->a1 * state_l->y1) >> 24;
        temp_l -= ((int64_t)c->a2 * state_l->y2) >> 24;
        
        int32_t output_l = (int32_t)temp_l;
        
        // Update state
        state_l->x2 = state_l->x1;
        state_l->x1 = input_l;
        state_l->y2 = state_l->y1;
        state_l->y1 = output_l;
        
        buffer[i] = output_l;
        
        // Right channel
        int32_t input_r = buffer[i + 1];
        
        int64_t temp_r = ((int64_t)c->b0 * input_r) >> 24;
        temp_r += ((int64_t)c->b1 * state_r->x1) >> 24;
        temp_r += ((int64_t)c->b2 * state_r->x2) >> 24;
        temp_r -= ((int64_t)c->a1 * state_r->y1) >> 24;
        temp_r -= ((int64_t)c->a2 * state_r->y2) >> 24;
        
        int32_t output_r = (int32_t)temp_r;
        
        // Update state
        state_r->x2 = state_r->x1;
        state_r->x1 = input_r;
        state_r->y2 = state_r->y1;
        state_r->y1 = output_r;
        
        buffer[i + 1] = output_r;
    }
}

void subsonic_set_enabled(subsonic_t *subsonic, bool enable)
{
    subsonic->enabled = enable;
    ESP_LOGI(TAG, "Subsonic filter %s", enable ? "enabled" : "disabled");
}

bool subsonic_get_enabled(subsonic_t *subsonic)
{
    return subsonic->enabled;
}

float subsonic_get_frequency(subsonic_t *subsonic)
{
    return subsonic->cutoff_freq;
}

void subsonic_reset(subsonic_t *subsonic)
{
    memset(&subsonic->state_left, 0, sizeof(subsonic_biquad_state_t));
    memset(&subsonic->state_right, 0, sizeof(subsonic_biquad_state_t));
    ESP_LOGD(TAG, "Filter state reset");
}

esp_err_t subsonic_load_settings(subsonic_t *subsonic, uint32_t sample_rate)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No saved settings found, using defaults");
        return err;
    }
    
    // Load enabled state
    uint8_t enabled;
    err = nvs_get_u8(nvs_handle, NVS_KEY_ENABLED, &enabled);
    if (err == ESP_OK) {
        subsonic->enabled = (enabled != 0);
    }
    
    // Load frequency
    float freq;
    size_t size = sizeof(freq);
    err = nvs_get_blob(nvs_handle, NVS_KEY_FREQUENCY, &freq, &size);
    if (err == ESP_OK) {
        subsonic_set_frequency(subsonic, freq, sample_rate);
    }
    
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "Settings loaded from NVS:");
    ESP_LOGI(TAG, "  Enabled: %s", subsonic->enabled ? "yes" : "no");
    ESP_LOGI(TAG, "  Frequency: %.1f Hz", subsonic->cutoff_freq);
    
    return ESP_OK;
}

esp_err_t subsonic_save_settings(subsonic_t *subsonic)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    // Save enabled state
    uint8_t enabled = subsonic->enabled ? 1 : 0;
    err = nvs_set_u8(nvs_handle, NVS_KEY_ENABLED, enabled);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save enabled state: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    
    // Save frequency
    err = nvs_set_blob(nvs_handle, NVS_KEY_FREQUENCY, &subsonic->cutoff_freq, 
                      sizeof(subsonic->cutoff_freq));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save frequency: %s", esp_err_to_name(err));
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
