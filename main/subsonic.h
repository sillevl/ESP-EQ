#ifndef SUBSONIC_H
#define SUBSONIC_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Subsonic filter configuration
#define SUBSONIC_FREQ_HZ        25.0f      // Cutoff frequency (25-30 Hz range)
#define SUBSONIC_Q              0.707f     // Q factor for Butterworth (0.707)

// Biquad filter coefficients structure (Q24 fixed-point format)
typedef struct {
    int32_t b0, b1, b2;  // Feedforward coefficients (Q24 fixed-point)
    int32_t a1, a2;      // Feedback coefficients (Q24 fixed-point, a0 is normalized to 1)
} subsonic_biquad_coeffs_t;

// Biquad filter state (one per channel)
typedef struct {
    int32_t x1, x2;    // Previous input samples
    int32_t y1, y2;    // Previous output samples
} subsonic_biquad_state_t;

// Subsonic filter structure
typedef struct {
    subsonic_biquad_coeffs_t coeffs;           // Filter coefficients
    subsonic_biquad_state_t state_left;        // State for left channel
    subsonic_biquad_state_t state_right;       // State for right channel
    float cutoff_freq;                         // Cutoff frequency in Hz
    bool enabled;                               // Enable/disable subsonic filter
} subsonic_t;

/**
 * Initialize subsonic filter with default settings
 * 
 * @param subsonic Pointer to subsonic structure
 * @param sample_rate Sample rate in Hz
 */
void subsonic_init(subsonic_t *subsonic, uint32_t sample_rate);

/**
 * Set cutoff frequency for the subsonic filter
 * 
 * @param subsonic Pointer to subsonic structure
 * @param freq Cutoff frequency in Hz (recommended: 25-30 Hz)
 * @param sample_rate Sample rate in Hz
 * @return true if successful, false if parameters invalid
 */
bool subsonic_set_frequency(subsonic_t *subsonic, float freq, uint32_t sample_rate);

/**
 * Process audio through subsonic filter
 * 
 * @param subsonic Pointer to subsonic structure
 * @param buffer Stereo interleaved audio buffer (int32_t samples)
 * @param num_samples Total number of samples (including both channels)
 */
void subsonic_process(subsonic_t *subsonic, int32_t *buffer, int num_samples);

/**
 * Enable or disable subsonic filter
 * 
 * @param subsonic Pointer to subsonic structure
 * @param enable true to enable, false to disable
 */
void subsonic_set_enabled(subsonic_t *subsonic, bool enable);

/**
 * Get enabled state of subsonic filter
 * 
 * @param subsonic Pointer to subsonic structure
 * @return true if enabled, false if disabled
 */
bool subsonic_get_enabled(subsonic_t *subsonic);

/**
 * Get current cutoff frequency
 * 
 * @param subsonic Pointer to subsonic structure
 * @return Cutoff frequency in Hz
 */
float subsonic_get_frequency(subsonic_t *subsonic);

/**
 * Reset filter state (clears history)
 * 
 * @param subsonic Pointer to subsonic structure
 */
void subsonic_reset(subsonic_t *subsonic);

/**
 * Load subsonic settings from NVS
 * 
 * @param subsonic Pointer to subsonic structure
 * @param sample_rate Sample rate in Hz
 * @return ESP_OK on success
 */
esp_err_t subsonic_load_settings(subsonic_t *subsonic, uint32_t sample_rate);

/**
 * Save subsonic settings to NVS
 * 
 * @param subsonic Pointer to subsonic structure
 * @return ESP_OK on success
 */
esp_err_t subsonic_save_settings(subsonic_t *subsonic);

#endif // SUBSONIC_H
