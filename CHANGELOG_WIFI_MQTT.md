# WiFi and MQTT Integration - Changelog

## Overview

Added comprehensive WiFi and MQTT support to ESP-DSP for remote control and management of all audio processors.

## New Features

### WiFi Client Manager
- **File**: `wifi_manager.cpp`/`wifi_manager.h`
- Persistent WiFi credentials storage in NVS
- Automatic reconnection with retry logic
- Status monitoring and IP address reporting
- Serial command interface integration

### MQTT Client Manager
- **File**: `mqtt_manager.cpp`/`mqtt_manager.h`
- Full bidirectional MQTT communication
- Automatic subscription to all control topics
- State publishing with retain flag for persistent state
- Command processing for all audio processors
- JSON-formatted status messages

### Supported Processors via MQTT

1. **Subsonic Filter**
   - Set cutoff frequency
   - Enable/disable
   - State publishing

2. **Pre-Gain**
   - Set gain level (-12 to +12 dB)
   - Enable/disable
   - State publishing

3. **Equalizer**
   - Individual band control (5 bands)
   - Preset loading (flat, bass, vocal, rock, jazz)
   - Enable/disable
   - State publishing

4. **Limiter**
   - Threshold adjustment
   - Enable/disable
   - State publishing

## Serial Commands Added

### WiFi Commands
```
wifi status                  - Show WiFi connection status
wifi set <ssid> <password>  - Connect to WiFi network
wifi disconnect             - Disconnect from WiFi
```

### MQTT Commands
```
mqtt status                 - Show MQTT connection status
mqtt set <broker_uri>       - Connect to MQTT broker
mqtt disconnect            - Disconnect from MQTT broker
mqtt publish               - Publish all current states
```

## MQTT Topic Structure

### Base Topic
`esp-dsp/`

### Command Topics (Subscribe)
- `esp-dsp/subsonic/freq` - Subsonic cutoff frequency
- `esp-dsp/subsonic/enable` - Subsonic enable/disable
- `esp-dsp/pregain/set` - Pre-gain level
- `esp-dsp/pregain/enable` - Pre-gain enable/disable
- `esp-dsp/eq/band/[0-4]` - Individual EQ band control
- `esp-dsp/eq/enable` - EQ enable/disable
- `esp-dsp/eq/preset` - EQ preset selection
- `esp-dsp/limiter/threshold` - Limiter threshold
- `esp-dsp/limiter/enable` - Limiter enable/disable

### State Topics (Publish)
- `esp-dsp/status` - Overall system status (JSON)
- `esp-dsp/subsonic/state` - Subsonic filter state (JSON)
- `esp-dsp/pregain/state` - Pre-gain state (JSON)
- `esp-dsp/eq/state` - Equalizer state (JSON)
- `esp-dsp/limiter/state` - Limiter state (JSON)

## Integration Examples

### MQTT Broker Setup
```bash
# Using Mosquitto
mosquitto -v

# Or with Docker
docker run -it -p 1883:1883 eclipse-mosquitto
```

### Configure ESP-DSP
```
wifi set MyNetwork MyPassword
mqtt set mqtt://192.168.1.100:1883
```

### Control via MQTT
```bash
# Set pre-gain
mosquitto_pub -h 192.168.1.100 -t esp-dsp/pregain/set -m "3.0"

# Apply bass preset
mosquitto_pub -h 192.168.1.100 -t esp-dsp/eq/preset -m "bass"

# Enable limiter
mosquitto_pub -h 192.168.1.100 -t esp-dsp/limiter/enable -m "true"
```

### Monitor State Changes
```bash
mosquitto_sub -h 192.168.1.100 -t esp-dsp/#
```

## Files Modified

### New Files
- `main/wifi_manager.h` - WiFi manager API
- `main/wifi_manager.cpp` - WiFi implementation
- `main/mqtt_manager.h` - MQTT manager API
- `main/mqtt_manager.cpp` - MQTT implementation
- `docs/WIFI_MQTT_SETUP.md` - Comprehensive setup guide
- `CHANGELOG_WIFI_MQTT.md` - This file

### Modified Files
- `main/CMakeLists.txt` - Added new source files and component dependencies
- `main/serial_commands.cpp` - Added WiFi and MQTT commands
- `main/serial_commands.h` - Updated API
- `main/esp-dsp.cpp` - Initialize WiFi and MQTT managers
- `README.md` - Updated feature list and documentation references

## Component Dependencies Added

- `driver` - GPIO and hardware support
- `nvs_flash` - Persistent storage
- `esp_wifi` - WiFi functionality
- `esp_netif` - Network interface
- `esp_event` - Event handling
- `mqtt` - MQTT client library

## Configuration Notes

### NVS Storage
WiFi and MQTT settings are stored in separate NVS namespaces:
- `wifi_config` - WiFi credentials (SSID, password)
- `mqtt_config` - MQTT broker URI

Settings persist across reboots and are automatically loaded on startup.

### Memory Usage
The addition of WiFi and MQTT increases binary size by approximately 40KB. Current partition is 96% full with all features enabled.

### Network Requirements
- 2.4GHz WiFi network (ESP32-C3 limitation)
- MQTT broker accessible on network
- Default MQTT port: 1883 (unencrypted)

## Future Enhancements

Potential improvements for future releases:
- [ ] MQTT authentication (username/password)
- [ ] TLS/SSL encryption (`mqtts://`)
- [ ] Home Assistant auto-discovery
- [ ] Custom MQTT client ID configuration
- [ ] QoS configuration options
- [ ] Last Will and Testament (LWT) messages
- [ ] WiFi provisioning via AP mode
- [ ] WebSocket support for browser-based control
- [ ] REST API endpoint

## Testing

### Unit Testing
- WiFi connection and reconnection
- MQTT message parsing and routing
- NVS storage and retrieval
- Command validation

### Integration Testing
- End-to-end MQTT control flow
- State synchronization
- Network resilience (disconnect/reconnect)
- Multiple simultaneous MQTT clients

## Documentation

See comprehensive documentation in:
- `docs/WIFI_MQTT_SETUP.md` - Setup and usage guide
- `README.md` - Updated feature list
- `docs/SERIAL_COMMANDS.md` - Serial command reference

## Version Information

- **ESP-IDF Version**: 5.4.1
- **Target**: ESP32-C3
- **MQTT Client**: esp-mqtt component
- **WiFi Mode**: Station (STA) only
- **Initial Release**: January 2025
