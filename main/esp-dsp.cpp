#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "audio_config.h"
#include "equalizer.h"

static const char *TAG = "ESP-DSP";

// Equalizer instance
static equalizer_t equalizer;

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
    
    // Verify handles were created
    ESP_LOGI(TAG, "I2S channels created - TX: %p, RX: %p", tx_handle, rx_handle);
    
    if (tx_handle == NULL || rx_handle == NULL) {
        ESP_LOGE(TAG, "I2S handle is NULL after creation!");
        return ESP_ERR_INVALID_STATE;
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

// Structure to pass handles to task
typedef struct {
    i2s_chan_handle_t tx_handle;
    i2s_chan_handle_t rx_handle;
} audio_handles_t;

/**
 * Audio processing task - Pass-through with equalizer
 */
static void audio_passthrough_task(void *pvParameters)
{
    // Get handles from parameters
    audio_handles_t *handles = (audio_handles_t *)pvParameters;
    i2s_chan_handle_t local_tx_handle = handles->tx_handle;
    i2s_chan_handle_t local_rx_handle = handles->rx_handle;
    
    // Free the parameter structure (we've copied the handles)
    free(handles);
    
    size_t bytes_read = 0;
    size_t bytes_written = 0;
    
    ESP_LOGI(TAG, "Audio pass-through task started");
    ESP_LOGI(TAG, "Local handles - TX: %p, RX: %p", local_tx_handle, local_rx_handle);
    
    // Safety check
    if (local_rx_handle == NULL || local_tx_handle == NULL) {
        ESP_LOGE(TAG, "I2S handles are NULL! Cannot process audio.");
        vTaskDelete(NULL);
        return;
    }
    
    // Add small delay to ensure I2S is fully initialized
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Starting audio processing loop...");
    
    while (1) {
        // Read from ADC (WM8782)
        esp_err_t ret = i2s_channel_read(local_rx_handle, audio_buffer, 
                                         sizeof(audio_buffer), &bytes_read, portMAX_DELAY);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S read error: %s (handle=%p)", esp_err_to_name(ret), local_rx_handle);
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        
        // Process audio through equalizer
        int num_samples = bytes_read / sizeof(int32_t);
        equalizer_process(&equalizer, audio_buffer, num_samples);
        
        // Write to DAC (PCM5102A)
        ret = i2s_channel_write(local_tx_handle, audio_buffer, bytes_read, 
                               &bytes_written, portMAX_DELAY);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S write error: %s (handle=%p)", esp_err_to_name(ret), local_tx_handle);
            vTaskDelay(pdMS_TO_TICKS(10));
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
    ESP_LOGI(TAG, "ESP32 Audio DSP with 5-Band Equalizer Starting...");
    ESP_LOGI(TAG, "Sample Rate: %d Hz", SAMPLE_RATE);
    ESP_LOGI(TAG, "Channels: %d", I2S_NUM_CHANNELS);
    ESP_LOGI(TAG, "Buffer Size: %d samples", DMA_BUFFER_SIZE);
    
    // Initialize equalizer
    ESP_LOGI(TAG, "Initializing 5-band equalizer...");
    equalizer_init(&equalizer, SAMPLE_RATE);
    
    // Set example EQ curve (customize these values as needed)
    // Band 1: 60Hz (Sub-bass) - boost by 3dB
    equalizer_set_band_gain(&equalizer, 0, 3.0f, SAMPLE_RATE);
    // Band 2: 250Hz (Bass) - boost by 2dB
    equalizer_set_band_gain(&equalizer, 1, 2.0f, SAMPLE_RATE);
    // Band 3: 1kHz (Mid) - neutral (0dB)
    equalizer_set_band_gain(&equalizer, 2, 0.0f, SAMPLE_RATE);
    // Band 4: 4kHz (Upper mid) - boost by 1dB
    equalizer_set_band_gain(&equalizer, 3, 1.0f, SAMPLE_RATE);
    // Band 5: 12kHz (Treble) - boost by 4dB
    equalizer_set_band_gain(&equalizer, 4, 4.0f, SAMPLE_RATE);
    
    ESP_LOGI(TAG, "EQ Settings:");
    ESP_LOGI(TAG, "  60Hz:   %.1f dB", equalizer.gain_db[0]);
    ESP_LOGI(TAG, "  250Hz:  %.1f dB", equalizer.gain_db[1]);
    ESP_LOGI(TAG, "  1kHz:   %.1f dB", equalizer.gain_db[2]);
    ESP_LOGI(TAG, "  4kHz:   %.1f dB", equalizer.gain_db[3]);
    ESP_LOGI(TAG, "  12kHz:  %.1f dB", equalizer.gain_db[4]);
    
    // Initialize I2S channels (both ADC and DAC)
    esp_err_t ret = init_i2s_channels();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S channels");
        return;
    }
    
    // Verify handles are valid before creating task
    ESP_LOGI(TAG, "Verifying I2S handles before task creation - TX: %p, RX: %p", tx_handle, rx_handle);
    
    if (tx_handle == NULL || rx_handle == NULL) {
        ESP_LOGE(TAG, "I2S handles are NULL after initialization!");
        return;
    }
    
    // Allocate structure to pass handles to task
    audio_handles_t *handles = (audio_handles_t *)malloc(sizeof(audio_handles_t));
    if (handles == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for task parameters");
        return;
    }
    
    handles->tx_handle = tx_handle;
    handles->rx_handle = rx_handle;
    
    // Create audio processing task with handles as parameter
    BaseType_t task_created = xTaskCreate(audio_passthrough_task, "audio_task", 4096, handles, 5, NULL);
    
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio task");
        free(handles);
        return;
    }
    
    ESP_LOGI(TAG, "Audio DSP with equalizer initialized successfully");
}
