#ifndef LIMITER_H
#define LIMITER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Limiter configuration
#define LIMITER_LOOKAHEAD_MS    5.0f       // Lookahead time in milliseconds
#define LIMITER_ATTACK_MS       0.5f       // Attack time in milliseconds
#define LIMITER_RELEASE_MS      50.0f      // Release time in milliseconds
#define LIMITER_THRESHOLD_DB    -0.5f      // Threshold in dB (slightly below 0dBFS)

// Maximum lookahead buffer size (calculated for 48kHz)
// 5ms at 48kHz stereo = 5 * 48 * 2 = 480 samples
#define MAX_LOOKAHEAD_SAMPLES   512

// Limiter structure
typedef struct {
    // Configuration
    float threshold;                        // Linear threshold (0.0 to 1.0)
    float threshold_db;                     // Threshold in dB
    float attack_coeff;                     // Attack coefficient for envelope follower
    float release_coeff;                    // Release coefficient for envelope follower
    int lookahead_samples;                  // Lookahead buffer size in samples
    
    // State
    float envelope;                         // Current gain reduction envelope
    int32_t lookahead_buffer[MAX_LOOKAHEAD_SAMPLES];  // Circular buffer for lookahead
    int write_index;                        // Write position in circular buffer
    
    // Statistics
    float peak_reduction_db;                // Maximum reduction applied (for monitoring)
    uint32_t clip_prevented_count;          // Number of clips prevented
    
    bool enabled;                           // Enable/disable limiter
} limiter_t;

/**
 * Initialize limiter with default settings
 * 
 * @param limiter Pointer to limiter structure
 * @param sample_rate Sample rate in Hz
 */
void limiter_init(limiter_t *limiter, uint32_t sample_rate);

/**
 * Process audio through limiter
 * 
 * @param limiter Pointer to limiter structure
 * @param buffer Audio buffer (interleaved stereo: L, R, L, R, ...)
 * @param num_samples Number of samples (total, not per channel)
 */
void limiter_process(limiter_t *limiter, int32_t *buffer, int num_samples);

/**
 * Enable or disable limiter
 * 
 * @param limiter Pointer to limiter structure
 * @param enabled true to enable, false to bypass
 */
void limiter_set_enabled(limiter_t *limiter, bool enabled);

/**
 * Set limiter threshold
 * 
 * @param limiter Pointer to limiter structure
 * @param threshold_db Threshold in dB (-12.0 to 0.0)
 * @return true if successful, false if parameters invalid
 */
bool limiter_set_threshold(limiter_t *limiter, float threshold_db);

/**
 * Get current threshold value
 * 
 * @param limiter Pointer to limiter structure
 * @return Threshold in dB
 */
float limiter_get_threshold(limiter_t *limiter);

/**
 * Reset limiter state
 * 
 * @param limiter Pointer to limiter structure
 */
void limiter_reset(limiter_t *limiter);

/**
 * Get peak reduction statistics
 * 
 * @param limiter Pointer to limiter structure
 * @return Peak reduction in dB (negative value)
 */
float limiter_get_peak_reduction(limiter_t *limiter);

/**
 * Get number of clips prevented
 * 
 * @param limiter Pointer to limiter structure
 * @return Number of clips prevented since last reset
 */
uint32_t limiter_get_clips_prevented(limiter_t *limiter);

/**
 * Reset statistics
 * 
 * @param limiter Pointer to limiter structure
 */
void limiter_reset_stats(limiter_t *limiter);

/**
 * Save limiter settings to NVS
 * 
 * @param limiter Pointer to limiter structure
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t limiter_save_settings(limiter_t *limiter);

/**
 * Load limiter settings from NVS
 * 
 * @param limiter Pointer to limiter structure
 * @param sample_rate Sample rate in Hz (needed for recalculation)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t limiter_load_settings(limiter_t *limiter, uint32_t sample_rate);

#endif // LIMITER_H
