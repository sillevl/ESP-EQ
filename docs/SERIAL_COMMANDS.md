# Serial Command Interface

The ESP32 DSP includes a serial command interface for real-time control of the equalizer and system monitoring.

## Accessing the Serial Interface

Connect to the ESP32 via serial terminal at **115200 baud**:

### Windows (PowerShell)
```powershell
idf.py -p COM3 monitor
```

### Linux/macOS
```bash
idf.py -p /dev/ttyUSB0 monitor
```

Or use any serial terminal (PuTTY, Tera Term, screen, minicom, etc.)

## Command Overview

| Command | Description |
|---------|-------------|
| `help` | Show all available commands |
| `status` | Display system status |
| `eq show` | Show current equalizer settings |
| `eq set <band> <gain>` | Set band gain |
| `eq enable` | Enable equalizer |
| `eq disable` | Disable equalizer (bypass) |
| `eq reset` | Reset equalizer state |
| `eq preset <name>` | Load EQ preset |

## Command Reference

### System Commands

#### help
Shows all available commands with descriptions

```
> help
```

#### status
Displays system information including:
- Sample rate
- Number of channels
- Buffer size
- Bit depth
- Memory usage

```
> status

System Status:
  Sample Rate: 48000 Hz
  Channels: 2 (Stereo)
  Buffer Size: 512 samples
  Bit Depth: 24-bit

  Free Heap: 156784 bytes
  Min Free Heap: 143920 bytes
```

### Equalizer Commands

#### eq show
Display current equalizer settings for all bands

```
> eq show

Equalizer Settings:
  Status: ENABLED

  Band  | Frequency | Gain
  ------|-----------|--------
    0   | 60Hz      | +3.0 dB
    1   | 250Hz     | +2.0 dB
    2   | 1kHz      | +0.0 dB
    3   | 4kHz      | +1.0 dB
    4   | 12kHz     | +4.0 dB
```

#### eq set <band> <gain>
Set gain for a specific frequency band

**Parameters:**
- `band`: Band number (0-4)
  - 0 = 60Hz (Sub-bass)
  - 1 = 250Hz (Bass)
  - 2 = 1kHz (Midrange)
  - 3 = 4kHz (Upper mid)
  - 4 = 12kHz (Treble)
- `gain`: Gain in dB (-12.0 to +12.0)

**Examples:**
```
> eq set 0 6.0
Set 60Hz (band 0) to 6.0 dB

> eq set 2 -3.0
Set 1kHz (band 2) to -3.0 dB

> eq set 4 0.0
Set 12kHz (band 4) to 0.0 dB
```

#### eq enable
Enable the equalizer (turn on processing)

```
> eq enable
Equalizer enabled
```

#### eq disable
Disable the equalizer (bypass mode - audio passes through unprocessed)

```
> eq disable
Equalizer disabled (bypass mode)
```

#### eq reset
Reset the equalizer filter state (clears filter history). Use this if you hear artifacts after changing settings.

```
> eq reset
Equalizer state reset (filter history cleared)
```

#### eq preset <name>
Load a predefined EQ preset

**Available Presets:**
- `flat` - All bands at 0dB (reference/neutral)
- `bass` - Enhanced bass response
- `vocal` - Optimized for speech/vocals
- `rock` - V-shaped curve (boosted bass and treble)
- `jazz` - Natural with slight warmth

**Examples:**
```
> eq preset bass
Applied 'Bass Boost' preset
New EQ settings:
  60Hz:   6.0 dB
  250Hz:  4.0 dB
  1kHz:   0.0 dB
  4kHz:   0.0 dB
  12kHz:  0.0 dB

> eq preset flat
Applied 'Flat' preset (all bands at 0dB)
New EQ settings:
  60Hz:   0.0 dB
  250Hz:  0.0 dB
  1kHz:   0.0 dB
  4kHz:   0.0 dB
  12kHz:  0.0 dB
```

## Preset Details

### Flat (Reference)
All bands at 0dB - transparent, no coloration
```
60Hz:   0.0 dB
250Hz:  0.0 dB
1kHz:   0.0 dB
4kHz:   0.0 dB
12kHz:  0.0 dB
```

### Bass Boost
Enhanced low-end for electronic music
```
60Hz:   +6.0 dB
250Hz:  +4.0 dB
1kHz:   0.0 dB
4kHz:   0.0 dB
12kHz:  0.0 dB
```

### Vocal Clarity
Enhanced midrange and highs for speech
```
60Hz:   -2.0 dB
250Hz:  0.0 dB
1kHz:   +3.0 dB
4kHz:   +5.0 dB
12kHz:  +2.0 dB
```

### Rock
Scooped mids with strong bass and treble
```
60Hz:   +5.0 dB
250Hz:  +3.0 dB
1kHz:   -4.0 dB
4kHz:   +2.0 dB
12kHz:  +6.0 dB
```

### Jazz
Natural, balanced with slight warmth
```
60Hz:   +2.0 dB
250Hz:  +1.0 dB
1kHz:   0.0 dB
4kHz:   +1.0 dB
12kHz:  +3.0 dB
```

## Usage Examples

### Basic EQ Adjustment
```
> eq show                    # Check current settings
> eq set 0 6.0              # Boost bass
> eq set 4 4.0              # Boost treble
> eq set 2 -2.0             # Cut mids slightly
> eq show                    # Verify changes
```

### Quick Preset Change
```
> eq preset rock            # Load rock preset
> eq show                    # View settings
> eq set 2 -2.0             # Fine-tune mids
```

### Toggle EQ On/Off
```
> eq disable                # Bypass EQ (A/B comparison)
> eq enable                 # Turn EQ back on
```

### Troubleshooting
```
> status                    # Check system health
> eq reset                  # Clear filter state if artifacts
> eq preset flat            # Return to neutral
```

## Tips

1. **Start with presets**: Use presets as starting points, then fine-tune individual bands

2. **Small adjustments**: Changes of 1-3 dB are often sufficient. Extreme boosts can cause distortion.

3. **A/B Testing**: Use `eq disable` and `eq enable` to compare processed vs. unprocessed audio

4. **Reset if needed**: If you hear clicks or pops after changing settings, use `eq reset`

5. **Save custom settings**: Note down your favorite settings and add them to the code as a custom preset

## Adding Custom Presets

Edit `main/serial_commands.cpp` in the `apply_preset()` function:

```cpp
else if (strcmp(preset_name, "mypreset") == 0) {
    // My custom preset
    equalizer_set_band_gain(&equalizer, 0, 4.0f, SAMPLE_RATE);
    equalizer_set_band_gain(&equalizer, 1, 2.0f, SAMPLE_RATE);
    equalizer_set_band_gain(&equalizer, 2, -1.0f, SAMPLE_RATE);
    equalizer_set_band_gain(&equalizer, 3, 3.0f, SAMPLE_RATE);
    equalizer_set_band_gain(&equalizer, 4, 5.0f, SAMPLE_RATE);
    printf("Applied 'My Custom' preset\n");
}
```

Then rebuild and flash the firmware.

## Command Line Editing

The serial interface supports:
- **Backspace**: Delete previous character
- **Enter**: Execute command
- **Echo**: Characters are echoed as you type

Note: Arrow keys for command history are not currently supported.

## Error Handling

The interface provides helpful error messages:

```
> eq set 10 5.0
Error: Band must be 0-4 (0=60Hz, 1=250Hz, 2=1kHz, 3=4kHz, 4=12kHz)

> eq set 0 20.0
Warning: Gain clamped to range -12.0 to +12.0 dB
Set 60Hz (band 0) to 12.0 dB

> eq preset unknown
Unknown preset: unknown
Available presets: flat, bass, vocal, rock, jazz

> unknown_command
Unknown command: unknown_command
Type 'help' for available commands
```

## Future Enhancements

Planned features:
- [ ] Command history (up/down arrows)
- [ ] Tab completion
- [ ] Save/load custom presets to flash
- [ ] MQTT/HTTP API for network control
- [ ] Spectrum analyzer display
- [ ] Real-time gain visualization

## Troubleshooting

### Commands not responding
- Verify serial connection at 115200 baud
- Press Enter to get a new prompt
- Try typing `help` and pressing Enter

### Characters not echoing
- Check terminal local echo settings
- Ensure correct baud rate (115200)

### Strange characters
- Verify baud rate is set to 115200
- Check that line endings are set to CR or LF

## See Also

- [Equalizer Documentation](EQUALIZER.md) - Detailed EQ technical information
- [Troubleshooting Guide](TROUBLESHOOTING.md) - General troubleshooting
- [Adding Effects](ADDING_EFFECTS.md) - How to add more audio processing
