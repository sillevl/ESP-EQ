# Project Overview

## ESP32 Audio DSP Platform

This project implements a real-time audio processing platform using ESP32 microcontroller with professional audio codecs.

## Architecture

```
Audio Input → WM8782 ADC → I2S → ESP32 → I2S → PCM5102A DAC → Audio Output
                 (24-bit)         (Processing)        (32-bit)
```

## Key Components

### Hardware

1. **ESP32 Microcontroller**
   - Dual-core processor (240 MHz)
   - Dual I2S interfaces
   - FreeRTOS operating system
   - DMA-based audio transfer

2. **WM8782 ADC**
   - 24-bit resolution
   - Up to 192 kHz sample rate
   - Stereo input
   - I2S slave mode
   - Low noise: 108 dB SNR

3. **PCM5102A DAC**
   - 32-bit resolution  
   - Up to 384 kHz sample rate
   - Stereo output
   - I2S mode
   - 112 dB SNR

### Software Architecture

```
┌─────────────────────────────────────────┐
│         ESP-IDF Framework                │
├─────────────────────────────────────────┤
│  FreeRTOS Task: audio_passthrough_task  │
│  ┌───────────────────────────────────┐  │
│  │  1. Read from ADC via I2S DMA     │  │
│  │  2. Process audio (DSP)           │  │
│  │  3. Write to DAC via I2S DMA      │  │
│  └───────────────────────────────────┘  │
├─────────────────────────────────────────┤
│         I2S Driver (ESP-IDF)            │
│  ┌──────────────┐    ┌──────────────┐  │
│  │  RX Channel  │    │  TX Channel  │  │
│  │  (ADC Input) │    │  (DAC Output)│  │
│  └──────────────┘    └──────────────┘  │
├─────────────────────────────────────────┤
│            Hardware (GPIO/I2S)          │
└─────────────────────────────────────────┘
```

## Data Flow

1. **Input Stage**
   - Audio signal enters WM8782 ADC
   - ADC converts analog to 24-bit digital
   - I2S interface transfers data to ESP32
   - DMA writes to circular buffer

2. **Processing Stage**
   - FreeRTOS task reads buffer
   - DSP algorithms applied (optional)
   - Processed audio ready for output

3. **Output Stage**
   - Processed audio written to DMA buffer
   - I2S interface transfers to PCM5102A
   - DAC converts digital to analog
   - Audio signal output

## Memory Management

- **Audio buffers**: Allocated in DRAM
- **DMA descriptors**: Managed by I2S driver
- **Stack**: 4096 bytes per audio task
- **Heap**: Dynamic allocation for effects

## Timing and Latency

### Buffer Configuration
```
DMA_BUFFER_COUNT = 4
DMA_BUFFER_SIZE = 1024 samples
SAMPLE_RATE = 48000 Hz
```

### Latency Calculation
```
Total samples buffered = 1024 × 4 = 4096 samples
Latency = 4096 / 48000 = 85.3 ms
```

### Processing Budget
```
Buffer duration = 1024 / 48000 = 21.3 ms
Available for DSP = ~20 ms (leaves margin for system)
```

## I2S Configuration

### Standard Mode Parameters
- **Format**: Philips I2S standard
- **Bit width**: 24-bit data
- **Channels**: 2 (stereo)
- **Clock**: Master mode (ESP32 generates clocks)

### Clock Frequencies (48 kHz)
- **BCLK**: 48000 × 2 × 24 = 2.304 MHz
- **LRCLK (WS)**: 48000 Hz
- **MCLK**: Not used (both codecs have internal PLLs)

## File Structure

```
esp-dsp/
├── main/
│   ├── esp-dsp.cpp          # Main application
│   ├── audio_config.h       # Configuration constants
│   ├── CMakeLists.txt       # Component build config
│   └── Kconfig.projbuild    # menuconfig options
├── docs/
│   ├── BUILD_INSTRUCTIONS.md
│   ├── HARDWARE_SETUP.md
│   ├── ADDING_EFFECTS.md
│   ├── TROUBLESHOOTING.md
│   └── PROJECT_OVERVIEW.md (this file)
├── CMakeLists.txt           # Project build config
├── sdkconfig.defaults       # Default ESP-IDF config
├── .gitignore
└── README.md
```

## Configuration Options

### Compile-Time (audio_config.h)
- Sample rate
- Buffer sizes
- Pin assignments
- Bit depth

### Run-Time (menuconfig)
- Log levels
- FreeRTOS settings
- ESP32 CPU frequency
- Memory allocation

## Performance Characteristics

### Current Implementation (Pass-Through)
- **CPU Usage**: 5-15%
- **Latency**: 85 ms (default config)
- **Throughput**: 2.304 Mbps (I2S data rate)
- **Memory**: ~20 KB (buffers + stack)

### With DSP Processing
- **CPU Usage**: Varies (depends on algorithms)
- **Available processing**: ~15-20 ms per buffer
- **Trade-offs**: More processing = higher latency or lower sample rate

## Extensibility

### Adding New Features

1. **Audio Effects**
   - Modify `audio_passthrough_task()`
   - Add processing between read/write
   - See `docs/ADDING_EFFECTS.md`

2. **External Control**
   - Add WiFi/Bluetooth
   - Implement MQTT/HTTP control
   - Add physical controls (potentiometers, buttons)

3. **Multiple Sample Rates**
   - Implement rate switching
   - Add resampling algorithms
   - Dynamic reconfiguration

4. **Advanced DSP**
   - Use ESP-DSP library
   - Implement FFT analysis
   - Add machine learning

## Design Decisions

### Why Two I2S Channels?
- Separate RX and TX for simultaneous operation
- Independent clock domains (future: different rates)
- Cleaner code organization

### Why DMA?
- CPU-free data transfer
- Enables real-time processing
- Prevents buffer overruns/underruns

### Why FreeRTOS Task?
- Real-time guarantees
- Easier to add multiple tasks (UI, network, etc.)
- Standard ESP-IDF pattern

### Why 24-bit?
- Matches WM8782 capability
- Good balance of quality vs. performance
- Sufficient dynamic range (144 dB theoretical)

## Future Enhancements

### Planned Features
- [ ] Web interface for control
- [ ] Preset management
- [ ] Spectrum analyzer display
- [ ] Multi-band EQ
- [ ] Dynamics processing (compressor, limiter)
- [ ] Reverb/delay effects
- [ ] MIDI control
- [ ] USB audio interface mode

### Hardware Improvements
- [ ] OLED display
- [ ] Rotary encoders for control
- [ ] Balanced audio inputs/outputs
- [ ] External clock input (word clock)
- [ ] Multiple input/output channels

## Resources

- [ESP-IDF I2S Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
- [ESP-DSP Library](https://github.com/espressif/esp-dsp)
- [WM8782 Datasheet](https://www.cirrus.com/products/wm8782/)
- [PCM5102A Datasheet](https://www.ti.com/lit/gpn/pcm5102a)

## License

This project is provided as-is for educational and development purposes.
