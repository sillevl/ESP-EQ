# ESP32 DSP Audio Processor - Release Notes

## Version 1.0 - Full Release

### Overview

A production-ready real-time audio processing platform for ESP32 featuring a professional 5-band parametric equalizer with persistent settings.

### Key Features

#### Audio Processing
- **Shared I2S Clock Domain**: Single I2S peripheral for both input and output ensures perfect synchronization
- **Master Clock (MCLK)**: 18.432MHz clock (384× sample rate) for jitter-free audio
- **24-bit Audio Processing**: Professional-grade audio quality at 48kHz
- **Low Latency**: ~80ms end-to-end latency (configurable)
- **5-Band Parametric Equalizer**: 
  - Frequencies: 60Hz, 250Hz, 1kHz, 4kHz, 12kHz
  - Gain range: -12dB to +12dB per band
  - Biquad peaking filters with Q=0.707 (Butterworth response)
  - Optimized fixed-point implementation (Direct Form II Transposed)

#### User Interface
- **Real-Time Serial Commands**: Adjust EQ settings without recompiling
- **Persistent Settings**: All settings automatically saved to NVS flash
- **Built-in Presets**: Flat, Bass, Vocal, Rock, Jazz
- **Settings Survival**: Configuration survives power cycles and resets

#### Hardware Support
- **ADC**: WM8782 (24-bit 192kHz Stereo)
- **DAC**: PCM5102A (32-bit 384kHz Stereo)
- **MCU**: ESP32-C3 or any ESP32 variant with I2S support

### Performance Metrics

- **CPU Usage**: 5-10% with equalizer enabled
- **Latency**: ~80ms default (configurable: 20-200ms)
- **Free Heap**: ~156KB during operation
- **Flash Usage**: ~25 bytes for settings storage

### Architecture Highlights

#### Shared Clock Design
The system uses a unified clock architecture where:
- ESP32 generates MCLK, BCLK, and WS
- Both ADC and DAC receive the same clock signals
- Eliminates sample rate conversion and synchronization issues
- Provides professional-grade jitter performance

#### Signal Flow
```
Audio Input → WM8782 ADC → I2S RX (DMA) → ESP32 Processing
                                              ↓
                                    5-Band Equalizer
                                    (Biquad Filters)
                                              ↓
Audio Output ← PCM5102A DAC ← I2S TX (DMA) ← Processed Audio
```

#### Dual-Task Architecture
- **audio_task()**: High-priority task for real-time audio processing
- **serial_commands_task()**: Normal-priority task for user interface
- FreeRTOS-based multitasking ensures responsive control without audio interruption

### Serial Command Interface

Complete command-line interface for real-time control:

```
help                      # Show all commands
status                    # Display system information

eq show                   # Display current EQ settings
eq set <band> <gain>      # Set band gain (-12 to +12 dB)
eq enable                 # Enable equalizer
eq disable                # Bypass equalizer
eq reset                  # Reset filter state
eq preset <name>          # Load preset (flat, bass, vocal, rock, jazz)
eq save                   # Manually save settings
```

### Persistent Settings

- **Automatic saving**: Settings saved on every change
- **Automatic loading**: Settings restored on boot
- **NVS storage**: Uses ESP32's Non-Volatile Storage
- **Wear leveling**: Built-in NVS wear leveling protects flash
- **No performance impact**: Saving happens asynchronously

### Hardware Configuration

#### Pin Assignments (Default)
```
Shared Clocks:
  GPIO10 → MCLK  (to both ADC and DAC)
  GPIO5  → BCLK  (to both ADC and DAC)
  GPIO6  → WS    (to both ADC and DAC)

Data Signals:
  GPIO4  ← ADC DOUT
  GPIO7  → DAC DIN
```

#### PCM5102A Hardware Mode
```
FLT  → GND
DEMP → GND
XSMT → 3.3V
FMT  → GND
SCK  → GPIO10 (MCLK)
```

### Documentation

Complete documentation set included:

1. **README.md** - Project overview and quick reference
2. **QUICK_START.md** - Get running in 5 steps
3. **docs/HARDWARE_SETUP.md** - Detailed wiring guide with shared clock architecture
4. **docs/BUILD_INSTRUCTIONS.md** - Complete build and configuration guide
5. **docs/PROJECT_OVERVIEW.md** - System architecture and technical details
6. **docs/EQUALIZER.md** - Equalizer specifications and presets
7. **docs/SERIAL_COMMANDS.md** - Complete command reference
8. **docs/PERSISTENT_SETTINGS.md** - NVS storage documentation
9. **docs/ADDING_EFFECTS.md** - Guide for custom DSP effects
10. **docs/TROUBLESHOOTING.md** - Common issues and solutions

### Known Limitations

- **Sample Rate**: Optimized for 48kHz (hardware supports up to 192kHz)
- **Latency**: Minimum ~20ms due to DMA buffering and processing overhead
- **GPIO Pins**: ESP32-C3 has limited GPIOs, avoid strapping and flash pins
- **Processing Budget**: ~20ms per buffer at 48kHz for additional effects

### Tested Configurations

- **ESP32-C3**: Full testing, production ready
- **ESP-IDF**: v5.0+ required
- **Hardware**: WM8782 + PCM5102A modules
- **Sample Rates**: 48kHz (primary), 44.1kHz, 96kHz tested

### Future Enhancements

Possible additions for future versions:
- Additional filter types (high-pass, low-pass, band-pass)
- Compressor/limiter
- Reverb/delay effects
- Web interface for remote control
- Multiple preset slots
- Spectrum analyzer
- Auto-gain control

### Breaking Changes from Previous Versions

If upgrading from early development versions:

1. **Pin Configuration Changed**: MCLK now required, shared clock architecture
2. **Buffer Sizes**: Default changed from 1024 to 480 samples
3. **Function Names**: `audio_passthrough_task()` renamed to `audio_task()`
4. **NVS Required**: NVS must be initialized for equalizer to function

### Migration Guide

To update from development versions:

1. **Update Wiring**: Add MCLK connections to both ADC and DAC
2. **Update Pin Definitions**: Check `audio_config.h` against new defaults
3. **Rebuild from Clean**: Run `idf.py fullclean && idf.py build`
4. **Erase Flash** (optional): Run `idf.py erase-flash` for clean start

### Credits

- ESP-IDF Framework by Espressif Systems
- WM8782 ADC datasheet by Cirrus Logic
- PCM5102A DAC datasheet by Texas Instruments
- Biquad filter implementation based on Audio EQ Cookbook by Robert Bristow-Johnson

### Support

For issues, questions, or contributions:
- Review documentation in `docs/` directory
- Check `docs/TROUBLESHOOTING.md` for common issues
- Verify hardware connections against `docs/HARDWARE_SETUP.md`

### License

See LICENSE file for details.

---

**Release Date**: 2025  
**Platform**: ESP32 (ESP-IDF v5.0+)  
**Status**: Production Ready
