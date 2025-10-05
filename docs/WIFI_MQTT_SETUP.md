# WiFi and MQTT Setup Guide

This guide explains how to configure and use WiFi and MQTT connectivity for remote control of the ESP-DSP audio processor.

## Overview

The ESP-DSP now supports WiFi connectivity and MQTT protocol for remote management of all audio processing parameters. This allows you to control the DSP from:
- MQTT clients (e.g., MQTT Explorer, Home Assistant)
- Custom applications
- IoT dashboards
- Mobile apps

## WiFi Configuration

### Connect to WiFi

Use the serial command interface to configure WiFi:

```
wifi set <ssid> <password>
```

**Example:**
```
wifi set MyNetwork MyPassword123
```

The WiFi credentials are saved to NVS (flash) and will be restored on reboot.

### Check WiFi Status

```
wifi status
```

This will show:
- Connection state
- Connected SSID
- IP address

### Disconnect from WiFi

```
wifi disconnect
```

## MQTT Configuration

### Set MQTT Broker

After connecting to WiFi, configure the MQTT broker:

```
mqtt set <broker_uri>
```

**Example:**
```
mqtt set mqtt://192.168.1.100:1883
```

The broker URI format is: `mqtt://hostname:port` or `mqtt://ip_address:port`

### Check MQTT Status

```
mqtt status
```

This will show:
- Connection state
- Base topic

### Disconnect from MQTT

```
mqtt disconnect
```

### Manually Publish States

To publish all current processor states:

```
mqtt publish
```

## MQTT Topics

The ESP-DSP uses a hierarchical topic structure under the base topic `esp-dsp`.

### Status Topics (Published by ESP-DSP)

| Topic | Description | Payload Example |
|-------|-------------|-----------------|
| `esp-dsp/status` | Overall system status | `{"sample_rate":48000,"channels":2,...}` |
| `esp-dsp/subsonic/state` | Subsonic filter state | `{"enabled":true,"freq":25.0}` |
| `esp-dsp/pregain/state` | Pre-gain state | `{"enabled":true,"gain":3.0}` |
| `esp-dsp/eq/state` | Equalizer state | `{"enabled":true,"bands":[6.0,4.0,0.0,0.0,0.0]}` |
| `esp-dsp/limiter/state` | Limiter state | `{"enabled":true,"threshold":-0.5}` |

All state topics are published with the **retain flag** so new clients receive the current state immediately.

### Command Topics (Subscribe to control ESP-DSP)

#### Subsonic Filter

| Topic | Payload | Description |
|-------|---------|-------------|
| `esp-dsp/subsonic/freq` | `25.0` | Set cutoff frequency (15-50 Hz) |
| `esp-dsp/subsonic/enable` | `true` or `false` | Enable/disable subsonic filter |

#### Pre-Gain

| Topic | Payload | Description |
|-------|---------|-------------|
| `esp-dsp/pregain/set` | `3.0` | Set pre-gain (-12 to +12 dB) |
| `esp-dsp/pregain/enable` | `true` or `false` | Enable/disable pre-gain |

#### Equalizer

| Topic | Payload | Description |
|-------|---------|-------------|
| `esp-dsp/eq/band/0` | `6.0` | Set band 0 (60Hz) gain (-12 to +12 dB) |
| `esp-dsp/eq/band/1` | `4.0` | Set band 1 (250Hz) gain |
| `esp-dsp/eq/band/2` | `0.0` | Set band 2 (1kHz) gain |
| `esp-dsp/eq/band/3` | `0.0` | Set band 3 (4kHz) gain |
| `esp-dsp/eq/band/4` | `0.0` | Set band 4 (12kHz) gain |
| `esp-dsp/eq/enable` | `true` or `false` | Enable/disable equalizer |
| `esp-dsp/eq/preset` | `flat`, `bass`, `vocal`, `rock`, `jazz` | Apply EQ preset |

#### Limiter

| Topic | Payload | Description |
|-------|---------|-------------|
| `esp-dsp/limiter/threshold` | `-0.5` | Set limiter threshold (-12 to 0 dB) |
| `esp-dsp/limiter/enable` | `true` or `false` | Enable/disable limiter |

## Usage Examples

### Using mosquitto_pub (Command Line)

Set pre-gain to 3 dB:
```bash
mosquitto_pub -h 192.168.1.100 -t esp-dsp/pregain/set -m "3.0"
```

Boost 60Hz by 6 dB:
```bash
mosquitto_pub -h 192.168.1.100 -t esp-dsp/eq/band/0 -m "6.0"
```

Apply bass preset:
```bash
mosquitto_pub -h 192.168.1.100 -t esp-dsp/eq/preset -m "bass"
```

Enable limiter:
```bash
mosquitto_pub -h 192.168.1.100 -t esp-dsp/limiter/enable -m "true"
```

### Using mosquitto_sub (Monitor Status)

Subscribe to all state updates:
```bash
mosquitto_sub -h 192.168.1.100 -t esp-dsp/#
```

Monitor only EQ state:
```bash
mosquitto_sub -h 192.168.1.100 -t esp-dsp/eq/state
```

## Home Assistant Integration

Add to your `configuration.yaml`:

```yaml
mqtt:
  sensor:
    - name: "ESP-DSP Pre-Gain"
      state_topic: "esp-dsp/pregain/state"
      value_template: "{{ value_json.gain }}"
      unit_of_measurement: "dB"
    
    - name: "ESP-DSP Limiter Threshold"
      state_topic: "esp-dsp/limiter/state"
      value_template: "{{ value_json.threshold }}"
      unit_of_measurement: "dB"
  
  number:
    - name: "ESP-DSP Pre-Gain Control"
      command_topic: "esp-dsp/pregain/set"
      state_topic: "esp-dsp/pregain/state"
      value_template: "{{ value_json.gain }}"
      min: -12
      max: 12
      step: 0.5
      mode: slider
  
  switch:
    - name: "ESP-DSP Limiter"
      command_topic: "esp-dsp/limiter/enable"
      state_topic: "esp-dsp/limiter/state"
      value_template: "{{ value_json.enabled }}"
      payload_on: "true"
      payload_off: "false"
```

## Python Example

```python
import paho.mqtt.client as mqtt

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT broker")
    # Subscribe to all state topics
    client.subscribe("esp-dsp/#")

def on_message(client, userdata, msg):
    print(f"{msg.topic}: {msg.payload.decode()}")

# Create MQTT client
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# Connect to broker
client.connect("192.168.1.100", 1883, 60)

# Set pre-gain to 3 dB
client.publish("esp-dsp/pregain/set", "3.0")

# Apply bass preset
client.publish("esp-dsp/eq/preset", "bass")

# Start loop
client.loop_forever()
```

## Node-RED Flow

Import this flow to control ESP-DSP from Node-RED:

```json
[
  {
    "id": "mqtt_out",
    "type": "mqtt out",
    "broker": "mqtt_broker",
    "topic": "esp-dsp/pregain/set",
    "name": "ESP-DSP Pre-Gain"
  },
  {
    "id": "mqtt_in",
    "type": "mqtt in",
    "broker": "mqtt_broker",
    "topic": "esp-dsp/pregain/state",
    "name": "ESP-DSP State"
  }
]
```

## Troubleshooting

### WiFi Connection Issues

1. Check SSID and password are correct
2. Ensure WiFi network is 2.4GHz (ESP32-C3 doesn't support 5GHz)
3. Check WiFi signal strength
4. Verify router allows ESP32 MAC address

### MQTT Connection Issues

1. Verify WiFi is connected first
2. Check broker IP address and port
3. Ensure broker allows anonymous connections (or configure authentication)
4. Check firewall settings on broker

### Commands Not Working

1. Verify MQTT connection with `mqtt status`
2. Check topic names match exactly (case-sensitive)
3. Monitor serial output for errors
4. Use MQTT Explorer to verify messages are received

## Security Considerations

**Important:** The current implementation does not include authentication. For production use:

1. Configure MQTT broker with username/password authentication
2. Use TLS/SSL encryption (`mqtts://` URI)
3. Use network segmentation (VLAN)
4. Consider implementing access control on broker

## Advanced Configuration

### Custom Base Topic

To change the base topic from `esp-dsp`, modify `MQTT_BASE_TOPIC` in `mqtt_manager.h` and rebuild.

### Custom Client ID

To change the MQTT client ID, modify `MQTT_CLIENT_ID` in `mqtt_manager.h` and rebuild.

### QoS and Retain Settings

The current implementation uses:
- QoS 1 for command subscriptions (at-least-once delivery)
- QoS 0 for state publications (at-most-once delivery)
- Retain flag enabled for state topics

These can be adjusted in `mqtt_manager.cpp` if needed.
