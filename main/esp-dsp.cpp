#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "audio_config.h"

static const char *TAG = "ESP-DSP";

// I2S Handles
static i2s_chan_handle_t tx_handle = NULL;  // DAC output
static i2s_chan_handle_t rx_handle = NULL;  // ADC input

// Audio buffer
static int32_t audio_buffer[DMA_BUFFER_SIZE];

/**
 * Initialize I2S channels for both ADC and DAC
 * ESP32 has one I2S peripheral that supports both TX and RX simultaneously
 * Both channels must be created in a single i2s_new_channel() call
 */
static esp_err_t init_i2s_channels(void)
{
    ESP_LOGI(TAG, "Initializing I2S channels...");
    
    // Configure I2S channel - create both TX and RX from same peripheral
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = DMA_BUFFER_COUNT;
    chan_cfg.dma_frame_num = DMA_BUFFER_SIZE;
    
    // Create both TX and RX channels in single call
    esp_err_t ret = i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channels: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure I2S standard mode for RX (ADC - WM8782)
    ESP_LOGI(TAG, "Configuring I2S RX for ADC (WM8782)...");
    i2s_std_config_t rx_std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_384,  // Must be multiple of 3 for 24-bit
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_24BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_ADC_BCLK,
            .ws = I2S_ADC_WS,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_ADC_DIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    
    ret = i2s_channel_init_std_mode(rx_handle, &rx_std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S RX: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure I2S standard mode for TX (DAC - PCM5102A)
    ESP_LOGI(TAG, "Configuring I2S TX for DAC (PCM5102A)...");
    i2s_std_config_t tx_std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_384,  // Must be multiple of 3 for 24-bit
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_24BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_DAC_BCLK,
            .ws = I2S_DAC_WS,
            .dout = I2S_DAC_DOUT,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    
    ret = i2s_channel_init_std_mode(tx_handle, &tx_std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S TX: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Enable RX channel
    ret = i2s_channel_enable(rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S RX: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Enable TX channel
    ret = i2s_channel_enable(tx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S TX: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "I2S channels initialized successfully");
    return ESP_OK;
}

/**
 * Audio processing task - Pass-through without processing
 */
static void audio_passthrough_task(void *pvParameters)
{
    size_t bytes_read = 0;
    size_t bytes_written = 0;
    
    ESP_LOGI(TAG, "Audio pass-through task started");
    
    while (1) {
        // Read from ADC (WM8782)
        esp_err_t ret = i2s_channel_read(rx_handle, audio_buffer, 
                                         sizeof(audio_buffer), &bytes_read, portMAX_DELAY);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S read error: %s", esp_err_to_name(ret));
            continue;
        }
        
        // Here you can add audio processing if needed
        // For now, we just pass through the audio data
        
        // Write to DAC (PCM5102A)
        ret = i2s_channel_write(tx_handle, audio_buffer, bytes_read, 
                               &bytes_written, portMAX_DELAY);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S write error: %s", esp_err_to_name(ret));
            continue;
        }
        
        // Optional: Monitor buffer underruns/overruns
        if (bytes_written != bytes_read) {
            ESP_LOGW(TAG, "Buffer size mismatch: read %d, written %d", 
                     bytes_read, bytes_written);
        }
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Audio Pass-Through Starting...");
    ESP_LOGI(TAG, "Sample Rate: %d Hz", SAMPLE_RATE);
    ESP_LOGI(TAG, "Channels: %d", I2S_NUM_CHANNELS);
    ESP_LOGI(TAG, "Buffer Size: %d samples", DMA_BUFFER_SIZE);
    
    // Initialize I2S channels (both ADC and DAC)
    esp_err_t ret = init_i2s_channels();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S channels");
        return;
    }
    
    // Create audio processing task
    xTaskCreate(audio_passthrough_task, "audio_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Audio pass-through initialized successfully");
}
