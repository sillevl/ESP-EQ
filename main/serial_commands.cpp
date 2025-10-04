#include "serial_commands.h"
#include "equalizer.h"
#include "audio_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "CMD";

// External references
extern equalizer_t equalizer;

// VU meter state (add near the top with other external variables)
static bool vu_meter_enabled = true;

void serial_commands_print_help(void)
{
    printf("\n");
    printf("=====================================\n");
    printf("  ESP32 DSP - Serial Commands\n");
    printf("=====================================\n");
    printf("\n");
    printf("System Commands:\n");
    printf("  help          - Show this help message\n");
    printf("  status        - Show system status\n");
    printf("\n");
    printf("Equalizer Commands:\n");
    printf("  eq show       - Display current EQ settings\n");
    printf("  eq set <band> <gain>\n");
    printf("                - Set band gain (band: 0-4, gain: -12 to +12 dB)\n");
    printf("                  Bands: 0=60Hz, 1=250Hz, 2=1kHz, 3=4kHz, 4=12kHz\n");
    printf("  eq enable     - Enable equalizer processing\n");
    printf("  eq disable    - Disable equalizer (bypass)\n");
    printf("  eq reset      - Reset EQ filter state\n");
    printf("  eq preset <name>\n");
    printf("                - Load EQ preset (flat, bass, vocal, rock, jazz)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  eq set 0 6.0  - Boost 60Hz by 6dB\n");
    printf("\n");
}

static void apply_preset(const char* preset_name)
{
    if (strcmp(preset_name, "flat") == 0) {
        // Flat - all bands at 0dB
        for (int i = 0; i < 5; i++) {
            equalizer_set_band_gain(&equalizer, i, 0.0f, SAMPLE_RATE);
        }
        printf("Applied 'Flat' preset (all bands at 0dB)\n");
    }
    else if (strcmp(preset_name, "bass") == 0) {
        // Bass boost
        equalizer_set_band_gain(&equalizer, 0, 6.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 1, 4.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 2, 0.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 3, 0.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 4, 0.0f, SAMPLE_RATE);
        printf("Applied 'Bass Boost' preset\n");
    }
    else if (strcmp(preset_name, "vocal") == 0) {
        // Vocal clarity
        equalizer_set_band_gain(&equalizer, 0, -2.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 1, 0.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 2, 3.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 3, 5.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 4, 2.0f, SAMPLE_RATE);
        printf("Applied 'Vocal Clarity' preset\n");
    }
    else if (strcmp(preset_name, "rock") == 0) {
        // Rock - V-shaped
        equalizer_set_band_gain(&equalizer, 0, 5.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 1, 3.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 2, -4.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 3, 2.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 4, 6.0f, SAMPLE_RATE);
        printf("Applied 'Rock' preset\n");
    }
    else if (strcmp(preset_name, "jazz") == 0) {
        // Jazz - natural with slight warmth
        equalizer_set_band_gain(&equalizer, 0, 2.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 1, 1.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 2, 0.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 3, 1.0f, SAMPLE_RATE);
        equalizer_set_band_gain(&equalizer, 4, 3.0f, SAMPLE_RATE);
        printf("Applied 'Jazz' preset\n");
    }
    else {
        printf("Unknown preset: %s\n", preset_name);
        printf("Available presets: flat, bass, vocal, rock, jazz\n");
        return;
    }
    
    // Show new settings
    printf("New EQ settings:\n");
    printf("  60Hz:   %.1f dB\n", equalizer.gain_db[0]);
    printf("  250Hz:  %.1f dB\n", equalizer.gain_db[1]);
    printf("  1kHz:   %.1f dB\n", equalizer.gain_db[2]);
    printf("  4kHz:   %.1f dB\n", equalizer.gain_db[3]);
    printf("  12kHz:  %.1f dB\n", equalizer.gain_db[4]);
}

static void show_eq_settings(void)
{
    const char* band_names[] = {"60Hz", "250Hz", "1kHz", "4kHz", "12kHz"};
    
    printf("\n");
    printf("Equalizer Settings:\n");
    printf("  Status: %s\n", equalizer.enabled ? "ENABLED" : "DISABLED (bypass)");
    printf("\n");
    printf("  Band  | Frequency | Gain\n");
    printf("  ------|-----------|--------\n");
    for (int i = 0; i < 5; i++) {
        printf("    %d   | %-9s | %+.1f dB\n", i, band_names[i], equalizer.gain_db[i]);
    }
    printf("\n");
}

static void show_system_status(void)
{
    printf("\n");
    printf("System Status:\n");
    printf("  Sample Rate: %d Hz\n", SAMPLE_RATE);
    printf("  Channels: %d (Stereo)\n", I2S_NUM_CHANNELS);
    printf("  Buffer Size: %d samples\n", DMA_BUFFER_SIZE);
    printf("  Bit Depth: 24-bit\n");
    printf("\n");
    printf("  Free Heap: %d bytes\n", (int) esp_get_free_heap_size());
    printf("  Min Free Heap: %d bytes\n", (int) esp_get_minimum_free_heap_size());
    printf("\n");
}


static void process_command(char* cmd)
{
    // Remove trailing newline/carriage return
    char* p = cmd;
    while (*p) {
        if (*p == '\n' || *p == '\r') {
            *p = '\0';
            break;
        }
        p++;
    }
    
    // Skip empty commands
    if (strlen(cmd) == 0) {
        return;
    }
    
    // Tokenize command
    char* token = strtok(cmd, " ");
    if (token == NULL) {
        return;
    }
    
    // Process commands
    if (strcmp(token, "help") == 0) {
        serial_commands_print_help();
    }
    else if (strcmp(token, "status") == 0) {
        show_system_status();
    }
    else if (strcmp(token, "eq") == 0) {
        token = strtok(NULL, " ");
        if (token == NULL) {
            printf("Error: EQ command requires subcommand\n");
            printf("Try: eq show, eq set, eq enable, eq disable, eq reset, eq preset\n");
            return;
        }
        
        if (strcmp(token, "show") == 0) {
            show_eq_settings();
        }
        else if (strcmp(token, "set") == 0) {
            char* band_str = strtok(NULL, " ");
            char* gain_str = strtok(NULL, " ");
            
            if (band_str == NULL || gain_str == NULL) {
                printf("Error: Usage: eq set <band> <gain>\n");
                printf("Example: eq set 0 6.0\n");
                return;
            }
            
            int band = atoi(band_str);
            float gain = atof(gain_str);
            
            if (band < 0 || band >= 5) {
                printf("Error: Band must be 0-4 (0=60Hz, 1=250Hz, 2=1kHz, 3=4kHz, 4=12kHz)\n");
                return;
            }
            
            if (gain < -12.0f || gain > 12.0f) {
                printf("Warning: Gain clamped to range -12.0 to +12.0 dB\n");
            }
            
            bool success = equalizer_set_band_gain(&equalizer, band, gain, SAMPLE_RATE);
            if (success) {
                const char* band_names[] = {"60Hz", "250Hz", "1kHz", "4kHz", "12kHz"};
                printf("Set %s (band %d) to %.1f dB\n", band_names[band], band, equalizer.gain_db[band]);
            } else {
                printf("Error: Failed to set band gain\n");
            }
        }
        else if (strcmp(token, "enable") == 0) {
            equalizer_set_enabled(&equalizer, true);
            printf("Equalizer enabled\n");
        }
        else if (strcmp(token, "disable") == 0) {
            equalizer_set_enabled(&equalizer, false);
            printf("Equalizer disabled (bypass mode)\n");
        }
        else if (strcmp(token, "reset") == 0) {
            equalizer_reset(&equalizer);
            printf("Equalizer state reset (filter history cleared)\n");
        }
        else if (strcmp(token, "preset") == 0) {
            char* preset_name = strtok(NULL, " ");
            if (preset_name == NULL) {
                printf("Error: Usage: eq preset <name>\n");
                printf("Available: flat, bass, vocal, rock, jazz\n");
                return;
            }
            apply_preset(preset_name);
        }
        else {
            printf("Unknown EQ subcommand: %s\n", token);
            printf("Try: eq show, eq set, eq enable, eq disable, eq reset, eq preset\n");
        }
    }
    else {
        printf("Unknown command: %s\n", token);
        printf("Type 'help' for available commands\n");
    }
}

static void serial_command_task(void *pvParameters)
{
    char cmd_buffer[128];
    int cmd_index = 0;
    
    ESP_LOGI(TAG, "Serial command interface started");
    printf("\n");
    printf("=====================================\n");
    printf("  ESP32 DSP - Ready\n");
    printf("=====================================\n");
    printf("Type 'help' for available commands\n");
    printf("\n> ");
    fflush(stdout);
    
    while (1) {
        int c = fgetc(stdin);
        
        if (c == EOF) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        
        // Echo character
        fputc(c, stdout);
        fflush(stdout);
        
        if (c == '\n' || c == '\r') {
            cmd_buffer[cmd_index] = '\0';
            
            // Process command
            if (cmd_index > 0) {
                printf("\n");
                process_command(cmd_buffer);
            }
            
            // Reset buffer and show prompt
            cmd_index = 0;
            printf("> ");
            fflush(stdout);
        }
        else if (c == '\b' || c == 127) {  // Backspace
            if (cmd_index > 0) {
                cmd_index--;
                printf(" \b");  // Erase character
                fflush(stdout);
            }
        }
        else if (cmd_index < sizeof(cmd_buffer) - 1) {
            cmd_buffer[cmd_index++] = (char)c;
        }
    }
}

void serial_commands_init(void)
{
    // Configure stdin to be non-blocking
    setvbuf(stdin, NULL, _IONBF, 0);
    
    // Create command processing task
    xTaskCreate(serial_command_task, "serial_cmd", 4096, NULL, 4, NULL);
}

// Add function to check VU meter state
bool is_vu_meter_enabled(void)
{
    return vu_meter_enabled;
}
