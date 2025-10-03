# 5-Band Equalizer

The ESP32 DSP now includes a professional 5-band parametric equalizer using biquad IIR filters.

## Features

- **5 frequency bands**: 60Hz, 250Hz, 1kHz, 4kHz, 12kHz
- **Adjustable gain**: -12dB to +12dB per band
- **High-quality filters**: Biquad peaking filters with Butterworth response
- **Low CPU usage**: Optimized fixed-point implementation
- **Real-time processing**: Cascade filter architecture

## Frequency Bands

| Band | Frequency | Range        | Description |
|------|-----------|--------------|-------------|
| 1    | 60 Hz     | Sub-bass     | Deep bass, kick drums, bass guitar fundamentals |
| 2    | 250 Hz    | Bass         | Bass warmth, lower mids |
| 3    | 1 kHz     | Midrange     | Vocal presence, guitar body |
| 4    | 4 kHz     | Upper Mid    | Vocal clarity, attack of instruments |
| 5    | 12 kHz    | Treble       | Air, brilliance, cymbal shimmer |

## Usage

### Default Configuration

The equalizer is initialized with a default curve in `app_main()`:

```cpp
// Initialize with sample rate
equalizer_init(&equalizer, SAMPLE_RATE);

// Set gains for each band
equalizer_set_band_gain(&equalizer, 0, 3.0f, SAMPLE_RATE);   // 60Hz: +3dB
equalizer_set_band_gain(&equalizer, 1, 2.0f, SAMPLE_RATE);   // 250Hz: +2dB
equalizer_set_band_gain(&equalizer, 2, 0.0f, SAMPLE_RATE);   // 1kHz: 0dB
equalizer_set_band_gain(&equalizer, 3, 1.0f, SAMPLE_RATE);   // 4kHz: +1dB
equalizer_set_band_gain(&equalizer, 4, 4.0f, SAMPLE_RATE);   // 12kHz: +4dB
```

### Customizing EQ Settings

Edit the gains in `main/esp-dsp.cpp` in the `app_main()` function:

```cpp
// Example: V-shaped EQ (boost bass and treble, cut mids)
equalizer_set_band_gain(&equalizer, 0, 6.0f, SAMPLE_RATE);    // 60Hz: +6dB
equalizer_set_band_gain(&equalizer, 1, 4.0f, SAMPLE_RATE);    // 250Hz: +4dB
equalizer_set_band_gain(&equalizer, 2, -3.0f, SAMPLE_RATE);   // 1kHz: -3dB
equalizer_set_band_gain(&equalizer, 3, 0.0f, SAMPLE_RATE);    // 4kHz: 0dB
equalizer_set_band_gain(&equalizer, 4, 5.0f, SAMPLE_RATE);    // 12kHz: +5dB
```

### Enable/Disable Equalizer

```cpp
// Disable equalizer (bypass)
equalizer_set_enabled(&equalizer, false);

// Re-enable equalizer
equalizer_set_enabled(&equalizer, true);
```

### Reset Filter State

If you hear artifacts or clicks, reset the filter state:

```cpp
equalizer_reset(&equalizer);
```

## EQ Presets

### Flat (Reference)
All bands at 0dB - transparent, no coloration

```cpp
for (int i = 0; i < 5; i++) {
    equalizer_set_band_gain(&equalizer, i, 0.0f, SAMPLE_RATE);
}
```

### Bass Boost
Enhanced low-end for electronic music

```cpp
equalizer_set_band_gain(&equalizer, 0, 6.0f, SAMPLE_RATE);    // 60Hz: +6dB
equalizer_set_band_gain(&equalizer, 1, 4.0f, SAMPLE_RATE);    // 250Hz: +4dB
equalizer_set_band_gain(&equalizer, 2, 0.0f, SAMPLE_RATE);    // 1kHz: 0dB
equalizer_set_band_gain(&equalizer, 3, 0.0f, SAMPLE_RATE);    // 4kHz: 0dB
equalizer_set_band_gain(&equalizer, 4, 0.0f, SAMPLE_RATE);    // 12kHz: 0dB
```

### Vocal Clarity
Enhanced midrange and highs for speech

```cpp
equalizer_set_band_gain(&equalizer, 0, -2.0f, SAMPLE_RATE);   // 60Hz: -2dB
equalizer_set_band_gain(&equalizer, 1, 0.0f, SAMPLE_RATE);    // 250Hz: 0dB
equalizer_set_band_gain(&equalizer, 2, 3.0f, SAMPLE_RATE);    // 1kHz: +3dB
equalizer_set_band_gain(&equalizer, 3, 5.0f, SAMPLE_RATE);    // 4kHz: +5dB
equalizer_set_band_gain(&equalizer, 4, 2.0f, SAMPLE_RATE);    // 12kHz: +2dB
```

### Rock
Scooped mids with strong bass and treble

```cpp
equalizer_set_band_gain(&equalizer, 0, 5.0f, SAMPLE_RATE);    // 60Hz: +5dB
equalizer_set_band_gain(&equalizer, 1, 3.0f, SAMPLE_RATE);    // 250Hz: +3dB
equalizer_set_band_gain(&equalizer, 2, -4.0f, SAMPLE_RATE);   // 1kHz: -4dB
equalizer_set_band_gain(&equalizer, 3, 2.0f, SAMPLE_RATE);    // 4kHz: +2dB
equalizer_set_band_gain(&equalizer, 4, 6.0f, SAMPLE_RATE);    // 12kHz: +6dB
```

### Jazz
Natural, balanced with slight warmth

```cpp
equalizer_set_band_gain(&equalizer, 0, 2.0f, SAMPLE_RATE);    // 60Hz: +2dB
equalizer_set_band_gain(&equalizer, 1, 1.0f, SAMPLE_RATE);    // 250Hz: +1dB
equalizer_set_band_gain(&equalizer, 2, 0.0f, SAMPLE_RATE);    // 1kHz: 0dB
equalizer_set_band_gain(&equalizer, 3, 1.0f, SAMPLE_RATE);    // 4kHz: +1dB
equalizer_set_band_gain(&equalizer, 4, 3.0f, SAMPLE_RATE);    // 12kHz: +3dB
```

## Technical Details

### Filter Type
**Biquad Peaking Filter** (also called parametric EQ)

- Creates a bell-shaped boost or cut at specified frequency
- Quality factor (Q) = 0.707 (Butterworth response for smooth curves)
- Each band is independent and can be adjusted separately

### Implementation
- **Fixed-point arithmetic**: Uses 32-bit integers for efficiency
- **Cascade architecture**: Audio passes through all 5 filters sequentially
- **Stereo processing**: Separate filter states for left and right channels
- **Direct Form II Transposed**: Optimal structure for reduced quantization noise

### CPU Usage
- Approximately 5-10% additional CPU load (depends on sample rate)
- At 48kHz: ~1-2ms processing time per buffer
- Optimized for real-time processing with minimal latency

### Frequency Response
Each band affects a range around its center frequency:

```
60Hz band:   ~30Hz to 120Hz
250Hz band:  ~125Hz to 500Hz
1kHz band:   ~500Hz to 2kHz
4kHz band:   ~2kHz to 8kHz
12kHz band:  ~6kHz to 20kHz+
```

## Advanced Customization

### Changing Band Frequencies

Edit `main/equalizer.h`:

```cpp
#define EQ_BAND_1_FREQ      80      // Was 60
#define EQ_BAND_2_FREQ      300     // Was 250
#define EQ_BAND_3_FREQ      1200    // Was 1000
#define EQ_BAND_4_FREQ      5000    // Was 4000
#define EQ_BAND_5_FREQ      15000   // Was 12000
```

After changing, rebuild and reflash.

### Changing Quality Factor

Edit `main/equalizer.cpp`:

```cpp
#define Q_FACTOR 1.0f   // Narrower bandwidth (was 0.707)
// or
#define Q_FACTOR 0.5f   // Wider bandwidth
```

- **Higher Q**: Narrower, more surgical cuts/boosts
- **Lower Q**: Wider, more gentle curves

### Adding More Bands

1. Increase `EQ_BANDS` in `equalizer.h`
2. Add new frequency definitions
3. Update initialization in `equalizer_init()`
4. Rebuild project

## Performance Tips

1. **Disable when not needed**: If you want flat response, disable rather than setting all to 0dB
2. **Moderate gains**: Keep gains between -6dB and +6dB for best results
3. **Watch for clipping**: High boosts may cause distortion, reduce input level if needed
4. **Test incrementally**: Adjust one band at a time to hear its effect

## Troubleshooting

### Distortion
- Reduce EQ gains (too much boost causes clipping)
- Reduce input signal level
- Check for overflow in processing

### No effect heard
- Verify `equalizer.enabled = true`
- Check that `equalizer_process()` is being called
- Confirm gains are not all at 0dB

### Clicks/pops
- Call `equalizer_reset()` after changing settings
- Ensure smooth gain transitions

### High CPU usage
- Reduce sample rate if possible
- Simplify EQ curve (fewer boosted bands)
- Consider reducing number of bands

## API Reference

### Functions

```cpp
void equalizer_init(equalizer_t *eq, uint32_t sample_rate);
bool equalizer_set_band_gain(equalizer_t *eq, int band, float gain_db, uint32_t sample_rate);
void equalizer_process(equalizer_t *eq, int32_t *buffer, int num_samples);
void equalizer_set_enabled(equalizer_t *eq, bool enabled);
void equalizer_reset(equalizer_t *eq);
```

See `main/equalizer.h` for detailed documentation.

## Future Enhancements

Possible additions:
- [ ] Adjustable Q-factor per band
- [ ] More bands (7 or 10-band EQ)
- [ ] Different filter types (shelf, high-pass, low-pass)
- [ ] Preset system with storage
- [ ] Web interface for real-time adjustment
- [ ] Spectrum analyzer display
- [ ] Auto-EQ based on room response

## References

- [Audio EQ Cookbook](https://webaudio.github.io/Audio-EQ-Cookbook/) by Robert Bristow-Johnson
- [Digital Audio Signal Processing](https://en.wikipedia.org/wiki/Audio_signal_processing)
- [Biquad Filter](https://en.wikipedia.org/wiki/Digital_biquad_filter)
