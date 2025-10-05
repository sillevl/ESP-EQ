#include "serial_commands.h"
#include "subsonic.h"
#include "pregain.h"
#include "equalizer.h"
#include "limiter.h"
#include "audio_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "CMD";

// External references
extern subsonic_t subsonic;
extern pregain_t pregain;
extern equalizer_t equalizer;
extern limiter_t limiter;

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
    printf("Subsonic Filter Commands:\n");
    printf("  sub show      - Display current subsonic filter settings\n");
    printf("  sub freq <hz> - Set cutoff frequency (15-50 Hz, default 25)\n");
    printf("  sub enable    - Enable subsonic filter (DC protection)\n");
    printf("  sub disable   - Disable subsonic filter (bypass)\n");
    printf("  sub reset     - Reset filter state\n");
    printf("  sub save      - Manually save settings to flash\n");
    printf("\n");
    printf("Pre-Gain Commands:\n");
    printf("  gain show     - Display current pre-gain settings\n");
    printf("  gain set <db> - Set pre-gain (-12 to +12 dB, default 0)\n");
    printf("                  (Settings are automatically saved to flash)\n");
    printf("  gain enable   - Enable pre-gain processing\n");
    printf("  gain disable  - Disable pre-gain (bypass)\n");
    printf("  gain save     - Manually save settings to flash\n");
    printf("\n");
    printf("Equalizer Commands:\n");
    printf("  eq show       - Display current EQ settings\n");
    printf("  eq set <band> <gain>\n");
    printf("                - Set band gain (band: 0-4, gain: -12 to +12 dB)\n");
    printf("                  Bands: 0=60Hz, 1=250Hz, 2=1kHz, 3=4kHz, 4=12kHz\n");
    printf("                  (Settings are automatically saved to flash)\n");
    printf("  eq enable     - Enable equalizer processing\n");
    printf("  eq disable    - Disable equalizer (bypass)\n");
    printf("  eq reset      - Reset EQ filter state (temporary)\n");
    printf("  eq preset <name>\n");
    printf("                - Load EQ preset (flat, bass, vocal, rock, jazz)\n");
    printf("  eq save       - Manually save current settings to flash\n");
    printf("\n");
    printf("Limiter Commands:\n");
    printf("  lim show      - Display current limiter settings\n");
    printf("  lim threshold <db>\n");
    printf("                - Set limiter threshold (-12 to 0 dB, default -0.5)\n");
    printf("  lim enable    - Enable limiter (clipping protection)\n");
    printf("  lim disable   - Disable limiter (bypass)\n");
    printf("  lim reset     - Reset limiter state\n");
    printf("  lim stats     - Show limiter statistics\n");
    printf("  lim save      - Manually save limiter settings to flash\n");
    printf("\n");
    printf("Examples:\n");
    printf("  sub freq 28.0  - Set subsonic cutoff to 28Hz\n");
    printf("  gain set 3.0   - Apply 3dB pre-gain\n");
    printf("  eq set 0 6.0   - Boost 60Hz by 6dB\n");
    printf("  lim threshold -1.0 - Set limiter threshold to -1dB\n");
    printf("\n");
    printf("Note: Settings are saved to flash and restored at boot.\n");
    printf("Processing order: Subsonic → Pre-Gain → Equalizer → Limiter\n");
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

static void show_pregain_settings(void)
{
    printf("\n");
    printf("Pre-Gain Settings:\n");
    printf("  Status: %s\n", pregain_is_enabled(&pregain) ? "ENABLED" : "DISABLED (bypass)");
    printf("  Gain: %+.1f dB (%.3fx linear)\n", pregain_get_gain(&pregain), pregain.gain_linear);
    printf("\n");
}

static void show_limiter_settings(void)
{
    printf("\n");
    printf("Limiter Settings:\n");
    printf("  Status: %s\n", limiter.enabled ? "ENABLED" : "DISABLED (bypass)");
    printf("  Threshold: %.1f dB\n", limiter_get_threshold(&limiter));
    printf("  Attack: %.1f ms\n", LIMITER_ATTACK_MS);
    printf("  Release: %.1f ms\n", LIMITER_RELEASE_MS);
    printf("  Lookahead: %.1f ms\n", LIMITER_LOOKAHEAD_MS);
    printf("\n");
}

static void show_limiter_stats(void)
{
    printf("\n");
    printf("Limiter Statistics:\n");
    printf("  Peak Reduction: %.2f dB\n", limiter_get_peak_reduction(&limiter));
    printf("  Clips Prevented: %u\n", (unsigned int)limiter_get_clips_prevented(&limiter));
    printf("\n");
}

static void show_subsonic_settings(void)
{
    printf("\n");
    printf("Subsonic Filter Settings:\n");
    printf("  Status: %s\n", subsonic_get_enabled(&subsonic) ? "ENABLED" : "DISABLED (bypass)");
    printf("  Type: 2nd-order high-pass Butterworth\n");
    printf("  Cutoff Frequency: %.1f Hz\n", subsonic_get_frequency(&subsonic));
    printf("  Q Factor: %.3f\n", SUBSONIC_Q);
    printf("  Purpose: DC blocking and subsonic protection\n");
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
    printf("DSP Processing Chain:\n");
    printf("  1. Subsonic Filter: %s (%.1f Hz HPF)\n", 
           subsonic_get_enabled(&subsonic) ? "ON" : "OFF",
           subsonic_get_frequency(&subsonic));
    printf("  2. Pre-Gain: %s (%+.1f dB)\n", 
           pregain_is_enabled(&pregain) ? "ON" : "OFF",
           pregain_get_gain(&pregain));
    printf("  3. Equalizer: %s (5-band)\n", 
           equalizer.enabled ? "ON" : "OFF");
    printf("  4. Limiter: %s (%.1f dB)\n", 
           limiter.enabled ? "ON" : "OFF",
           limiter_get_threshold(&limiter));
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
            printf("Try: eq show, eq set, eq pregain, eq enable, eq disable, eq reset, eq preset, eq save\n");
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
                
                // Save settings to flash
                esp_err_t err = equalizer_save_settings(&equalizer);
                if (err != ESP_OK) {
                    printf("Warning: Failed to save settings to flash\n");
                }
            } else {
                printf("Error: Failed to set band gain\n");
            }
        }
        else if (strcmp(token, "enable") == 0) {
            equalizer_set_enabled(&equalizer, true);
            printf("Equalizer enabled\n");
            
            // Save settings to flash
            esp_err_t err = equalizer_save_settings(&equalizer);
            if (err != ESP_OK) {
                printf("Warning: Failed to save settings to flash\n");
            }
        }
        else if (strcmp(token, "disable") == 0) {
            equalizer_set_enabled(&equalizer, false);
            printf("Equalizer disabled (bypass mode)\n");
            
            // Save settings to flash
            esp_err_t err = equalizer_save_settings(&equalizer);
            if (err != ESP_OK) {
                printf("Warning: Failed to save settings to flash\n");
            }
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
            
            // Save settings to flash
            esp_err_t err = equalizer_save_settings(&equalizer);
            if (err != ESP_OK) {
                printf("Warning: Failed to save settings to flash\n");
            }
        }
        else if (strcmp(token, "save") == 0) {
            esp_err_t err = equalizer_save_settings(&equalizer);
            if (err == ESP_OK) {
                printf("Equalizer settings saved to flash successfully\n");
            } else {
                printf("Error: Failed to save settings to flash: %s\n", esp_err_to_name(err));
            }
        }
        else {
            printf("Unknown EQ subcommand: %s\n", token);
            printf("Try: eq show, eq set, eq enable, eq disable, eq reset, eq preset, eq save\n");
        }
    }
    else if (strcmp(token, "lim") == 0 || strcmp(token, "limiter") == 0) {
        token = strtok(NULL, " ");
        if (token == NULL) {
            printf("Error: Limiter command requires subcommand\n");
            printf("Try: lim show, lim threshold, lim enable, lim disable, lim reset, lim stats, lim save\n");
            return;
        }
        
        if (strcmp(token, "show") == 0) {
            show_limiter_settings();
        }
        else if (strcmp(token, "threshold") == 0) {
            char* threshold_str = strtok(NULL, " ");
            
            if (threshold_str == NULL) {
                printf("Error: Usage: lim threshold <db>\n");
                printf("Example: lim threshold -1.0\n");
                return;
            }
            
            float threshold = atof(threshold_str);
            
            if (threshold < -12.0f || threshold > 0.0f) {
                printf("Warning: Threshold clamped to range -12.0 to 0.0 dB\n");
            }
            
            bool success = limiter_set_threshold(&limiter, threshold);
            if (success) {
                printf("Set limiter threshold to %.1f dB\n", limiter_get_threshold(&limiter));
                
                // Save settings to flash
                esp_err_t err = limiter_save_settings(&limiter);
                if (err != ESP_OK) {
                    printf("Warning: Failed to save settings to flash\n");
                }
            } else {
                printf("Error: Failed to set threshold\n");
            }
        }
        else if (strcmp(token, "enable") == 0) {
            limiter_set_enabled(&limiter, true);
            printf("Limiter enabled\n");
            
            // Save settings to flash
            esp_err_t err = limiter_save_settings(&limiter);
            if (err != ESP_OK) {
                printf("Warning: Failed to save settings to flash\n");
            }
        }
        else if (strcmp(token, "disable") == 0) {
            limiter_set_enabled(&limiter, false);
            printf("Limiter disabled (bypass mode)\n");
            
            // Save settings to flash
            esp_err_t err = limiter_save_settings(&limiter);
            if (err != ESP_OK) {
                printf("Warning: Failed to save settings to flash\n");
            }
        }
        else if (strcmp(token, "reset") == 0) {
            limiter_reset(&limiter);
            printf("Limiter state reset (buffer and envelope cleared)\n");
        }
        else if (strcmp(token, "stats") == 0) {
            show_limiter_stats();
        }
        else if (strcmp(token, "save") == 0) {
            esp_err_t err = limiter_save_settings(&limiter);
            if (err == ESP_OK) {
                printf("Limiter settings saved to flash successfully\n");
            } else {
                printf("Error: Failed to save settings to flash: %s\n", esp_err_to_name(err));
            }
        }
        else {
            printf("Unknown limiter subcommand: %s\n", token);
            printf("Try: lim show, lim threshold, lim enable, lim disable, lim reset, lim stats, lim save\n");
        }
    }
    else if (strcmp(token, "gain") == 0 || strcmp(token, "pregain") == 0) {
        token = strtok(NULL, " ");
        if (token == NULL) {
            printf("Error: Pre-gain command requires subcommand\n");
            printf("Try: gain show, gain set, gain enable, gain disable, gain save\n");
            return;
        }
        
        if (strcmp(token, "show") == 0) {
            show_pregain_settings();
        }
        else if (strcmp(token, "set") == 0) {
            char* gain_str = strtok(NULL, " ");
            
            if (gain_str == NULL) {
                printf("Error: Usage: gain set <db>\n");
                printf("Example: gain set 3.0\n");
                return;
            }
            
            float gain = atof(gain_str);
            
            if (gain < -12.0f || gain > 12.0f) {
                printf("Warning: Pre-gain clamped to range -12.0 to +12.0 dB\n");
            }
            
            bool success = pregain_set_gain(&pregain, gain);
            if (success) {
                printf("Set pre-gain to %.1f dB (%.3fx linear)\n", pregain_get_gain(&pregain), pregain.gain_linear);
                
                // Save settings to flash
                esp_err_t err = pregain_save_settings(&pregain);
                if (err != ESP_OK) {
                    printf("Warning: Failed to save settings to flash\n");
                }
            } else {
                printf("Error: Failed to set pre-gain\n");
            }
        }
        else if (strcmp(token, "enable") == 0) {
            pregain_set_enabled(&pregain, true);
            printf("Pre-gain enabled\n");
            
            // Save settings to flash
            esp_err_t err = pregain_save_settings(&pregain);
            if (err != ESP_OK) {
                printf("Warning: Failed to save settings to flash\n");
            }
        }
        else if (strcmp(token, "disable") == 0) {
            pregain_set_enabled(&pregain, false);
            printf("Pre-gain disabled (bypass mode)\n");
            
            // Save settings to flash
            esp_err_t err = pregain_save_settings(&pregain);
            if (err != ESP_OK) {
                printf("Warning: Failed to save settings to flash\n");
            }
        }
        else if (strcmp(token, "save") == 0) {
            esp_err_t err = pregain_save_settings(&pregain);
            if (err == ESP_OK) {
                printf("Pre-gain settings saved to flash successfully\n");
            } else {
                printf("Error: Failed to save settings to flash: %s\n", esp_err_to_name(err));
            }
        }
        else {
            printf("Unknown pre-gain subcommand: %s\n", token);
            printf("Try: gain show, gain set, gain enable, gain disable, gain save\n");
        }
    }
    else if (strcmp(token, "sub") == 0) {
        token = strtok(NULL, " ");
        if (token == NULL) {
            printf("Error: Subsonic command requires subcommand\n");
            printf("Try: sub show, sub freq, sub enable, sub disable, sub reset, sub save\n");
            return;
        }
        
        if (strcmp(token, "show") == 0) {
            show_subsonic_settings();
        }
        else if (strcmp(token, "freq") == 0) {
            char* freq_str = strtok(NULL, " ");
            
            if (freq_str == NULL) {
                printf("Error: Usage: sub freq <hz>\n");
                printf("Example: sub freq 25.0\n");
                printf("Recommended range: 25-30 Hz\n");
                return;
            }
            
            float freq = atof(freq_str);
            
            if (freq < 15.0f || freq > 50.0f) {
                printf("Warning: Frequency out of recommended range (15-50 Hz)\n");
            }
            
            bool success = subsonic_set_frequency(&subsonic, freq, SAMPLE_RATE);
            if (success) {
                printf("Set subsonic cutoff frequency to %.1f Hz\n", subsonic_get_frequency(&subsonic));
                
                // Save settings to flash
                esp_err_t err = subsonic_save_settings(&subsonic);
                if (err != ESP_OK) {
                    printf("Warning: Failed to save settings to flash\n");
                }
            } else {
                printf("Error: Failed to set frequency\n");
            }
        }
        else if (strcmp(token, "enable") == 0) {
            subsonic_set_enabled(&subsonic, true);
            printf("Subsonic filter enabled\n");
            
            // Save settings to flash
            esp_err_t err = subsonic_save_settings(&subsonic);
            if (err != ESP_OK) {
                printf("Warning: Failed to save settings to flash\n");
            }
        }
        else if (strcmp(token, "disable") == 0) {
            subsonic_set_enabled(&subsonic, false);
            printf("Subsonic filter disabled (bypass mode)\n");
            
            // Save settings to flash
            esp_err_t err = subsonic_save_settings(&subsonic);
            if (err != ESP_OK) {
                printf("Warning: Failed to save settings to flash\n");
            }
        }
        else if (strcmp(token, "reset") == 0) {
            subsonic_reset(&subsonic);
            printf("Subsonic filter state reset (history cleared)\n");
        }
        else if (strcmp(token, "save") == 0) {
            esp_err_t err = subsonic_save_settings(&subsonic);
            if (err == ESP_OK) {
                printf("Subsonic filter settings saved to flash successfully\n");
            } else {
                printf("Error: Failed to save settings to flash: %s\n", esp_err_to_name(err));
            }
        }
        else {
            printf("Unknown subsonic subcommand: %s\n", token);
            printf("Try: sub show, sub freq, sub enable, sub disable, sub reset, sub save\n");
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
