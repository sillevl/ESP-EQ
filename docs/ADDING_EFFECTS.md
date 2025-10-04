# Adding Audio Effects

This guide shows how to add audio processing effects to your ESP32 DSP.

## Basic Structure

All audio processing happens in the `audio_task()` function in `main/esp-dsp.cpp`:

```cpp
while (1) {
    // 1. Read audio from ADC
    i2s_channel_read(rx_handle, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);
    
    int num_samples = bytes_read / sizeof(int32_t);
    
    // 2. Shift samples for 24-bit processing
    for(int i = 0; i < num_samples; i++) {
        audio_buffer[i] = audio_buffer[i] >> 8;
    }
    
    // 3. Process through equalizer
    equalizer_process(&equalizer, audio_buffer, num_samples);
    
    // 4. ADD YOUR CUSTOM PROCESSING HERE
    // YOUR CODE GOES HERE
    
    // 5. Shift back for output
    for(int i = 0; i < num_samples; i++) {
        audio_buffer[i] = audio_buffer[i] << 8;
    }
    
    // 6. Write audio to DAC
    i2s_channel_write(tx_handle, audio_buffer, bytes_read, &bytes_written, portMAX_DELAY);
}
```

## Audio Buffer Format

- **Type**: `int32_t audio_buffer[DMA_BUFFER_SIZE]`
- **Format**: 24-bit signed integer (after >> 8 shift, before << 8 shift)
- **Channels**: Interleaved stereo (L, R, L, R, ...)
- **Range**: -8388608 to +8388607 (24-bit signed)
- **Sample layout**: 
  - `audio_buffer[0]` = Left channel sample 0
  - `audio_buffer[1]` = Right channel sample 0
  - `audio_buffer[2]` = Left channel sample 1
  - `audio_buffer[3]` = Right channel sample 1
  
**Important**: The bit shifting (>> 8 and << 8) converts between the hardware 32-bit format and 24-bit processing format. Add your custom effects AFTER the >> 8 shift and BEFORE the << 8 shift.

## Example Effects

### 1. Volume Control

```cpp
// Volume: 0-256 (256 = unity gain, 128 = -6dB, 512 = +6dB)
int volume = 256;

for (int i = 0; i < DMA_BUFFER_SIZE; i++) {
    audio_buffer[i] = (audio_buffer[i] * volume) >> 8;
}
```

### 2. Stereo Balance

```cpp
// Balance: -100 (full left) to +100 (full right), 0 = center
int balance = 0;
int left_gain = (balance < 0) ? 256 : (256 - (balance * 256) / 100);
int right_gain = (balance > 0) ? 256 : (256 + (balance * 256) / 100);

for (int i = 0; i < DMA_BUFFER_SIZE; i += 2) {
    audio_buffer[i] = (audio_buffer[i] * left_gain) >> 8;      // Left
    audio_buffer[i + 1] = (audio_buffer[i + 1] * right_gain) >> 8;  // Right
}
```

### 3. Mono Mix

```cpp
// Convert stereo to mono (same signal on both channels)
for (int i = 0; i < DMA_BUFFER_SIZE; i += 2) {
    int32_t mono = (audio_buffer[i] + audio_buffer[i + 1]) >> 1;
    audio_buffer[i] = mono;
    audio_buffer[i + 1] = mono;
}
```

### 4. Simple High-Pass Filter (DC Blocker)

```cpp
// Static variables to maintain state between calls
static int32_t prev_input_l = 0, prev_output_l = 0;
static int32_t prev_input_r = 0, prev_output_r = 0;

// Filter coefficient (0.995 gives ~20Hz cutoff at 48kHz)
const int32_t COEFF = 258;  // 0.995 * 256

for (int i = 0; i < DMA_BUFFER_SIZE; i += 2) {
    // Left channel
    int32_t output_l = audio_buffer[i] - prev_input_l + ((prev_output_l * COEFF) >> 8);
    prev_input_l = audio_buffer[i];
    prev_output_l = output_l;
    audio_buffer[i] = output_l;
    
    // Right channel
    int32_t output_r = audio_buffer[i + 1] - prev_input_r + ((prev_output_r * COEFF) >> 8);
    prev_input_r = audio_buffer[i + 1];
    prev_output_r = output_r;
    audio_buffer[i + 1] = output_r;
}
```

### 5. Simple Delay/Echo Effect

```cpp
#define DELAY_SAMPLES (SAMPLE_RATE / 4)  // 250ms delay
#define DELAY_BUFFER_SIZE (DELAY_SAMPLES * 2)  // Stereo

static int32_t delay_buffer[DELAY_BUFFER_SIZE] = {0};
static int delay_pos = 0;
int feedback = 128;  // 50% feedback (0-256)

for (int i = 0; i < DMA_BUFFER_SIZE; i++) {
    // Read from delay buffer
    int32_t delayed = delay_buffer[delay_pos];
    
    // Mix with input and write back to delay buffer
    delay_buffer[delay_pos] = audio_buffer[i] + ((delayed * feedback) >> 8);
    
    // Mix delayed signal with dry signal
    audio_buffer[i] = audio_buffer[i] + (delayed >> 1);  // 50% wet mix
    
    // Advance delay position
    delay_pos = (delay_pos + 1) % DELAY_BUFFER_SIZE;
}
```

### 6. Soft Clipping (Distortion)

```cpp
// Soft clipping for overdrive effect
#define CLIP_THRESHOLD (1 << 22)  // ~50% of full scale

for (int i = 0; i < DMA_BUFFER_SIZE; i++) {
    int32_t sample = audio_buffer[i];
    
    if (sample > CLIP_THRESHOLD) {
        sample = CLIP_THRESHOLD + ((sample - CLIP_THRESHOLD) >> 2);
    } else if (sample < -CLIP_THRESHOLD) {
        sample = -CLIP_THRESHOLD + ((sample + CLIP_THRESHOLD) >> 2);
    }
    
    audio_buffer[i] = sample;
}
```

### 7. Simple Tremolo Effect

```cpp
static float phase = 0.0f;
const float TREMOLO_RATE = 5.0f;  // Hz
const float TREMOLO_DEPTH = 0.3f;  // 0.0 to 1.0

for (int i = 0; i < DMA_BUFFER_SIZE; i++) {
    // Calculate modulation (sine wave oscillator)
    float mod = 1.0f - (TREMOLO_DEPTH * (1.0f + sinf(phase)) / 2.0f);
    
    // Apply to sample
    audio_buffer[i] = (int32_t)(audio_buffer[i] * mod);
    
    // Advance phase (every sample)
    phase += (2.0f * M_PI * TREMOLO_RATE) / SAMPLE_RATE;
    if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
}
```

## Using ESP-DSP Library

For more advanced DSP, consider using the ESP-DSP library:

1. Add to `main/CMakeLists.txt`:
```cmake
idf_component_register(SRCS "esp-dsp.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES esp-dsp)
```

2. Include headers:
```cpp
#include "esp_dsp.h"
```

3. Use optimized functions:
```cpp
// FFT, FIR filters, IIR filters, matrix operations, etc.
dsps_fft2r_fc32(fft_data, N);
```

## Performance Tips

1. **Avoid floating-point**: Use fixed-point math (integer operations) when possible
2. **Minimize divisions**: Use bit shifts instead (e.g., `>> 1` instead of `/ 2`)
3. **Use lookup tables**: For sin/cos/exp operations
4. **Profile your code**: Use `esp_timer_get_time()` to measure processing time
5. **Watch CPU usage**: Monitor with `vTaskGetRunTimeStats()`

## Monitoring Processing Time

Add this to measure DSP performance:

```cpp
#include "esp_timer.h"

// In your processing loop:
int64_t start_time = esp_timer_get_time();

// ... your processing code ...

int64_t end_time = esp_timer_get_time();
int64_t processing_time = end_time - start_time;

// Log every 1000 buffers
static int count = 0;
if (++count >= 1000) {
    ESP_LOGI(TAG, "Processing time: %lld us", processing_time);
    count = 0;
}
```

## Combining Multiple Effects

Create a processing chain:

```cpp
// Effect chain
void apply_volume(int32_t* buffer, int size, int volume);
void apply_highpass(int32_t* buffer, int size);
void apply_delay(int32_t* buffer, int size);

// In audio task:
i2s_channel_read(rx_handle, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);

apply_volume(audio_buffer, DMA_BUFFER_SIZE, 200);
apply_highpass(audio_buffer, DMA_BUFFER_SIZE);
apply_delay(audio_buffer, DMA_BUFFER_SIZE);

i2s_channel_write(tx_handle, audio_buffer, bytes_read, &bytes_written, portMAX_DELAY);
```

## Resources

- [ESP-DSP Library](https://github.com/espressif/esp-dsp)
- [Audio DSP Algorithms](https://github.com/MTG/sms-tools)
- [Fixed-Point Arithmetic](https://en.wikipedia.org/wiki/Fixed-point_arithmetic)
