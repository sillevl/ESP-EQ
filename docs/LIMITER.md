# True-Peak Limiter

## Overview

The true-peak limiter is a safety feature that prevents audio clipping that can occur when the equalizer boosts certain frequencies. It acts as the final stage in the audio processing chain, applying transparent gain reduction only when necessary to keep peaks below the threshold.

## Features

- **True-Peak Detection**: Detects peaks that would cause clipping
- **Lookahead**: 5ms lookahead buffer prevents harsh limiting artifacts
- **Fast Attack**: 0.5ms attack time catches transients quickly
- **Smooth Release**: 50ms release time provides natural recovery
- **Statistics**: Track peak reduction and clips prevented
- **Persistent Settings**: Settings are saved to flash memory

## Technical Details

### Processing Chain

The limiter is the last stage in the audio processing chain:

```
Audio Input → EQ Processing → Limiter → Audio Output
```

### Algorithm

The limiter uses a sophisticated envelope follower algorithm:

1. **Peak Detection**: Analyzes current audio samples to detect potential clipping
2. **Lookahead Buffer**: Delays the audio by 5ms to allow the limiter to react before peaks occur
3. **Envelope Follower**: Smoothly applies gain reduction with fast attack and slower release
4. **Gain Application**: Multiplies the delayed audio by the envelope to prevent clipping

### Default Settings

- **Threshold**: -0.5 dB (slightly below full scale)
- **Attack**: 0.5 ms (fast enough to catch transients)
- **Release**: 50 ms (smooth recovery without pumping)
- **Lookahead**: 5 ms (prevents artifacts while minimizing latency)

## Serial Commands

### Show Current Settings
```
lim show
```
Displays limiter status, threshold, and timing parameters.

### Set Threshold
```
lim threshold <db>
```
Set the limiter threshold in dB. Range: -12.0 to 0.0 dB

**Example:**
```
lim threshold -1.0    # Set threshold to -1 dB
```

### Enable/Disable
```
lim enable     # Enable limiter protection
lim disable    # Bypass limiter
```

### View Statistics
```
lim stats
```
Shows:
- Peak reduction (maximum gain reduction applied)
- Number of clips prevented

### Reset State
```
lim reset
```
Clears the lookahead buffer and envelope state. Useful if audio glitches occur.

### Save Settings
```
lim save
```
Manually save current limiter settings to flash memory (settings are also auto-saved when changed).

## Usage Guidelines

### When to Adjust the Threshold

**Default (-0.5 dB)**: Suitable for most situations
- Provides a small safety margin
- Prevents digital clipping
- Minimal limiting in normal operation

**Lower Threshold (-2.0 to -3.0 dB)**: More aggressive limiting
- Useful when applying large EQ boosts
- Provides more headroom
- May reduce overall loudness

**Higher Threshold (-0.1 dB)**: Maximum loudness
- Only use with conservative EQ settings
- Less safety margin
- Higher risk of inter-sample peaks

### Monitoring Limiter Activity

Use `lim stats` periodically to check limiter performance:

```
Limiter Statistics:
  Peak Reduction: -2.34 dB
  Clips Prevented: 1523
```

- **Peak Reduction**: If consistently > -3dB, consider:
  - Reducing EQ gain
  - Lowering pre-gain
  - Lowering limiter threshold
  
- **Clips Prevented**: Shows how many times limiting occurred
  - High count is normal with aggressive EQ
  - Zero count means limiter is not needed (but good to keep enabled for safety)

### Recommended Settings by Use Case

#### Conservative Audio Processing
```
eq pregain 0.0
lim threshold -0.5
lim enable
```

#### Heavy EQ with Bass Boost
```
eq pregain -3.0
lim threshold -1.0
lim enable
```

#### Maximum Loudness (use with caution)
```
eq pregain 3.0
lim threshold -0.1
lim enable
```

## Troubleshooting

### Audio Sounds "Pumping" or "Breathing"
- This indicates excessive limiting
- Solution: Reduce EQ gains or lower pre-gain
- Alternative: Lower limiter threshold for smoother limiting

### Limiter Stats Show Zero Activity
- Limiter is not engaging (which is fine!)
- EQ settings are conservative enough
- You can keep it enabled as a safety net

### Sudden Volume Drops
- Limiter is working hard to prevent clipping
- Check `lim stats` to see peak reduction
- Solution: Reduce EQ boost or pre-gain

### Audio Glitches After EQ Changes
- Limiter state may need reset
- Use `lim reset` to clear buffers
- Should resolve glitches immediately

## Technical Implementation

The limiter is implemented in `limiter.cpp` with:
- Fixed-point arithmetic for efficiency
- Circular buffer for lookahead
- Logarithmic envelope detection
- Per-sample processing

Memory usage:
- ~2KB for lookahead buffer
- Minimal CPU overhead (~5% at 48kHz)

## Integration with EQ

The limiter and EQ work together:

1. **Pre-gain** is applied before EQ bands
2. **EQ bands** boost/cut specific frequencies
3. **Limiter** catches any resulting peaks

This allows you to use aggressive EQ settings without fear of clipping.

## Example Workflow

1. Start with default settings:
   ```
   lim show
   eq show
   ```

2. Apply desired EQ settings:
   ```
   eq set 0 6.0    # Boost bass
   eq set 4 4.0    # Boost treble
   ```

3. Check if limiting occurs:
   ```
   lim stats
   ```

4. If peak reduction is excessive (< -5dB), adjust pre-gain:
   ```
   eq pregain -3.0
   ```

5. Verify improvement:
   ```
   lim stats
   ```

6. Save configuration:
   ```
   eq save
   lim save
   ```

## Notes

- The limiter adds approximately 5ms of latency (lookahead time)
- This latency is negligible for most audio applications
- Settings persist across reboots
- Limiter statistics reset when the device restarts
