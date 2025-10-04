# Changelog - Persistent Settings Feature

## Changes Made

### New Feature: Persistent Equalizer Settings

Equalizer settings are now automatically saved to flash memory and restored on boot.

### Files Modified

#### 1. `main/equalizer.h`
- Added `#include "esp_err.h"`
- Added `equalizer_save_settings()` function declaration
- Added `equalizer_load_settings()` function declaration

#### 2. `main/equalizer.cpp`
- Added includes: `nvs_flash.h`, `nvs.h`, `esp_log.h`
- Added NVS namespace and key definitions
- Implemented `equalizer_save_settings()` - saves all band gains and enabled state to NVS
- Implemented `equalizer_load_settings()` - loads settings from NVS on boot

#### 3. `main/esp-dsp.cpp`
- Added `#include "nvs_flash.h"`
- Added NVS initialization in `app_main()`
- Modified equalizer initialization to load saved settings
- Added fallback to defaults if no saved settings exist

#### 4. `main/serial_commands.cpp`
- Updated help text to mention automatic saving
- Added `eq save` command for manual saving
- Modified `eq set` command to auto-save after setting band gain
- Modified `eq enable/disable` commands to auto-save after state change
- Modified `eq preset` command to auto-save after applying preset

#### 5. `docs/PERSISTENT_SETTINGS.md` (NEW)
- Comprehensive documentation of the persistent settings feature
- Usage guide and examples
- Technical implementation details
- Troubleshooting information

## How It Works

1. **On Boot**: NVS is initialized, and saved settings are loaded into the equalizer structure
2. **During Operation**: Any setting change automatically triggers a save to flash
3. **Next Boot**: Settings are restored exactly as they were

## Testing Checklist

- [ ] Build project successfully
- [ ] Flash to ESP32
- [ ] Change equalizer settings via serial commands
- [ ] Verify "saved to flash" messages appear
- [ ] Power cycle the device
- [ ] Verify settings are restored on boot
- [ ] Try each command:
  - [ ] `eq set 0 6.0` (auto-saves)
  - [ ] `eq preset bass` (auto-saves)
  - [ ] `eq enable` / `eq disable` (auto-saves)
  - [ ] `eq save` (manual save)
  - [ ] `eq show` (display current)

## Build Instructions

```bash
cd d:\esp-dsp\esp-dsp
idf.py build
idf.py flash monitor
```

## Example Usage Session

```
# Set custom EQ
eq set 0 6.0
eq set 1 4.0
eq set 4 3.0

# Power cycle device

# Check settings after reboot
eq show

# You should see:
#   60Hz:   6.0 dB
#   250Hz:  4.0 dB
#   1kHz:   0.0 dB
#   4kHz:   0.0 dB
#   12kHz:  3.0 dB
```

## Storage Size

- Total flash usage: ~25 bytes (plus NVS overhead)
- No impact on audio processing performance
- Settings persist indefinitely across power cycles

## Backward Compatibility

- First boot with new firmware will use default settings
- No migration needed from previous versions
- Existing functionality unchanged (only extended)
