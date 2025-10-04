# Hardware Setup Guide

## Architecture Overview

This project uses a **shared I2S clock domain** where both the ADC and DAC are synchronized using the same clock signals. The ESP32 generates MCLK, BCLK, and WS (word select) which are shared between both audio codecs for jitter-free audio.

## Components

### WM8782 ADC (24-bit 192 kHz Stereo ADC)
- **Data Format**: I2S
- **Operating Mode**: Slave (receives clocks from ESP32)
- **Power Supply**: 3.3V or 5V (check your module)
- **Requires**: MCLK for optimal operation

### PCM5102A DAC (32-bit 384 kHz Stereo DAC)
- **Data Format**: I2S
- **Operating Mode**: Hardware mode (control pins hardwired)
- **Power Supply**: 3.3V or 5V (check your module)
- **Features**: Internal PLL for jitter reduction

## Wiring Diagram

### Shared Clock Signals (ESP32 to Both ADC and DAC)

**CRITICAL**: These signals connect to BOTH the ADC and DAC:

```
ESP32 Pin      WM8782         PCM5102A
---------      ------         --------
GPIO10   --->  MCLK    AND    SCK (MCLK input)
GPIO5    --->  BCLK    AND    BCK
GPIO6    --->  LRCLK   AND    LRCK
```

### Data Signals (Separate for each device)

**ADC Input (WM8782 → ESP32)**:
```
ESP32          WM8782
------         ------
GPIO4   <---   DOUT  (Audio data from ADC)
3.3V    --->   VDD
GND     --->   GND
```

**DAC Output (ESP32 → PCM5102A)**:
```
ESP32          PCM5102A
------         --------
GPIO7   --->   DIN   (Audio data to DAC)
3.3V    --->   VIN
GND     --->   GND
```

### PCM5102A Control Pins (Hardware Mode)

Hardwire these pins on the PCM5102A module:

```
PCM5102A Pin   Connection
------------   ----------
FLT      --->  GND (Normal filter)
DEMP     --->  GND (No de-emphasis)
XSMT     --->  3.3V (Normal operation)
FMT      --->  GND (I2S format)
```

**Note**: SCK is used as MCLK input on PCM5102A (connected to GPIO10).

## Complete Wiring Summary

```
ESP32-C3       WM8782 ADC     PCM5102A DAC
--------       ----------     ------------
GPIO10   --->  MCLK     AND   SCK (MCLK)
GPIO5    --->  BCLK     AND   BCK
GPIO6    --->  LRCLK    AND   LRCK
GPIO4    <---  DOUT
GPIO7    --->               DIN
3.3V     --->  VDD      AND   VIN
GND      --->  GND      AND   GND

                         PCM5102A Control Pins:
                         FLT  --> GND
                         DEMP --> GND
                         XSMT --> 3.3V
                         FMT  --> GND
```

## Important Notes

### 1. Shared Clock Domain
**CRITICAL**: MCLK, BCLK, and LRCLK must be connected to BOTH the ADC and DAC. This ensures:
- Perfect synchronization between input and output
- Eliminates sample rate conversion issues
- Minimizes jitter and audio artifacts

### 2. Master Clock (MCLK)
- ESP32 generates 18.432MHz MCLK (384 × 48kHz sample rate)
- WM8782 requires MCLK for proper operation
- PCM5102A uses MCLK input via SCK pin
- **Do not skip MCLK connection** - it's essential for audio quality

### 3. Power Supply
- Use clean, stable power supply for both ADC and DAC
- Consider separate voltage regulators for analog components
- Add 0.1µF ceramic capacitors near VDD pins of each IC
- Recommended: 3.3V for best compatibility with ESP32

### 4. Grounding
- Use a common ground for all components
- Consider star grounding topology for better audio quality
- Avoid ground loops

### 5. Audio Connections
- **ADC input**: Connect line-level audio source (≈2Vrms max)
- **DAC output**: Can drive headphones or line-level input to amplifier
- Use shielded audio cables for longer connections

### 6. Signal Integrity
- Keep I2S signal traces short
- Use proper pull-ups/pull-downs if experiencing signal integrity issues
- Avoid running I2S signals parallel to high-speed digital signals

## Pin Customization

To change pin assignments, edit `main/audio_config.h`:

```c
// Shared clock signals
#define I2S_MCLK        GPIO_NUM_10  // Master clock
#define I2S_DAC_BCLK    GPIO_NUM_5   // Bit clock (shared)
#define I2S_DAC_WS      GPIO_NUM_6   // Word select (shared)

// Data signals
#define I2S_ADC_DIN     GPIO_NUM_4   // Data from ADC
#define I2S_DAC_DOUT    GPIO_NUM_7   // Data to DAC
```

**Note**: ESP32-C3 has limited GPIO pins. Choose pins that don't conflict with:
- UART/USB (GPIO18, GPIO19 on ESP32-C3)
- Strapping pins
- SPI flash pins

## Testing

1. Connect audio source to WM8782 input
2. Connect headphones or speaker to PCM5102A output
3. Flash the firmware and monitor logs
4. You should hear the input audio at the output with minimal latency
