# Build Instructions

## Prerequisites

1. **ESP-IDF**: Install ESP-IDF v5.0 or later
   - Follow the official guide: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/

2. **Python**: ESP-IDF requires Python 3.8 or later

3. **ESP32 Board**: Any ESP32 development board (ESP32-WROOM, ESP32-DevKit, etc.)

## Building the Project

### Windows (PowerShell)

1. **Set up ESP-IDF environment**:
   ```powershell
   # Navigate to your ESP-IDF installation directory
   cd $env:IDF_PATH
   .\export.ps1
   
   # Or if you've added it to your path permanently:
   # Just open a new terminal and ESP-IDF should be available
   ```

2. **Navigate to project directory**:
   ```powershell
   cd d:\esp-dsp\esp-dsp
   ```

3. **Configure the project** (optional):
   ```powershell
   idf.py menuconfig
   ```
   You can adjust settings in "ESP-DSP Audio Configuration"

4. **Build the project**:
   ```powershell
   idf.py build
   ```

5. **Flash to ESP32**:
   ```powershell
   # Replace COM3 with your ESP32's port
   idf.py -p COM3 flash
   ```

6. **Monitor serial output**:
   ```powershell
   idf.py -p COM3 monitor
   ```
   Press `Ctrl+]` to exit monitor

7. **Build, flash, and monitor in one command**:
   ```powershell
   idf.py -p COM3 flash monitor
   ```

### Linux/macOS

1. **Set up ESP-IDF environment**:
   ```bash
   . $HOME/esp/esp-idf/export.sh
   ```

2. **Navigate to project directory**:
   ```bash
   cd ~/esp-dsp/esp-dsp
   ```

3. **Build, flash, and monitor**:
   ```bash
   idf.py -p /dev/ttyUSB0 build flash monitor
   ```

## Configuration Options

### Sample Rate

Edit `main/audio_config.h` to change sample rate:

```c
#define SAMPLE_RATE     48000  // Recommended: 48000 Hz
```

**Note**: MCLK frequency will automatically scale (384 × sample rate)

### Buffer Size

Adjust DMA buffer size for latency vs. stability:

```c
#define DMA_BUFFER_SIZE  480   // Samples per buffer (smaller = less latency)
#define DMA_BUFFER_COUNT 8     // Number of DMA buffers (more = more stable)
```

**Latency calculation**: (DMA_BUFFER_SIZE × DMA_BUFFER_COUNT) / SAMPLE_RATE × 1000 ms

### Pin Configuration

Modify pin assignments in `main/audio_config.h`:

```c
// Shared clock signals (both ADC and DAC)
#define I2S_MCLK        GPIO_NUM_10  // Master clock (18.432MHz @ 48kHz)
#define I2S_DAC_BCLK    GPIO_NUM_5   // Bit clock (shared)
#define I2S_DAC_WS      GPIO_NUM_6   // Word select (shared)

// Data signals
#define I2S_ADC_DIN     GPIO_NUM_4   // Data from ADC
#define I2S_DAC_DOUT    GPIO_NUM_7   // Data to DAC
```

**Important**: MCLK, BCLK, and WS must connect to both ADC and DAC for synchronization.

## Troubleshooting

### Build Errors

1. **"command not found: idf.py"**
   - ESP-IDF environment not set up. Run the export script.

2. **"CMake Error"**
   - Make sure you're in the project root directory (where the top-level CMakeLists.txt is)

3. **Include errors**
   - These are normal in the editor. The code will compile correctly with ESP-IDF.

### Runtime Issues

1. **No audio output**
   - Check wiring connections (especially MCLK!)
   - Verify power supply to ADC/DAC (3.3V present?)
   - Check serial monitor for error messages
   - Verify PCM5102A control pins are configured correctly

2. **Distorted audio**
   - Check if equalizer is set to extreme values
   - Reduce input level
   - Check for ground loops
   - Try `eq disable` to bypass equalizer for testing

3. **Buffer underruns/overruns**
   - Increase `DMA_BUFFER_SIZE` for more stability
   - Increase `DMA_BUFFER_COUNT` for more buffering
   - Reduce CPU load (disable debug logging)

4. **Equalizer settings not saving**
   - Check NVS initialization in serial log
   - Verify flash is not write-protected
   - Use `eq save` command to manually trigger save
   - See [Persistent Settings Guide](PERSISTENT_SETTINGS.md)

## Expected Serial Output

```
I (xxx) ESP-DSP: ESP32 Audio Pass-Through Starting...
I (xxx) ESP-DSP: Sample Rate: 48000 Hz
I (xxx) ESP-DSP: Buffer Size: 480 samples
I (xxx) ESP-DSP: NVS initialized
I (xxx) ESP-DSP: Initializing I2S channels...
I (xxx) ESP-DSP: I2S channels created - RX: 0x3fcxxxxx, TX: 0x3fcxxxxx
I (xxx) ESP-DSP: I2S initialized successfully with shared clock domain and MCLK
I (xxx) ESP-DSP: MCLK: 18432000 Hz (48kHz * 384 = 18.432MHz)
I (xxx) EQUALIZER: Equalizer settings loaded from flash:
I (xxx) EQUALIZER:   Status: ENABLED
I (xxx) EQUALIZER:   60Hz:   +3.0 dB
I (xxx) EQUALIZER:   250Hz:  +2.0 dB
I (xxx) EQUALIZER:   1kHz:   +0.0 dB
I (xxx) EQUALIZER:   4kHz:   +1.0 dB
I (xxx) EQUALIZER:   12kHz:  +4.0 dB
I (xxx) ESP-DSP: Serial command interface started
I (xxx) ESP-DSP: Audio pass-through initialized
I (xxx) ESP-DSP: Connect audio source to ADC and speakers to DAC
I (xxx) ESP-DSP: Audio pass-through task started
```

## Using Serial Commands

After flashing, you can control the equalizer via serial commands:

```
> help                    # Show all available commands
> eq show                 # Display current EQ settings
> eq set 0 6.0            # Set 60Hz band to +6dB
> eq preset bass          # Load bass boost preset
> eq disable              # Bypass equalizer for A/B comparison
> eq enable               # Re-enable equalizer
```

See [Serial Commands Guide](SERIAL_COMMANDS.md) for complete reference.

## Next Steps

After successful operation:

1. Edit the `audio_passthrough_task()` function in `main/esp-dsp.cpp`
2. Add DSP algorithms between the read and write operations
3. Examples: EQ, compression, reverb, delay, filters, etc.
