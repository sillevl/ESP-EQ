# Troubleshooting Guide

## Common Issues and Solutions

### Build Issues

#### "idf.py: command not found"

**Problem**: ESP-IDF environment not activated.

**Solution**:
```powershell
# Windows PowerShell
cd $env:IDF_PATH
.\export.ps1

# Linux/macOS
. $HOME/esp/esp-idf/export.sh
```

#### "CMake Error: Could not find a package configuration file"

**Problem**: Wrong directory or corrupted ESP-IDF installation.

**Solution**:
1. Verify you're in the project root: `cd d:\esp-dsp\esp-dsp`
2. Clean build: `idf.py fullclean`
3. Rebuild: `idf.py build`

### Hardware/Wiring Issues

#### No Audio Output

**Checklist**:

1. ✓ Power supply connected to both ADC and DAC (3.3V or 5V)
2. ✓ Ground connected between ESP32, ADC, and DAC
3. ✓ **MCLK connected to both ADC and DAC** (GPIO10 → WM8782 MCLK AND PCM5102A SCK)
4. ✓ **BCLK connected to both ADC and DAC** (GPIO5 to both)
5. ✓ **WS connected to both ADC and DAC** (GPIO6 to both)
6. ✓ Data pins: GPIO4 from ADC, GPIO7 to DAC
7. ✓ Audio source connected to ADC input
8. ✓ Headphones/speakers connected to DAC output
9. ✓ PCM5102A control pins configured (FLT→GND, DEMP→GND, XSMT→3.3V, FMT→GND)

**Debug Steps**:

```powershell
# Monitor serial output for errors
idf.py -p COM3 monitor

# Look for these messages:
# - "I2S initialized successfully with shared clock domain and MCLK"
# - "MCLK: 18432000 Hz"
# - "Equalizer settings loaded from flash"
# - "Audio pass-through task started"
```

**Common Issues**:

- **Missing MCLK**: Most common issue! MCLK (GPIO10) MUST connect to BOTH devices
- **Wrong pin mapping**: Double-check GPIO numbers match audio_config.h
- **PCM5102A not configured**: Control pins must be hardwired correctly
- **No power to codecs**: Verify 3.3V is present on VDD/VIN pins

#### Distorted or Noisy Audio

**Possible Causes**:

1. **Excessive EQ Boost**
   - High EQ gains can cause clipping
   - Try `eq preset flat` to disable EQ and test
   - Reduce individual band gains: `eq set 0 0.0`

2. **Power supply issues**
   - Use clean, stable power source
   - Add decoupling capacitors (0.1µF) near ICs
   - Use separate regulators for analog and digital power if possible

3. **Ground loops**
   - Ensure single-point ground
   - Keep analog and digital grounds separate if possible
   - Use star grounding topology

4. **Sample rate mismatch**
   - Both ADC and DAC use shared clocks, so this is unlikely
   - But check if audio source is at different sample rate

5. **Buffer underruns/overruns**

   ```cpp
   // Increase buffer size in audio_config.h
   #define DMA_BUFFER_SIZE  960   // Was 480
   #define DMA_BUFFER_COUNT 12    // Was 8
   ```

6. **Improper clock connections**
   - Verify MCLK is connected to BOTH ADC and DAC
   - Check BCLK and WS are shared between both devices
   - Use logic analyzer to verify I2S signals if available

7. **Bad solder joints or loose wires**
   - Especially on MCLK/BCLK/WS shared connections
   - Check continuity with multimeter

#### One Channel Missing (Mono Output)

**Check**:
1. WS/LRCLK signal connected?
2. Both left and right audio inputs connected?
3. Verify I2S channel configuration is stereo
4. Check solder joints on stereo connections

### Runtime Issues

#### "I2S read error" or "I2S write error"

**Possible causes**:

1. Incorrect pin configuration
2. Hardware not responding (ADC/DAC not powered or not receiving clocks)
3. DMA buffer issues
4. Missing MCLK connection

**Solution**:

First, verify clock signals are present:

- MCLK should be 18.432 MHz (at 48kHz sample rate)
- BCLK should be 2.304 MHz
- WS should be 48 kHz

Then check serial logs for specific errors. The code already logs detailed error information.

**If errors persist**:

- Verify GPIO pins are not being used by other peripherals
- Check ESP32-C3 strapping pins are not conflicting
- Try different GPIO pins and update `audio_config.h`
- Add debug logging to see which channel is failing

#### High CPU Usage / System Instability

**Check**:
1. Processing time per buffer:
   ```cpp
   #include "esp_timer.h"
   
   int64_t start = esp_timer_get_time();
   // ... processing ...
   int64_t elapsed = esp_timer_get_time() - start;
   ESP_LOGI(TAG, "Processing: %lld us", elapsed);
   ```

2. Target: < 20ms per buffer at 48kHz (1024 samples)
3. If too high: Optimize DSP code or reduce sample rate

#### Buffer Size Warnings

**Message**: "Buffer size mismatch: read X, written Y"

**Cause**: DMA buffer configuration mismatch

**Solution**:
1. Ensure same buffer settings for ADC and DAC
2. Check `DMA_BUFFER_SIZE` in `audio_config.h`
3. Verify both I2S channels use same config

### Audio Quality Issues

#### Latency Too High

**Reduce latency**:

1. Decrease `DMA_BUFFER_SIZE` (e.g., 240 or 120)
2. Decrease `DMA_BUFFER_COUNT` (e.g., 4 or 2)
3. Trade-off: Lower values may cause dropouts

**Calculate latency**:

```
Latency (ms) = (DMA_BUFFER_SIZE * DMA_BUFFER_COUNT) / SAMPLE_RATE * 1000

Current: (480 * 8) / 48000 * 1000 = 80ms
Lower:   (240 * 4) / 48000 * 1000 = 20ms
```

**Note**: Some latency is unavoidable due to DSP processing overhead. The 5-band equalizer adds ~1-2ms processing time.

#### Audio Dropouts/Clicks

**Increase stability**:
1. Increase `DMA_BUFFER_SIZE` (e.g., 2048)
2. Increase `DMA_BUFFER_COUNT` (e.g., 6)
3. Reduce CPU load (simplify DSP processing)
4. Check for WiFi/BT interference if enabled

#### DC Offset

**Add DC blocking filter**:
```cpp
// See ADDING_EFFECTS.md - High-pass filter example
```

### Testing and Debugging


#### Signal Level Monitoring

```cpp
// Monitor peak levels
static int log_counter = 0;
if (++log_counter >= 1000) {
    int32_t peak = 0;
    for (int i = 0; i < DMA_BUFFER_SIZE; i++) {
        int32_t abs_val = abs(audio_buffer[i]);
        if (abs_val > peak) peak = abs_val;
    }
    float peak_db = 20.0f * log10f((float)peak / (1 << 23));
    ESP_LOGI(TAG, "Peak level: %.1f dBFS", peak_db);
    log_counter = 0;
}
```

#### Logic Analyzer Verification

If you have a logic analyzer, verify I2S signals:

**Expected at 48kHz, 24-bit stereo**:
- BCLK frequency: 48000 × 2 × 24 = 2.304 MHz
- WS frequency: 48000 Hz (48 kHz)
- WS duty cycle: 50% (one half-period per channel)

### Pin Configuration Verification

Use this code to verify pin configuration:

```cpp
ESP_LOGI(TAG, "ADC Pins - BCLK:%d WS:%d DIN:%d", 
         I2S_ADC_BCLK, I2S_ADC_WS, I2S_ADC_DIN);
ESP_LOGI(TAG, "DAC Pins - BCLK:%d WS:%d DOUT:%d", 
         I2S_DAC_BCLK, I2S_DAC_WS, I2S_DAC_DOUT);
```

### Getting Help

If you're still stuck:

1. Check ESP-IDF logs with increased verbosity:
   ```
   idf.py menuconfig
   → Component config → Log output → Default log verbosity → Verbose
   ```

2. Enable I2S driver debug:
   ```
   idf.py menuconfig
   → Component config → Driver configurations → I2S → I2S debug log
   ```

3. Post on ESP32 forums with:
   - Full serial log output
   - Hardware photos/schematic
   - ESP-IDF version (`idf.py --version`)
   - ESP32 board type

### Hardware Testing Checklist

Before software debugging, verify hardware:

- [ ] Measure 3.3V (or 5V) on ADC VDD pin
- [ ] Measure 3.3V (or 5V) on DAC VIN pin
- [ ] Continuity test: ESP32 GND to ADC GND to DAC GND
- [ ] Verify ADC input with audio source (check with multimeter AC mode)
- [ ] Check for BCLK signal (should see ~2.3MHz square wave)
- [ ] Check for WS signal (should see 48kHz square wave)
- [ ] Verify PCM5102A control pin configuration

## Performance Benchmarks

### Typical Values

- **Pass-through latency**: 21-85ms (depends on buffer size)
- **CPU usage**: 5-15% (pass-through only)
- **Maximum sample rate**: 96kHz tested, 192kHz supported
- **Processing budget per buffer**: ~20ms (at 48kHz, 1024 samples)

### Optimization Tips

1. **Use integer math**: Avoid `float`/`double` operations
2. **Compiler optimization**: Already enabled in ESP-IDF (O2)
3. **Use IRAM**: For critical functions (add `IRAM_ATTR`)
4. **Profile code**: Use `esp_timer_get_time()`
5. **Consider ESP-DSP library**: Hardware-optimized functions
