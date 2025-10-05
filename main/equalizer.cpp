#include "equalizer.h"
#include <string.h>
#include <math.h>
#include <nvs.h>
#include "esp_log.h"

// Quality factor for peaking filters
#define Q_FACTOR 0.707f  // Butterworth response (wider bandwidth)

// NVS storage keys
#define NVS_NAMESPACE "eq_settings"
#define NVS_KEY_ENABLED "enabled"
#define NVS_KEY_BAND_PREFIX "band_"

static const char *TAG = "EQUALIZER";

/**
 * Calculate biquad peaking filter coefficients
 * This creates a peak/dip at the specified frequency
 * Coefficients are converted to Q24 fixed-point format
 */
static void calculate_peaking_filter(biquad_coeffs_t *coeffs, float freq, float gain_db, 
                                     float sample_rate, float Q)
{
    float A = powf(10.0f, gain_db / 40.0f);  // Amplitude
    float w0 = 2.0f * M_PI * freq / sample_rate;  // Normalized frequency
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.0f * Q);
    
    // Peaking filter coefficients (floating-point)
    float a0 = 1.0f + alpha / A;
    float b0_f = (1.0f + alpha * A) / a0;
    float b1_f = (-2.0f * cos_w0) / a0;
    float b2_f = (1.0f - alpha * A) / a0;
    float a1_f = (-2.0f * cos_w0) / a0;
    float a2_f = (1.0f - alpha / A) / a0;
    
    // Convert to Q24 fixed-point (multiply by 2^24)
    coeffs->b0 = (int32_t)(b0_f * 16777216.0f);
    coeffs->b1 = (int32_t)(b1_f * 16777216.0f);
    coeffs->b2 = (int32_t)(b2_f * 16777216.0f);
    coeffs->a1 = (int32_t)(a1_f * 16777216.0f);
    coeffs->a2 = (int32_t)(a2_f * 16777216.0f);
}

void equalizer_init(equalizer_t *eq, uint32_t sample_rate)
{
    memset(eq, 0, sizeof(equalizer_t));
    
    // Set default gains to 0dB (no change)
    for (int i = 0; i < EQ_BANDS; i++) {
        eq->gain_db[i] = 0.0f;
    }
    
    // Calculate initial filter coefficients (all at 0dB = unity)
    const float frequencies[] = {EQ_BAND_1_FREQ, EQ_BAND_2_FREQ, EQ_BAND_3_FREQ, 
                                  EQ_BAND_4_FREQ, EQ_BAND_5_FREQ};
    
    for (int i = 0; i < EQ_BANDS; i++) {
        calculate_peaking_filter(&eq->coeffs[i], frequencies[i], 0.0f, 
                                (float)sample_rate, Q_FACTOR);
    }
    
    eq->enabled = true;
}

bool equalizer_set_band_gain(equalizer_t *eq, int band, float gain_db, uint32_t sample_rate)
{
    if (band < 0 || band >= EQ_BANDS) {
        return false;
    }
    
    // Clamp gain to reasonable range
    if (gain_db < -12.0f) gain_db = -12.0f;
    if (gain_db > 12.0f) gain_db = 12.0f;
    
    eq->gain_db[band] = gain_db;
    
    // Recalculate filter coefficients for this band
    const float frequencies[] = {EQ_BAND_1_FREQ, EQ_BAND_2_FREQ, EQ_BAND_3_FREQ, 
                                  EQ_BAND_4_FREQ, EQ_BAND_5_FREQ};
    
    calculate_peaking_filter(&eq->coeffs[band], frequencies[band], gain_db, 
                            (float)sample_rate, Q_FACTOR);
    
    return true;
}

void equalizer_process(equalizer_t *eq, int32_t *buffer, int num_samples)
{
    if (!eq->enabled) {
        return;  // Bypass
    }

    // Quick optimization: if all band gains are exactly 0.0f then the filters
    // are unity and we can skip processing entirely. This avoids costly
    // multipass processing when equalizer is enabled but set to flat.
    bool all_zero = true;
    for (int b = 0; b < EQ_BANDS; ++b) {
        if (eq->gain_db[b] != 0.0f) { all_zero = false; break; }
    }
    if (all_zero) return;
    
    // Process each band sequentially (cascade)
    for (int band = 0; band < EQ_BANDS; band++) {
        biquad_coeffs_t *c = &eq->coeffs[band];
        biquad_state_t *state_l = &eq->state_left[band];
        biquad_state_t *state_r = &eq->state_right[band];
        
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
}

void equalizer_set_enabled(equalizer_t *eq, bool enabled)
{
    eq->enabled = enabled;
}

void equalizer_reset(equalizer_t *eq)
{
    // Clear all filter state but keep coefficients
    for (int i = 0; i < EQ_BANDS; i++) {
        memset(&eq->state_left[i], 0, sizeof(biquad_state_t));
        memset(&eq->state_right[i], 0, sizeof(biquad_state_t));
    }
}

esp_err_t equalizer_save_settings(equalizer_t *eq)
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
    err = nvs_set_u8(nvs_handle, NVS_KEY_ENABLED, eq->enabled ? 1 : 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving enabled state: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    
    // Save band gains (convert float to int32 to store in NVS)
    for (int i = 0; i < EQ_BANDS; i++) {
        char key[16];
        snprintf(key, sizeof(key), "%s%d", NVS_KEY_BAND_PREFIX, i);
        
        // Store gain as fixed-point (multiply by 100 to preserve 2 decimal places)
        int32_t gain_fixed = (int32_t)(eq->gain_db[i] * 100.0f);
        err = nvs_set_i32(nvs_handle, key, gain_fixed);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error saving band %d gain: %s", i, esp_err_to_name(err));
            nvs_close(nvs_handle);
            return err;
        }
    }
    
    // Commit changes to flash
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing to NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Equalizer settings saved to flash");
    }
    
    nvs_close(nvs_handle);
    return err;
}

esp_err_t equalizer_load_settings(equalizer_t *eq, uint32_t sample_rate)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "No saved equalizer settings found, using defaults");
        } else {
            ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        }
        return err;
    }
    
    // Load enabled state
    uint8_t enabled_u8 = 1;
    err = nvs_get_u8(nvs_handle, NVS_KEY_ENABLED, &enabled_u8);
    if (err == ESP_OK) {
        eq->enabled = (enabled_u8 != 0);
    }
    
    bool settings_loaded = false;
    
    // Load band gains
    for (int i = 0; i < EQ_BANDS; i++) {
        char key[16];
        snprintf(key, sizeof(key), "%s%d", NVS_KEY_BAND_PREFIX, i);
        
        int32_t gain_fixed = 0;
        err = nvs_get_i32(nvs_handle, key, &gain_fixed);
        if (err == ESP_OK) {
            // Convert from fixed-point back to float
            float gain_db = (float)gain_fixed / 100.0f;
            equalizer_set_band_gain(eq, i, gain_db, sample_rate);
            settings_loaded = true;
        }
    }
    
    nvs_close(nvs_handle);
    
    if (settings_loaded) {
        ESP_LOGI(TAG, "Equalizer settings loaded from flash:");
        ESP_LOGI(TAG, "  Status: %s", eq->enabled ? "ENABLED" : "DISABLED");
        ESP_LOGI(TAG, "  60Hz:   %.1f dB", eq->gain_db[0]);
        ESP_LOGI(TAG, "  250Hz:  %.1f dB", eq->gain_db[1]);
        ESP_LOGI(TAG, "  1kHz:   %.1f dB", eq->gain_db[2]);
        ESP_LOGI(TAG, "  4kHz:   %.1f dB", eq->gain_db[3]);
        ESP_LOGI(TAG, "  12kHz:  %.1f dB", eq->gain_db[4]);
        return ESP_OK;
    }
    
    return ESP_ERR_NVS_NOT_FOUND;
}
