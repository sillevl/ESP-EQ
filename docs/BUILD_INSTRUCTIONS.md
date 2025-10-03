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
#define SAMPLE_RATE     48000  // Change to 44100, 96000, or 192000
```

### Buffer Size
Adjust DMA buffer size for latency vs. stability:
```c
#define DMA_BUFFER_SIZE  1024  // Smaller = less latency, larger = more stable
```

### Pin Configuration
Modify pin assignments in `main/audio_config.h`:
```c
// ADC Pins
#define I2S_ADC_BCLK    GPIO_NUM_26
#define I2S_ADC_WS      GPIO_NUM_25
#define I2S_ADC_DIN     GPIO_NUM_22

// DAC Pins
#define I2S_DAC_BCLK    GPIO_NUM_14
#define I2S_DAC_WS      GPIO_NUM_27
#define I2S_DAC_DOUT    GPIO_NUM_12
```

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
   - Check wiring connections
   - Verify power supply to ADC/DAC
   - Check serial monitor for error messages

2. **Distorted audio**
   - May need to adjust buffer sizes
   - Check for ground loops
   - Verify sample rate matches audio source

3. **Buffer underruns/overruns**
   - Increase `DMA_BUFFER_SIZE` for more stability
   - Increase `DMA_BUFFER_COUNT` for more buffering

## Expected Serial Output

```
I (xxx) ESP-DSP: ESP32 Audio Pass-Through Starting...
I (xxx) ESP-DSP: Sample Rate: 48000 Hz
I (xxx) ESP-DSP: Channels: 2
I (xxx) ESP-DSP: Buffer Size: 1024 samples
I (xxx) ESP-DSP: Initializing I2S ADC (WM8782)...
I (xxx) ESP-DSP: I2S ADC initialized successfully
I (xxx) ESP-DSP: Initializing I2S DAC (PCM5102A)...
I (xxx) ESP-DSP: I2S DAC initialized successfully
I (xxx) ESP-DSP: Audio pass-through initialized successfully
I (xxx) ESP-DSP: Audio pass-through task started
```

## Next Steps

After successful pass-through operation, you can add audio processing:

1. Edit the `audio_passthrough_task()` function in `main/esp-dsp.cpp`
2. Add DSP algorithms between the read and write operations
3. Examples: EQ, compression, reverb, delay, filters, etc.
