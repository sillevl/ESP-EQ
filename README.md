# ESP32 DSP Audio Processor

A real-time audio processing platform for ESP32 using high-quality audio codecs with a professional 5-band parametric equalizer.

## Hardware

- **DAC**: [PCM5102A](https://www.ti.com/lit/gpn/pcm5102a) - 32-bit 384kHz Stereo DAC (Master Clock Generator)
- **ADC**: [WM8782](https://www.mouser.com/datasheet/2/76/cirr_s_a0009699137_1-2263098.pdf) - 24-bit 192kHz Stereo ADC
- **MCU**: ESP32-C3 or any ESP32 variant with I2S support

## Features

- ✅ **Shared I2S Clock Domain** - Single I2S peripheral for both input and output
- ✅ **Master Clock (MCLK)** - 18.432MHz for jitter-free audio (384x sample rate)
- ✅ 24-bit audio at 48kHz sample rate (configurable)
- ✅ Low-latency audio pass-through (~80ms)
- ✅ **5-Band Parametric Equalizer** (60Hz, 250Hz, 1kHz, 4kHz, 12kHz)
- ✅ **Real-time Serial Command Interface** for EQ control
- ✅ **Persistent Settings** - EQ settings saved to NVS flash
- ✅ Built-in EQ presets (Flat, Bass, Vocal, Rock, Jazz)
- ✅ FreeRTOS-based real-time processing
- ✅ Optimized fixed-point biquad IIR filters (Direct Form II Transposed)
- ✅ Modular architecture for easy DSP algorithm integration

## Quick Start

1. **Hardware Setup**: Wire ESP32 to WM8782 and PCM5102A with shared clocks
   - See [Hardware Setup Guide](docs/HARDWARE_SETUP.md)
   - **Important**: MCLK connects to WM8782 for synchronization

2. **Build & Flash**:
   ```powershell
   idf.py -p COM3 build flash monitor
   ```
   - See [Build Instructions](docs/BUILD_INSTRUCTIONS.md) for details

3. **Connect Audio**: 
   - Input: Connect audio source to WM8782
   - Output: Connect headphones/speakers to PCM5102A

4. **Test**: You should hear input audio with EQ processing applied
   - Default: Bass boost preset active
   - Latency: ~80ms

5. **Control via Serial**: Use serial commands to adjust EQ in real-time
   - See [Serial Commands Guide](docs/SERIAL_COMMANDS.md)

## Project Structure

```
esp-dsp/
├── main/
│   ├── esp-dsp.cpp           # Main application and audio processing task
│   ├── audio_config.h        # Audio configuration and pin definitions
│   ├── equalizer.cpp         # 5-band parametric equalizer implementation
│   ├── equalizer.h           # Equalizer API and structures
│   ├── serial_commands.cpp   # Serial command interface
│   ├── serial_commands.h     # Command interface API
│   ├── CMakeLists.txt        # Component build configuration
│   └── Kconfig.projbuild     # menuconfig options
├── docs/
│   ├── HARDWARE_SETUP.md     # Wiring and hardware guide
│   ├── BUILD_INSTRUCTIONS.md # Detailed build instructions
│   ├── EQUALIZER.md          # Equalizer documentation and presets
│   ├── SERIAL_COMMANDS.md    # Serial command reference
│   ├── PERSISTENT_SETTINGS.md # NVS flash storage documentation
│   ├── ADDING_EFFECTS.md     # Guide for adding custom DSP effects
│   ├── TROUBLESHOOTING.md    # Common issues and solutions
│   └── PROJECT_OVERVIEW.md   # Architecture and technical details
├── CMakeLists.txt            # Top-level CMake configuration
├── sdkconfig.defaults        # Default ESP-IDF configuration
├── QUICK_START.md            # Quick start guide
└── README.md
```

## Configuration

### Pin Assignments

Edit `main/audio_config.h` to match your hardware:

```c
// Shared I2S pins (both ADC and DAC use same clock domain)
#define I2S_MCLK        GPIO_NUM_10  // Master clock (18.432MHz @ 48kHz)
#define I2S_DAC_BCLK    GPIO_NUM_5   // Bit clock (shared)
#define I2S_DAC_WS      GPIO_NUM_6   // Word select (shared)

// DAC (PCM5102A) - Data output
#define I2S_DAC_DOUT    GPIO_NUM_7   // Audio data to DAC

// ADC (WM8782) - Data input
#define I2S_ADC_DIN     GPIO_NUM_4   // Audio data from ADC
```

**Note**: Both ADC and DAC share BCLK, WS, and MCLK for perfect synchronization.

### Sample Rate

Change sample rate in `main/audio_config.h`:
```c
#define SAMPLE_RATE     48000  // Recommended: 48000 Hz
```

### Buffer Size

Adjust for latency/stability trade-off:
```c
#define DMA_BUFFER_SIZE  480     // Samples per buffer
#define DMA_BUFFER_COUNT 8       // Number of DMA buffers
```

## Using the Equalizer

The system includes a professional 5-band parametric equalizer with **persistent settings** saved to flash memory.

### Real-Time Control via Serial Commands

Control the equalizer in real-time without recompiling:

```
> eq show                  # Display current settings
> eq set 0 6.0             # Set 60Hz band to +6dB
> eq preset bass           # Load bass boost preset
> eq enable                # Enable equalizer
> eq disable               # Bypass equalizer
```

See [Serial Commands Guide](docs/SERIAL_COMMANDS.md) for complete command reference.

### Settings Persistence

All EQ settings are **automatically saved to NVS flash** and restored on every boot:
- No need to manually save
- Settings survive power cycles
- See [Persistent Settings Guide](docs/PERSISTENT_SETTINGS.md)

### Built-in Presets

Five professionally tuned presets available:
- **flat** - Neutral reference (all bands at 0dB)
- **bass** - Enhanced low-end for electronic music
- **vocal** - Optimized for speech clarity
- **rock** - V-shaped curve (boosted bass and treble)
- **jazz** - Natural with slight warmth

Load a preset:
```
> eq preset bass
```

See [Equalizer Guide](docs/EQUALIZER.md) for detailed preset specifications.

## Technical Specifications

- **Sample Rate**: 48 kHz (configurable)
- **Bit Depth**: 24-bit audio processing
- **Latency**: ~80ms (configurable via buffer size)
- **Equalizer Bands**: 5 (60Hz, 250Hz, 1kHz, 4kHz, 12kHz)
- **Gain Range**: -12dB to +12dB per band
- **Filter Type**: Biquad peaking (Q=0.707 Butterworth)
- **Implementation**: Fixed-point Direct Form II Transposed
- **Clock Architecture**: Shared I2S clock domain with MCLK (18.432MHz)

## Adding Custom Audio Effects

To add additional DSP effects beyond the equalizer:

1. Open `main/esp-dsp.cpp`
2. Find the `audio_task()` function
3. Add your DSP code after the equalizer processing:

```cpp
// Read from ADC
i2s_channel_read(rx_handle, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);

// Shift samples for 24-bit processing
for(int i = 0; i < num_samples; i++) {
    audio_buffer[i] = audio_buffer[i] >> 8;
}

// Process through equalizer
equalizer_process(&equalizer, audio_buffer, num_samples);

// Add your custom processing here:
// Example: Simple volume control
for (int i = 0; i < num_samples; i++) {
    audio_buffer[i] = (audio_buffer[i] * volume) >> 8;
}

// Shift back for output
for(int i = 0; i < num_samples; i++) {
    audio_buffer[i] = audio_buffer[i] << 8;
}

// Write to DAC
i2s_channel_write(tx_handle, audio_buffer, bytes_read, &bytes_written, portMAX_DELAY);
```

See [Adding Effects Guide](docs/ADDING_EFFECTS.md) for more examples (delay, reverb, compression, etc.)

## Performance

- **Latency**: ~80ms at 48kHz with 480 sample buffers (8 buffers)
- **CPU Usage**: ~5-10% with equalizer enabled
- **Sample Rate**: Optimized for 48kHz (hardware supports up to 192kHz)
- **Memory**: ~156KB free heap during operation

## Resources

- [ESP-IDF I2S Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
- [PCM5102A Datasheet](https://www.ti.com/lit/gpn/pcm5102a)
- [WM8782 Datasheet](https://www.cirrus.com/products/wm8782/)

## License

This project is provided as-is for educational and development purposes.

## Contributing

Feel free to open issues or submit pull requests for improvements!
