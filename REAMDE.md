# ESP32 DSP Audio Processor

A real-time audio processing platform for ESP32 using high-quality audio codecs.

## Hardware

- **DAC**: [PCM5102A](https://www.ti.com/lit/gpn/pcm5102a) - 32-bit 384kHz Stereo DAC
- **ADC**: [WM8782](https://www.mouser.com/datasheet/2/76/cirr_s_a0009699137_1-2263098.pdf) - 24-bit 192kHz Stereo ADC
- **MCU**: ESP32 (any variant with I2S support)

## Features

- ✅ Dual I2S interface (simultaneous input and output)
- ✅ 24-bit audio at 48kHz sample rate (configurable up to 192kHz)
- ✅ Low-latency audio pass-through
- ✅ FreeRTOS-based real-time processing
- ✅ Modular architecture for easy DSP algorithm integration
- 🚧 Audio effects processing (coming soon)

## Quick Start

1. **Hardware Setup**: Wire ESP32 to WM8782 and PCM5102A
   - See [Hardware Setup Guide](docs/HARDWARE_SETUP.md)

2. **Build & Flash**:
   ```powershell
   idf.py -p COM3 build flash monitor
   ```
   - See [Build Instructions](docs/BUILD_INSTRUCTIONS.md) for details

3. **Connect Audio**: 
   - Input: Connect audio source to WM8782
   - Output: Connect headphones/speakers to PCM5102A

4. **Test**: You should hear input audio at output with minimal latency

## Project Structure

```
esp-dsp/
├── main/
│   ├── esp-dsp.cpp         # Main application code
│   ├── audio_config.h      # Audio configuration and pin definitions
│   └── CMakeLists.txt
├── docs/
│   ├── HARDWARE_SETUP.md   # Wiring and hardware guide
│   └── BUILD_INSTRUCTIONS.md # Detailed build instructions
├── CMakeLists.txt          # Top-level CMake configuration
├── sdkconfig.defaults      # Default ESP-IDF configuration
└── README.md
```

## Configuration

### Pin Assignments

Edit `main/audio_config.h` to match your hardware:

```c
// ADC (WM8782) pins
#define I2S_ADC_BCLK    GPIO_NUM_26
#define I2S_ADC_WS      GPIO_NUM_25
#define I2S_ADC_DIN     GPIO_NUM_22

// DAC (PCM5102A) pins
#define I2S_DAC_BCLK    GPIO_NUM_14
#define I2S_DAC_WS      GPIO_NUM_27
#define I2S_DAC_DOUT    GPIO_NUM_12
```

### Sample Rate

Change sample rate in `main/audio_config.h`:
```c
#define SAMPLE_RATE     48000  // 44100, 48000, 96000, or 192000
```

## Adding Audio Processing

The current implementation is a pass-through. To add processing:

1. Open `main/esp-dsp.cpp`
2. Find the `audio_passthrough_task()` function
3. Add your DSP code between the read and write operations:

```cpp
// Read from ADC
i2s_channel_read(rx_handle, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);

// Add your processing here:
// Example: Simple volume control
for (int i = 0; i < DMA_BUFFER_SIZE; i++) {
    audio_buffer[i] = (audio_buffer[i] * volume) >> 8;
}

// Write to DAC
i2s_channel_write(tx_handle, audio_buffer, bytes_read, &bytes_written, portMAX_DELAY);
```

## Performance

- **Latency**: ~21ms at 48kHz with 1024 sample buffers (4 buffers)
- **CPU Usage**: ~10-20% (depends on processing)
- **Sample Rate**: Tested up to 96kHz (192kHz supported by hardware)

## Resources

- [ESP-IDF I2S Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
- [PCM5102A Datasheet](https://www.ti.com/lit/gpn/pcm5102a)
- [WM8782 Datasheet](https://www.cirrus.com/products/wm8782/)

## License

This project is provided as-is for educational and development purposes.

## Contributing

Feel free to open issues or submit pull requests for improvements!
