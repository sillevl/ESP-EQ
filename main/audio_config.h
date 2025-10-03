#ifndef AUDIO_CONFIG_H
#define AUDIO_CONFIG_H

#include "driver/gpio.h"

// Audio Configuration
#define SAMPLE_RATE     48000  // Can be changed to 44100, 48000, 96000, 192000
#define I2S_NUM_CHANNELS 2
#define DMA_BUFFER_COUNT 4
#define DMA_BUFFER_SIZE  1024  // Number of samples per buffer

// Bit depth configuration
// WM8782 supports up to 24-bit
// PCM5102A supports up to 32-bit (but 24-bit is common)
#define AUDIO_BIT_DEPTH  24

// I2S Pin Definitions for ADC (WM8782) - Adjust to your hardware
// The WM8782 is a slave device, so ESP32 will be I2S master
#define I2S_ADC_BCLK    GPIO_NUM_26  // Bit clock
#define I2S_ADC_WS      GPIO_NUM_25  // Word select (LRCLK)
#define I2S_ADC_DIN     GPIO_NUM_22  // Data in

// I2S Pin Definitions for DAC (PCM5102A) - Adjust to your hardware
// The PCM5102A can be used in hardware mode or I2S mode
#define I2S_DAC_BCLK    GPIO_NUM_14  // Bit clock (BCK)
#define I2S_DAC_WS      GPIO_NUM_27  // Word select (LRCK)
#define I2S_DAC_DOUT    GPIO_NUM_12  // Data out (DIN on PCM5102A)

// Optional: PCM5102A control pins (if not hardwired)
// For hardware mode, tie these pins as follows:
// - SCK: Low (to GND) for I2S mode
// - FLT: Can be low (normal filter) or high (44.1kHz de-emphasis)
// - DEMP: Low for no de-emphasis
// - XSMT: High for normal operation (low for soft mute)
// - FMT: Low for I2S format, high for left-justified

#endif // AUDIO_CONFIG_H
