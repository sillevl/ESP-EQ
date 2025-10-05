#include <stdio.h>
#include <cstring>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_err.h"
#include "audio_config.h"
#include "esp_pm.h"
#include "subsonic.h"
#include "equalizer.h"
#include "limiter.h"
#include "serial_commands.h"
#include "nvs_flash.h"

static const char *TAG = "ESP-DSP";

// I2S Handles - both on same peripheral
static i2s_chan_handle_t rx_handle = NULL;
static i2s_chan_handle_t tx_handle = NULL;

// Global instances accessible to serial_commands
subsonic_t subsonic;    // Subsonic/DC protection filter
equalizer_t equalizer;  // Changed from 'eq' to 'equalizer' and made non-static
limiter_t limiter;      // True-peak limiter for clipping prevention

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
            .ext_clk_freq_hz = 0, // No external clock
            .mclk_multiple = I2S_MCLK_MULTIPLE_384,  // Still set for internal calculation
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
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

        for(int i = 0; i < num_samples; i++) {
            audio_buffer[i] = audio_buffer[i] >> 8;
            // audio_buffer[i] = __builtin_bswap32(audio_buffer[i]);
        }

        // Apply subsonic filter first (DC/subsonic protection)
        subsonic_process(&subsonic, audio_buffer, num_samples);
        
        equalizer_process(&equalizer, audio_buffer, num_samples);
        
        // Apply limiter after EQ to prevent clipping
        limiter_process(&limiter, audio_buffer, num_samples);

        // // Debug: Log first sample before and after EQ (only occasionally)
        // static int debug_counter = 0;
        // if (debug_counter == 0) {

        //     ESP_LOGI(TAG, "[ 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X ]", 
        //         ((unsigned int)audio_buffer[0]), ((unsigned int)audio_buffer[2]), ((unsigned int)audio_buffer[4]), ((unsigned int)audio_buffer[6]), ((unsigned int)audio_buffer[8]),
        //         ((unsigned int)audio_buffer[10]), ((unsigned int)audio_buffer[12]), ((unsigned int)audio_buffer[14]), ((unsigned int)audio_buffer[16]), ((unsigned int)audio_buffer[18]) );
        // }

        
        // debug_counter = (debug_counter + 1) % 10;  // Log every 100th buffer
                

        for(int i = 0; i < num_samples; i++) {
            audio_buffer[i] = audio_buffer[i] << 8;
            // audio_buffer[i] = __builtin_bswap32(audio_buffer[i]);
        }

        // Write to DAC
        ret = i2s_channel_write(tx_handle, audio_buffer, bytes_read, &bytes_written, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S write error: %s", esp_err_to_name(ret));
        }
        

    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Audio Pass-Through Starting...");
    ESP_LOGI(TAG, "Sample Rate: %d Hz", SAMPLE_RATE);
    ESP_LOGI(TAG, "Buffer Size: %d samples", DMA_BUFFER_SIZE);
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_LOGI(TAG, "Erasing NVS flash...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");
    
    // Power management configuration
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 240,
        .light_sleep_enable = false
    };
    esp_pm_configure(&pm_config);
    
    // Initialize I2S
    ret = init_i2s();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S");
        return;
    }

    // Initialize subsonic filter with defaults
    subsonic_init(&subsonic, SAMPLE_RATE);
    
    // Try to load saved subsonic settings from flash
    ret = subsonic_load_settings(&subsonic, SAMPLE_RATE);
    if (ret != ESP_OK) {
        // No saved settings, use defaults
        subsonic_set_enabled(&subsonic, true);
        ESP_LOGI(TAG, "Using default subsonic filter settings");
    }
    
    // Initialize equalizer with defaults
    equalizer_init(&equalizer, SAMPLE_RATE);
    
    // Try to load saved settings from flash
    ret = equalizer_load_settings(&equalizer, SAMPLE_RATE);
    if (ret != ESP_OK) {
        // No saved settings, use defaults
        equalizer_set_enabled(&equalizer, true);
        ESP_LOGI(TAG, "Using default equalizer settings");
    }
    
    // Initialize limiter with defaults
    limiter_init(&limiter, SAMPLE_RATE);
    
    // Try to load saved limiter settings from flash
    ret = limiter_load_settings(&limiter, SAMPLE_RATE);
    if (ret != ESP_OK) {
        // No saved settings, use defaults
        limiter_set_enabled(&limiter, true);
        ESP_LOGI(TAG, "Using default limiter settings");
    }
    
    // Initialize serial command interface
    serial_commands_init();
    ESP_LOGI(TAG, "Serial command interface started");
    
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
