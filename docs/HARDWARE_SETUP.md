# Hardware Setup Guide

## Components

### WM8782 ADC (24-bit 192 kHz Stereo ADC)
- **Data Format**: I2S
- **Operating Mode**: Slave
- **Power Supply**: 3.3V or 5V (check your module)

### PCM5102A DAC (32-bit 384 kHz Stereo DAC)
- **Data Format**: I2S
- **Operating Mode**: Can be hardware or I2S controlled
- **Power Supply**: 3.3V or 5V (check your module)

## Wiring Diagram

### ESP32 to WM8782 (ADC Input)
```
ESP32          WM8782
------         ------
GPIO26  --->   BCLK  (Bit Clock)
GPIO25  --->   LRCLK (Word Select / Frame Clock)
GPIO22  <---   DOUT  (Data Output from ADC)
3.3V    --->   VDD
GND     --->   GND
```

### ESP32 to PCM5102A (DAC Output)
```
ESP32          PCM5102A
------         --------
GPIO14  --->   BCK   (Bit Clock)
GPIO27  --->   LRCK  (Word Select)
GPIO12  --->   DIN   (Data Input)
3.3V    --->   VIN
GND     --->   GND

For hardware mode (no MCU control needed):
SCK  --->  GND (I2S mode)
FLT  --->  GND (Normal filter)
DEMP --->  GND (No de-emphasis)
XSMT --->  3.3V (Normal operation)
FMT  --->  GND (I2S format)
```

## Notes

1. **Power Supply**: Ensure clean power supply for both ADC and DAC. Consider using separate voltage regulators for analog components.

2. **Grounding**: Use a common ground for all components. Consider star grounding for better audio quality.

3. **PCM5102A Configuration**: The PCM5102A can be used in hardware mode by hardwiring the control pins. This is the simplest setup.

4. **WM8782 Configuration**: The WM8782 is typically in hardware mode with control pins set via resistors/jumpers on the module.

5. **Signal Levels**: Both devices work with 3.3V logic levels, which is compatible with ESP32.

6. **Decoupling Capacitors**: Add 0.1µF ceramic capacitors near VDD pins of each IC.

7. **Audio Connections**: 
   - ADC input: Connect line-level audio source (≈2Vrms max)
   - DAC output: Can drive headphones or line-level input to amplifier

## Pin Customization

To change pin assignments, edit `main/audio_config.h`:
```c
#define I2S_ADC_BCLK    GPIO_NUM_26
#define I2S_ADC_WS      GPIO_NUM_25
#define I2S_ADC_DIN     GPIO_NUM_22

#define I2S_DAC_BCLK    GPIO_NUM_14
#define I2S_DAC_WS      GPIO_NUM_27
#define I2S_DAC_DOUT    GPIO_NUM_12
```

## Testing

1. Connect audio source to WM8782 input
2. Connect headphones or speaker to PCM5102A output
3. Flash the firmware and monitor logs
4. You should hear the input audio at the output with minimal latency
