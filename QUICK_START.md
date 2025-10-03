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

### Minimum Connections

**ESP32 → WM8782 (ADC)**
```
GPIO26 → BCLK
GPIO25 → WS (LRCLK)
GPIO22 → DOUT
3.3V   → VDD
GND    → GND
```

**ESP32 → PCM5102A (DAC)**
```
GPIO14 → BCK
GPIO27 → LRCK  
GPIO12 → DIN
3.3V   → VIN
GND    → GND
```

**PCM5102A Hardware Mode Pins** (tie these)
```
SCK  → GND
FLT  → GND
DEMP → GND
XSMT → 3.3V
FMT  → GND
```

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

You should hear your audio source playing through the output with about 85ms latency.

Expected serial output:
```
I (xxx) ESP-DSP: ESP32 Audio Pass-Through Starting...
I (xxx) ESP-DSP: Sample Rate: 48000 Hz
I (xxx) ESP-DSP: I2S ADC initialized successfully
I (xxx) ESP-DSP: I2S DAC initialized successfully
I (xxx) ESP-DSP: Audio pass-through task started
```

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

1. **Customize pins**: Edit `main/audio_config.h`
2. **Change sample rate**: Edit `SAMPLE_RATE` in `audio_config.h`
3. **Add effects**: See `docs/ADDING_EFFECTS.md`
4. **Lower latency**: Reduce `DMA_BUFFER_SIZE` and `DMA_BUFFER_COUNT`

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

- 📖 [Hardware Setup](docs/HARDWARE_SETUP.md) - Detailed wiring guide
- 🔧 [Build Instructions](docs/BUILD_INSTRUCTIONS.md) - Complete build guide
- ✨ [Adding Effects](docs/ADDING_EFFECTS.md) - DSP examples
- 🐛 [Troubleshooting](docs/TROUBLESHOOTING.md) - Problem solving
- 📋 [Project Overview](docs/PROJECT_OVERVIEW.md) - Architecture details

## Support

Having issues? Check the troubleshooting guide or ESP32 forums.

Happy coding! 🎵
