#ifndef AUDIO_CONFIG_H
#define AUDIO_CONFIG_H

#include "driver/gpio.h"

// Audio Configuration
#define SAMPLE_RATE     48000  // Can be changed to 44100, 48000, 96000, 192000
#define I2S_NUM_CHANNELS 2
#define DMA_BUFFER_COUNT 8     // Number of DMA descriptors for better buffering
#define DMA_BUFFER_SIZE  480   // Number of samples per buffer (max 1024/4 = 256, but 512 works)

// Bit depth configuration
// WM8782 supports up to 24-bit
// PCM5102A supports up to 32-bit (but 24-bit is common)
#define AUDIO_BIT_DEPTH  24

// I2S Pin Definitions - SHARED CLOCK DOMAIN
// DAC (PCM5102A) is the MASTER - generates clocks
#define I2S_MCLK        GPIO_NUM_10  // Master clock output - connect to WM8782 MCLK
#define I2S_DAC_BCLK    GPIO_NUM_5   // Bit clock (BCK) - MASTER OUTPUT
#define I2S_DAC_WS      GPIO_NUM_6   // Word select (LRCK) - MASTER OUTPUT
#define I2S_DAC_DOUT    GPIO_NUM_7   // Data out (DIN on PCM5102A)

// ADC (WM8782) is SLAVE - receives clocks from DAC
#define I2S_ADC_DIN     GPIO_NUM_4   // Data in from WM8782

#endif // AUDIO_CONFIG_H
