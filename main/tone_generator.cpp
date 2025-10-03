#include "tone_generator.h"
#include <math.h>

#define TWO_PI 6.28318530718f

void tone_generator_init(tone_generator_t *gen, uint32_t sample_rate)
{
    gen->sample_rate = sample_rate;
    gen->frequency = 440.0f;
    gen->amplitude = 0.5f;
    gen->phase = 0.0f;
    gen->waveform = WAVE_SINE;
    gen->enabled = false;
}

void tone_generator_set_frequency(tone_generator_t *gen, float frequency)
{
    gen->frequency = frequency;
    // Reset phase to avoid discontinuities
    gen->phase = 0.0f;
}

void tone_generator_set_amplitude(tone_generator_t *gen, float amplitude)
{
    // Clamp amplitude between 0.0 and 1.0
    if (amplitude < 0.0f) amplitude = 0.0f;
    if (amplitude > 1.0f) amplitude = 1.0f;
    gen->amplitude = amplitude;
}

void tone_generator_set_waveform(tone_generator_t *gen, waveform_t waveform)
{
    gen->waveform = waveform;
    // Reset phase to avoid discontinuities
    gen->phase = 0.0f;
}

void tone_generator_set_enabled(tone_generator_t *gen, bool enabled)
{
    gen->enabled = enabled;
    if (enabled) {
        // Reset phase when enabling
        gen->phase = 0.0f;
    }
}

void tone_generator_generate(tone_generator_t *gen, int32_t *buffer, int num_samples)
{
    float phase_increment = TWO_PI * gen->frequency / gen->sample_rate;
    float scale = gen->amplitude * 0x7FFFFF; // Scale to 24-bit range
    
    for (int i = 0; i < num_samples; i += 2) {
        float sample_value = 0.0f;
        
        // Generate waveform based on type
        switch (gen->waveform) {
            case WAVE_SINE:
                sample_value = sinf(gen->phase);
                break;
                
            case WAVE_SQUARE:
                sample_value = (gen->phase < M_PI) ? 1.0f : -1.0f;
                break;
                
            case WAVE_TRIANGLE:
                if (gen->phase < M_PI) {
                    sample_value = -1.0f + (2.0f * gen->phase / M_PI);
                } else {
                    sample_value = 3.0f - (2.0f * gen->phase / M_PI);
                }
                break;
                
            case WAVE_SAWTOOTH:
                sample_value = -1.0f + (gen->phase / M_PI);
                break;
        }
        
        // Scale to 24-bit integer and apply to both channels (stereo)
        int32_t sample_int = (int32_t)(sample_value * scale);
        buffer[i] = sample_int;      // Left channel
        buffer[i + 1] = sample_int;  // Right channel
        
        // Increment phase and wrap around
        gen->phase += phase_increment;
        if (gen->phase >= TWO_PI) {
            gen->phase -= TWO_PI;
        }
    }
}
