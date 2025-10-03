#include <stdio.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_err.h"
#include "audio_config.h"
#include "esp_pm.h"

#define DMA_BUFFER_SIZE 480  // Changed from 512 to 480 (divisible by many factors)
#define DMA_BUFFER_COUNT 8

static const char *TAG = "ESP-DSP";

// I2S Handles - both on same peripheral
static i2s_chan_handle_t rx_handle = NULL;
static i2s_chan_handle_t tx_handle = NULL;

// Audio buffer
static int32_t audio_buffer[DMA_BUFFER_SIZE];

/**
 * Initialize I2S channels for ADC and DAC on same peripheral
 */
static esp_err_t init_i2s(void)
{
    ESP_LOGI(TAG, "Initializing I2S channels...");
    
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = DMA_BUFFER_COUNT;
    chan_cfg.dma_frame_num = DMA_BUFFER_SIZE;
    chan_cfg.auto_clear = true;
    
    esp_err_t ret = i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channels: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "I2S channels created - RX: %p, TX: %p", rx_handle, tx_handle);
    
    // Configure RX channel without MCLK
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_384,  // Still set for internal calculation
        },
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_24BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK, 
            .bclk = I2S_DAC_BCLK,
            .ws = I2S_DAC_WS,
            .dout = I2S_DAC_DOUT,
            .din = I2S_ADC_DIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    
    // Initialize TX channel
    ret = i2s_channel_init_std_mode(tx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S TX: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize RX channel with SAME config (clocks already shared)
    ret = i2s_channel_init_std_mode(rx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S RX: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Enable TX first, then RX
    ret = i2s_channel_enable(tx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S TX: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = i2s_channel_enable(rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S RX: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "I2S initialized successfully with shared clock domain and MCLK");
    ESP_LOGI(TAG, "MCLK: %d Hz (48kHz * 384 = 18.432MHz)", SAMPLE_RATE * 384);
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
    
    // Longer delay for clock stabilization
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Pre-fill TX buffer with silence
    memset(audio_buffer, 0, sizeof(audio_buffer));
    for (int i = 0; i < 4; i++) {
        i2s_channel_write(tx_handle, audio_buffer, sizeof(audio_buffer), &bytes_written, portMAX_DELAY);
    }
    ESP_LOGI(TAG, "TX buffer pre-filled");
    
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
        
        // Process samples
        for (int i = 0; i < num_samples; i += 2) {
            int32_t left = audio_buffer[i];
            int32_t right = audio_buffer[i + 1];
            
            // Extract 24-bit values for monitoring
            int32_t left_24bit = left >> 8;
            int32_t right_24bit = right >> 8;
            
            // Update statistics
            if (left_24bit > max_value) max_value = left_24bit;
            if (left_24bit < min_value) min_value = left_24bit;
            sum += left_24bit;
            
            if (right_24bit > max_value) max_value = right_24bit;
            if (right_24bit < min_value) min_value = right_24bit;
            sum += right_24bit;
        }
        
        // Write to DAC
        ret = i2s_channel_write(tx_handle, audio_buffer, bytes_read, &bytes_written, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S write error: %s", esp_err_to_name(ret));
        }
        
        // Report every 50 frames
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
    
    // Power management configuration
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 240,
        .light_sleep_enable = false
    };
    esp_pm_configure(&pm_config);
    
    // Initialize I2S
    esp_err_t ret = init_i2s();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S");
        return;
    }
    
    // Create audio task with high priority
    BaseType_t task_created = xTaskCreatePinnedToCore(
        audio_task, 
        "audio_task", 
        4096, 
        NULL, 
        configMAX_PRIORITIES - 1,
        NULL,
        0
    );
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio task");
        return;
    }
    
    ESP_LOGI(TAG, "Audio pass-through initialized");
    ESP_LOGI(TAG, "Connect audio source to ADC and speakers to DAC");
}
