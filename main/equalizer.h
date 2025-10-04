#ifndef EQUALIZER_H
#define EQUALIZER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// 5-band equalizer frequencies (Hz)
#define EQ_BAND_1_FREQ      60      // Sub-bass
#define EQ_BAND_2_FREQ      250     // Bass
#define EQ_BAND_3_FREQ      1000    // Mid
#define EQ_BAND_4_FREQ      4000    // Upper mid
#define EQ_BAND_5_FREQ      12000   // Treble

// Number of bands
#define EQ_BANDS            5

// Biquad filter coefficients structure (Q24 fixed-point format)
typedef struct {
    int32_t b0, b1, b2;  // Feedforward coefficients (Q24 fixed-point)
    int32_t a1, a2;      // Feedback coefficients (Q24 fixed-point, a0 is normalized to 1)
} biquad_coeffs_t;

// Biquad filter state (one per channel per band)
typedef struct {
    int32_t x1, x2;    // Previous input samples
    int32_t y1, y2;    // Previous output samples
} biquad_state_t;

// Equalizer structure
typedef struct {
    biquad_coeffs_t coeffs[EQ_BANDS];           // Filter coefficients for each band
    biquad_state_t state_left[EQ_BANDS];        // State for left channel
    biquad_state_t state_right[EQ_BANDS];       // State for right channel
    float gain_db[EQ_BANDS];                    // Gain in dB for each band (-12 to +12)
    float pre_gain_db;                          // Pre-gain in dB (-12 to +12) applied before EQ
    bool enabled;                                // Enable/disable equalizer
} equalizer_t;

/**
 * Initialize equalizer with default settings (all bands at 0dB)
 * 
 * @param eq Pointer to equalizer structure
 * @param sample_rate Sample rate in Hz
 */
void equalizer_init(equalizer_t *eq, uint32_t sample_rate);

/**
 * Set gain for a specific band
 * 
 * @param eq Pointer to equalizer structure
 * @param band Band index (0-4)
 * @param gain_db Gain in dB (-12.0 to +12.0)
 * @param sample_rate Sample rate in Hz
 * @return true if successful, false if parameters invalid
 */
bool equalizer_set_band_gain(equalizer_t *eq, int band, float gain_db, uint32_t sample_rate);

/**
 * Process audio through equalizer
 * 
 * @param eq Pointer to equalizer structure
 * @param buffer Audio buffer (interleaved stereo: L, R, L, R, ...)
 * @param num_samples Number of samples (total, not per channel)
 */
void equalizer_process(equalizer_t *eq, int32_t *buffer, int num_samples);

/**
 * Enable or disable equalizer
 * 
 * @param eq Pointer to equalizer structure
 * @param enabled true to enable, false to bypass
 */
void equalizer_set_enabled(equalizer_t *eq, bool enabled);

/**
 * Reset equalizer state (clear filter history)
 * 
 * @param eq Pointer to equalizer structure
 */
void equalizer_reset(equalizer_t *eq);

/**
 * Set pre-gain (applied before EQ processing)
 * 
 * @param eq Pointer to equalizer structure
 * @param gain_db Pre-gain in dB (-12.0 to +12.0)
 * @return true if successful, false if parameters invalid
 */
bool equalizer_set_pre_gain(equalizer_t *eq, float gain_db);

/**
 * Get current pre-gain value
 * 
 * @param eq Pointer to equalizer structure
 * @return Pre-gain in dB
 */
float equalizer_get_pre_gain(equalizer_t *eq);

/**
 * Save equalizer settings to NVS flash storage
 * 
 * @param eq Pointer to equalizer structure
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t equalizer_save_settings(equalizer_t *eq);

/**
 * Load equalizer settings from NVS flash storage
 * 
 * @param eq Pointer to equalizer structure
 * @param sample_rate Sample rate in Hz
 * @return ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if no saved settings, error code otherwise
 */
esp_err_t equalizer_load_settings(equalizer_t *eq, uint32_t sample_rate);

#endif // EQUALIZER_H
