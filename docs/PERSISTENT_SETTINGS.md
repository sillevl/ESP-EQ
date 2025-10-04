# Persistent Equalizer Settings

## Overview

The equalizer settings are now stored in the ESP32's NVS (Non-Volatile Storage) flash memory and automatically restored on every boot. This ensures your EQ settings are preserved across power cycles and resets.

## Features

### Automatic Saving
Settings are **automatically saved to flash** whenever you change them via serial commands:
- Setting individual band gains (`eq set`)
- Enabling/disabling the equalizer (`eq enable`, `eq disable`)
- Applying presets (`eq preset`)

### Automatic Loading
On every boot, the system:
1. Initializes NVS flash storage
2. Loads saved equalizer settings (if they exist)
3. Falls back to default settings if no saved settings are found

### What is Saved
- **Enabled/Disabled state**: Whether the equalizer is active or bypassed
- **All 5 band gains**: Individual gain values for each frequency band
  - 60Hz (Sub-bass)
  - 250Hz (Bass)
  - 1kHz (Mid)
  - 4kHz (Upper mid)
  - 12kHz (Treble)

## Usage

### Normal Operation
Just use the equalizer commands as usual - settings are saved automatically:

```
eq set 0 6.0          # Set 60Hz to +6dB (automatically saved)
eq preset bass        # Load bass preset (automatically saved)
eq disable            # Disable EQ (automatically saved)
```

### Manual Save
You can also manually trigger a save (though it's not usually needed):

```
eq save               # Manually save current settings
```

### Viewing Settings
Check your current settings at any time:

```
eq show               # Display current EQ configuration
```

## Implementation Details

### Storage Format
- **Namespace**: `eq_settings`
- **Storage Method**: NVS (Non-Volatile Storage) - part of ESP32's flash memory
- **Data Format**: 
  - Enabled state: 8-bit unsigned integer (0 or 1)
  - Band gains: 32-bit signed integers (fixed-point, multiplied by 100)
  - Example: 6.5 dB is stored as 650

### Functions Added

#### `equalizer.h` / `equalizer.cpp`
```cpp
esp_err_t equalizer_save_settings(equalizer_t *eq);
esp_err_t equalizer_load_settings(equalizer_t *eq, uint32_t sample_rate);
```

### Boot Sequence
1. Initialize NVS flash
2. Initialize I2S audio
3. Initialize equalizer with default settings
4. Attempt to load saved settings from NVS
5. If loading succeeds, settings are restored
6. If loading fails (first boot), use defaults

### Log Messages
On boot, you'll see log messages indicating whether settings were loaded:

**Settings found:**
```
I (1234) EQUALIZER: Equalizer settings loaded from flash:
I (1235) EQUALIZER:   Status: ENABLED
I (1236) EQUALIZER:   60Hz:   6.0 dB
I (1237) EQUALIZER:   250Hz:  4.0 dB
...
```

**No settings found (first boot):**
```
I (1234) EQUALIZER: No saved equalizer settings found, using defaults
I (1235) ESP-DSP: Using default equalizer settings
```

## Flash Wear Considerations

NVS uses wear-leveling internally, so it's safe to save settings frequently. The ESP32 flash is rated for:
- **Minimum**: 100,000 erase cycles per sector
- With wear-leveling, this extends to millions of writes

Since settings are only saved when you change them (not continuously), flash wear is not a concern for normal usage.

## Troubleshooting

### Settings Not Persisting
If settings don't persist across reboots:

1. Check serial output for error messages:
   ```
   Warning: Failed to save settings to flash
   ```

2. Try manually saving:
   ```
   eq save
   ```

3. If NVS is corrupted, you may see:
   ```
   I (123) ESP-DSP: Erasing NVS flash...
   ```
   This is normal on first boot or after flash corruption.

### Resetting to Defaults
To clear all saved settings and start fresh:

1. Erase flash:
   ```bash
   idf.py erase-flash
   ```

2. Or manually reset to flat preset and save:
   ```
   eq preset flat
   ```

## Technical Notes

### NVS Partition
The NVS partition is defined in the partition table (typically 24KB). The equalizer settings use a small portion of this space:
- Enabled state: 1 byte
- 5 band gains: 5 × 4 bytes = 20 bytes
- **Total**: ~25 bytes (plus NVS overhead)

### Thread Safety
The save/load functions are called from:
- Main thread (during initialization)
- Serial command task (when settings change)

Since settings changes are infrequent and serialized through the command parser, no additional locking is needed.

## Future Enhancements

Possible improvements for future versions:
- Save/load multiple preset slots
- Import/export settings via serial port
- Reset to factory defaults command
- Settings version tracking for backward compatibility
