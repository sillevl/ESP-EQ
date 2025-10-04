# Quick Start Guide

Get your ESP32 audio processor up and running in 5 steps!

## Prerequisites

- ESP-IDF v5.0+ installed
- ESP32 development board
- WM8782 ADC module
- PCM5102A DAC module
- Audio source (phone, mixer, etc.)
- Headphones or powered speakers

## Step 1: Wire the Hardware

### Shared Clock Connections (CRITICAL!)

**ESP32 → BOTH WM8782 AND PCM5102A**

These signals must connect to BOTH devices:

```
GPIO10 → WM8782 MCLK  AND  PCM5102A SCK (MCLK input)
GPIO5  → WM8782 BCLK  AND  PCM5102A BCK
GPIO6  → WM8782 LRCLK AND  PCM5102A LRCK
```

### Data Connections

**ESP32 → WM8782 (ADC Input)**

```
GPIO4  → DOUT (audio data from ADC)
3.3V   → VDD
GND    → GND
```

**ESP32 → PCM5102A (DAC Output)**

```
GPIO7  → DIN (audio data to DAC)
3.3V   → VIN
GND    → GND
```

### PCM5102A Control Pins

Hardwire these pins on the PCM5102A:

```
FLT  → GND (Normal filter)
DEMP → GND (No de-emphasis)
XSMT → 3.3V (Normal operation)
FMT  → GND (I2S format)
```

**Note**: SCK on PCM5102A is used as MCLK input (connected to GPIO10).

## Step 2: Set Up ESP-IDF

**Windows PowerShell:**
```powershell
cd $env:IDF_PATH
.\export.ps1
```

**Linux/macOS:**
```bash
. $HOME/esp/esp-idf/export.sh
```

## Step 3: Build and Flash

```powershell
cd d:\esp-dsp\esp-dsp
idf.py -p COM3 build flash monitor
```

Replace `COM3` with your ESP32's serial port.

## Step 4: Connect Audio

1. **Input**: Plug audio source into WM8782 input
2. **Output**: Connect headphones/speakers to PCM5102A output
3. **Power**: Ensure ESP32 and modules are powered

## Step 5: Test

You should hear your audio source playing through the output with ~80ms latency and EQ processing applied.

Expected serial output:

```
I (xxx) ESP-DSP: ESP32 Audio Pass-Through Starting...
I (xxx) ESP-DSP: Sample Rate: 48000 Hz
I (xxx) ESP-DSP: Buffer Size: 480 samples
I (xxx) ESP-DSP: NVS initialized
I (xxx) ESP-DSP: Initializing I2S channels...
I (xxx) ESP-DSP: I2S initialized successfully with shared clock domain and MCLK
I (xxx) ESP-DSP: MCLK: 18432000 Hz (48kHz * 384 = 18.432MHz)
I (xxx) EQUALIZER: Equalizer settings loaded from flash
I (xxx) EQUALIZER:   Status: ENABLED
I (xxx) EQUALIZER:   60Hz:   +3.0 dB
I (xxx) EQUALIZER:   250Hz:  +2.0 dB
I (xxx) EQUALIZER:   1kHz:   +0.0 dB
I (xxx) EQUALIZER:   4kHz:   +1.0 dB
I (xxx) EQUALIZER:   12kHz:  +4.0 dB
I (xxx) ESP-DSP: Serial command interface started
I (xxx) ESP-DSP: Audio pass-through task started
```

## Step 6: Control the Equalizer

Use serial commands to adjust the EQ in real-time:

```
> eq show                  # Display current settings
> eq preset bass           # Load bass boost preset
> eq set 0 6.0             # Set 60Hz to +6dB
> eq disable               # Bypass for A/B comparison
> eq enable                # Turn EQ back on
> help                     # Show all commands
```

All settings are automatically saved to flash!

## Troubleshooting

### No audio?
- Check power connections (3.3V present?)
- Verify all GND connected
- Check pin wiring (BCLK, WS, Data)
- See `docs/TROUBLESHOOTING.md`

### Distorted audio?
- Reduce input level
- Check for ground loops
- Add 0.1µF caps near VDD pins

### Compile errors?
- ESP-IDF activated? Run export script
- Correct directory? Should see `CMakeLists.txt`

## Next Steps

1. **Adjust equalizer**: Edit EQ gains in `main/esp-dsp.cpp` - see `docs/EQUALIZER.md`
2. **Customize pins**: Edit `main/audio_config.h`
3. **Change sample rate**: Edit `SAMPLE_RATE` in `audio_config.h`
4. **Add more effects**: See `docs/ADDING_EFFECTS.md`
5. **Lower latency**: Reduce `DMA_BUFFER_SIZE` and `DMA_BUFFER_COUNT`

## Pin Reference

| Function | ESP32 GPIO | WM8782 Pin | PCM5102A Pin |
|----------|-----------|------------|--------------|
| ADC BCLK | GPIO26    | BCLK       | -            |
| ADC WS   | GPIO25    | LRCLK      | -            |
| ADC Data | GPIO22    | DOUT       | -            |
| DAC BCLK | GPIO14    | -          | BCK          |
| DAC WS   | GPIO27    | -          | LRCK         |
| DAC Data | GPIO12    | -          | DIN          |

## Commands Cheat Sheet

```powershell
# Build only
idf.py build

# Flash only  
idf.py -p COM3 flash

# Monitor only
idf.py -p COM3 monitor

# Clean build
idf.py fullclean

# Configuration menu
idf.py menuconfig

# Build + Flash + Monitor
idf.py -p COM3 build flash monitor
```

## Documentation

- 🎚️ [Equalizer Guide](docs/EQUALIZER.md) - 5-band EQ configuration and presets
- 📖 [Hardware Setup](docs/HARDWARE_SETUP.md) - Detailed wiring guide
- 🔧 [Build Instructions](docs/BUILD_INSTRUCTIONS.md) - Complete build guide
- ✨ [Adding Effects](docs/ADDING_EFFECTS.md) - DSP examples
- 🐛 [Troubleshooting](docs/TROUBLESHOOTING.md) - Problem solving
- 📋 [Project Overview](docs/PROJECT_OVERVIEW.md) - Architecture details

## Support

Having issues? Check the troubleshooting guide or ESP32 forums.

Happy coding! 🎵
