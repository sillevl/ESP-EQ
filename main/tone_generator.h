#ifndef TONE_GENERATOR_H
#define TONE_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>

// Waveform types
typedef enum {
    WAVE_SINE,
    WAVE_SQUARE,
    WAVE_TRIANGLE,
    WAVE_SAWTOOTH
} waveform_t;

// Tone generator structure
typedef struct {
    uint32_t sample_rate;  // Sample rate in Hz
    float frequency;        // Frequency in Hz
    float amplitude;        // Amplitude (0.0 to 1.0)
    float phase;           // Current phase (0.0 to 2*PI)
    waveform_t waveform;   // Waveform type
    bool enabled;          // Enable/disable tone generation
} tone_generator_t;

/**
 * Initialize tone generator
 * 
 * @param gen Pointer to tone generator structure
 * @param sample_rate Sample rate in Hz
 */
void tone_generator_init(tone_generator_t *gen, uint32_t sample_rate);

/**
 * Set tone frequency
 * 
 * @param gen Pointer to tone generator structure
 * @param frequency Frequency in Hz (20 to 20000)
 */
void tone_generator_set_frequency(tone_generator_t *gen, float frequency);

/**
 * Set tone amplitude
 * 
 * @param gen Pointer to tone generator structure
 * @param amplitude Amplitude (0.0 to 1.0, where 1.0 is full scale)
 */
void tone_generator_set_amplitude(tone_generator_t *gen, float amplitude);

/**
 * Set waveform type
 * 
 * @param gen Pointer to tone generator structure
 * @param waveform Waveform type
 */
void tone_generator_set_waveform(tone_generator_t *gen, waveform_t waveform);

/**
 * Enable or disable tone generator
 * 
 * @param gen Pointer to tone generator structure
 * @param enabled true to enable, false to disable
 */
void tone_generator_set_enabled(tone_generator_t *gen, bool enabled);

/**
 * Generate tone samples
 * Replaces buffer contents with generated tone
 * 
 * @param gen Pointer to tone generator structure
 * @param buffer Output buffer (interleaved stereo: L, R, L, R, ...)
 * @param num_samples Number of samples (total, not per channel)
 */
void tone_generator_generate(tone_generator_t *gen, int32_t *buffer, int num_samples);

#endif // TONE_GENERATOR_H
