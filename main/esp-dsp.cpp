#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_err.h"
#include "audio_config.h"

static const char *TAG = "ESP-DSP";

// I2S Handles
static i2s_chan_handle_t rx_handle = NULL;
static i2s_chan_handle_t tx_handle = NULL;

// Audio buffer
static int32_t audio_buffer[DMA_BUFFER_SIZE];

/**
 * Initialize I2S channels for ADC and DAC
 */
static esp_err_t init_i2s(void)
{
    ESP_LOGI(TAG, "Initializing I2S channels...");
    
    // Configure I2S channels
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = DMA_BUFFER_COUNT;
    chan_cfg.dma_frame_num = DMA_BUFFER_SIZE;
    
    // Create RX and TX channels
    esp_err_t ret = i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channels: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "I2S channels created - RX: %p, TX: %p", rx_handle, tx_handle);
    
    // Configure RX (ADC - WM8782) - MSB format
    i2s_std_config_t rx_std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_384,
        },
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_24BIT, I2S_SLOT_MODE_STEREO),
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
    
    // Configure TX (DAC - PCM5102A) - MSB format (not Philips!)
    i2s_std_config_t tx_std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_384,
        },
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_24BIT, I2S_SLOT_MODE_STEREO),
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
    
    // Enable channels
    ret = i2s_channel_enable(rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S RX: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = i2s_channel_enable(tx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S TX: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "I2S initialized successfully");
    return ESP_OK;
}

/**
 * Audio pass-through task with monitoring
 */
static void audio_task(void *pvParameters)
{
    size_t bytes_read = 0;
    size_t bytes_written = 0;
    int frame_count = 0;
    
    ESP_LOGI(TAG, "Audio pass-through task started");
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    while (1) {
        // Read from ADC
        esp_err_t ret = i2s_channel_read(rx_handle, audio_buffer, 
                                         sizeof(audio_buffer), &bytes_read, portMAX_DELAY);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S read error: %s", esp_err_to_name(ret));
            continue;
        }
        
        int num_samples = bytes_read / sizeof(int32_t);
        
        // Monitor audio levels
        int32_t max_value = INT32_MIN;
        int32_t min_value = INT32_MAX;
        int64_t sum = 0;
        
        // No conversion needed - both use MSB format
        for (int i = 0; i < num_samples; i++) {
            int32_t sample = audio_buffer[i];
            
            // Extract 24-bit value for monitoring
            int32_t sample_24bit = sample >> 8;
            
            // Update statistics
            if (sample_24bit > max_value) max_value = sample_24bit;
            if (sample_24bit < min_value) min_value = sample_24bit;
            sum += sample_24bit;
            
            // // Log first 10 samples in the first frame
            // if (frame_count == 0 && i < 10) {
            //     ESP_LOGI(TAG, "Sample[%d]: Raw=0x%08lX, 24bit=%ld", i, sample, sample_24bit);
            // }
        }
        
        // Write to DAC - direct pass-through, no conversion
        ret = i2s_channel_write(tx_handle, audio_buffer, bytes_read, &bytes_written, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S write error: %s", esp_err_to_name(ret));
        }
        
        // Report every 50 frames (~1 second)
        frame_count++;
        if (frame_count >= 50) {
            int32_t dc_offset = sum / num_samples;
            int32_t peak_to_peak = max_value - min_value;
            
            ESP_LOGI(TAG, "24bit - DC: %7ld, P2P: %8ld, Min: %8ld, Max: %8ld", 
                     dc_offset, peak_to_peak, min_value, max_value);
            
            frame_count = 0;
        }
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Audio Pass-Through Starting...");
    ESP_LOGI(TAG, "Sample Rate: %d Hz", SAMPLE_RATE);
    ESP_LOGI(TAG, "Buffer Size: %d samples", DMA_BUFFER_SIZE);
    
    // Initialize I2S
    esp_err_t ret = init_i2s();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S");
        return;
    }
    
    // Create audio task
    BaseType_t task_created = xTaskCreate(audio_task, "audio_task", 4096, NULL, 5, NULL);
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio task");
        return;
    }
    
    ESP_LOGI(TAG, "Audio pass-through initialized");
    ESP_LOGI(TAG, "Connect audio source to ADC and speakers to DAC");
}
