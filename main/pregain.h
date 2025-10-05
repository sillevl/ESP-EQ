#ifndef PREGAIN_H
#define PREGAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Pre-gain configuration
#define PREGAIN_MIN_DB          -12.0f     // Minimum pre-gain in dB
#define PREGAIN_MAX_DB          12.0f      // Maximum pre-gain in dB
#define PREGAIN_DEFAULT_DB      0.0f       // Default pre-gain in dB (unity gain)

// Pre-gain structure
typedef struct {
    float gain_db;                          // Gain in dB (-12.0 to +12.0)
    float gain_linear;                      // Linear gain multiplier (calculated from gain_db)
    bool enabled;                           // Enable/disable pre-gain
} pregain_t;

/**
 * Initialize pre-gain with default settings (0dB = unity gain)
 * 
 * @param pregain Pointer to pre-gain structure
 */
void pregain_init(pregain_t *pregain);

/**
 * Set pre-gain value
 * 
 * @param pregain Pointer to pre-gain structure
 * @param gain_db Gain in dB (-12.0 to +12.0)
 * @return true if successful, false if parameters invalid
 */
bool pregain_set_gain(pregain_t *pregain, float gain_db);

/**
 * Get current pre-gain value
 * 
 * @param pregain Pointer to pre-gain structure
 * @return Gain in dB
 */
float pregain_get_gain(pregain_t *pregain);

/**
 * Process audio through pre-gain
 * 
 * @param pregain Pointer to pre-gain structure
 * @param buffer Audio buffer (interleaved stereo: L, R, L, R, ...)
 * @param num_samples Number of samples (total, not per channel)
 */
void pregain_process(pregain_t *pregain, int32_t *buffer, int num_samples);

/**
 * Enable or disable pre-gain
 * 
 * @param pregain Pointer to pre-gain structure
 * @param enabled true to enable, false to bypass
 */
void pregain_set_enabled(pregain_t *pregain, bool enabled);

/**
 * Check if pre-gain is enabled
 * 
 * @param pregain Pointer to pre-gain structure
 * @return true if enabled, false otherwise
 */
bool pregain_is_enabled(pregain_t *pregain);

/**
 * Save pre-gain settings to NVS flash storage
 * 
 * @param pregain Pointer to pre-gain structure
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t pregain_save_settings(pregain_t *pregain);

/**
 * Load pre-gain settings from NVS flash storage
 * 
 * @param pregain Pointer to pre-gain structure
 * @return ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if no saved settings, error code otherwise
 */
esp_err_t pregain_load_settings(pregain_t *pregain);

#endif // PREGAIN_H
