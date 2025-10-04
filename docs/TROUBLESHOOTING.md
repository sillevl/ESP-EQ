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
3. ✓ I2S pins correctly wired (BCLK, WS/LRCLK, Data)
4. ✓ Audio source connected to ADC input
5. ✓ Headphones/speakers connected to DAC output
6. ✓ PCM5102A control pins configured (if not hardwired)

**Debug Steps**:
```powershell
# Monitor serial output for errors
idf.py -p COM3 monitor

# Look for these messages:
# - "I2S ADC initialized successfully"
# - "I2S DAC initialized successfully"
# - "Audio pass-through task started"
```

#### Distorted or Noisy Audio

**Possible Causes**:

1. **Power supply issues**
   - Use clean, stable power source
   - Add decoupling capacitors (0.1µF) near ICs
   - Use separate regulators for analog and digital power if possible

2. **Ground loops**
   - Ensure single-point ground
   - Keep analog and digital grounds separate if possible
   - Use star grounding topology

3. **Sample rate mismatch**
   - Verify ADC and DAC both configured for same rate
   - Check if audio source is at different sample rate

4. **Buffer underruns/overruns**
   ```cpp
   // Increase buffer size in audio_config.h
   #define DMA_BUFFER_SIZE  2048  // Was 1024
   ```

5. **Improper wire connections**
   - Double-check pinout against datasheet
   - Verify BCLK and WS signals are connected correctly
   - Use logic analyzer to verify I2S signals if available

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
2. Hardware not responding
3. DMA buffer issues

**Solution**:
```cpp
// Add more detailed logging in esp-dsp.cpp
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "I2S read error: %s (0x%x)", esp_err_to_name(ret), ret);
}
```

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
1. Decrease `DMA_BUFFER_SIZE` (e.g., 512 or 256)
2. Decrease `DMA_BUFFER_COUNT` (e.g., 2)
3. Trade-off: Lower values may cause dropouts

**Calculate latency**:
```
Latency (ms) = (DMA_BUFFER_SIZE * DMA_BUFFER_COUNT) / SAMPLE_RATE * 1000
Example: (1024 * 4) / 48000 * 1000 = 85.3ms
```

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
