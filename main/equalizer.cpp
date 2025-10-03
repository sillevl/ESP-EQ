#include "equalizer.h"
#include <string.h>
#include <math.h>

// Quality factor for peaking filters
#define Q_FACTOR 0.707f  // Butterworth response (wider bandwidth)

/**
 * Calculate biquad peaking filter coefficients
 * This creates a peak/dip at the specified frequency
 */
static void calculate_peaking_filter(biquad_coeffs_t *coeffs, float freq, float gain_db, 
                                     float sample_rate, float Q)
{
    float A = powf(10.0f, gain_db / 40.0f);  // Amplitude
    float w0 = 2.0f * M_PI * freq / sample_rate;  // Normalized frequency
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.0f * Q);
    
    // Peaking filter coefficients
    float a0 = 1.0f + alpha / A;
    coeffs->b0 = (1.0f + alpha * A) / a0;
    coeffs->b1 = (-2.0f * cos_w0) / a0;
    coeffs->b2 = (1.0f - alpha * A) / a0;
    coeffs->a1 = (-2.0f * cos_w0) / a0;
    coeffs->a2 = (1.0f - alpha / A) / a0;
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
            // Scale down to prevent overflow, process as fixed-point
            int64_t temp_l = ((int64_t)c->b0 * input_l) >> 8;
            temp_l += ((int64_t)c->b1 * state_l->x1) >> 8;
            temp_l += ((int64_t)c->b2 * state_l->x2) >> 8;
            temp_l -= ((int64_t)c->a1 * state_l->y1) >> 8;
            temp_l -= ((int64_t)c->a2 * state_l->y2) >> 8;
            
            int32_t output_l = (int32_t)temp_l;
            
            // Update state
            state_l->x2 = state_l->x1;
            state_l->x1 = input_l;
            state_l->y2 = state_l->y1;
            state_l->y1 = output_l;
            
            buffer[i] = output_l;
            
            // Right channel
            int32_t input_r = buffer[i + 1];
            
            int64_t temp_r = ((int64_t)c->b0 * input_r) >> 8;
            temp_r += ((int64_t)c->b1 * state_r->x1) >> 8;
            temp_r += ((int64_t)c->b2 * state_r->x2) >> 8;
            temp_r -= ((int64_t)c->a1 * state_r->y1) >> 8;
            temp_r -= ((int64_t)c->a2 * state_r->y2) >> 8;
            
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
