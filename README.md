# DMX_ESPNOW Library

Status: Untested, WIP!

Send and receive DMX512 universes wirelessly using ESP-NOW on ESP32.

## Features

- ✅ Full DMX512 universe (512 channels)
- ✅ Broadcast mode - unlimited receivers
- ✅ Automatic or manual frame sending
- ✅ Frame loss detection and statistics
- ✅ Simple callback-based API
- ✅ Compatible with ESP32 (ESP-NOW v1)

## Installation

### Arduino IDE
1. Download this library as ZIP
2. In Arduino IDE: Sketch → Include Library → Add .ZIP Library
3. Select the downloaded ZIP file

### PlatformIO
Add to `platformio.ini`:
```ini
lib_deps = 
    DMX_ESPNOW
```

## Quick Start

### Sender (Transmitter)

```cpp
#include <DMX_ESPNOW.h>

DMX_ESPNOW dmx;

void setup() {
  Serial.begin(115200);
  dmx.beginSender(1);  // WiFi channel 1
  dmx.setRefreshRate(50);  // 50 Hz refresh
}

void loop() {
  dmx.setChannel(0, 255);  // Channel 0 = full brightness
  dmx.update();  // Auto-send
}
```

### Receiver

```cpp
#include <DMX_ESPNOW.h>

DMX_ESPNOW dmx;

void onFrame() {
  uint8_t value = dmx.getChannel(0);
  Serial.println(value);
}

void setup() {
  Serial.begin(115200);
  dmx.beginReceiver(1, onFrame);
}

void loop() {
  delay(10);
}
```

## API Reference

### Initialization

#### `bool beginSender(uint8_t channel = 1)`
Initialize as DMX sender.
- `channel`: WiFi channel (1-13)
- Returns: `true` if successful

#### `bool beginReceiver(uint8_t channel = 1, DMXFrameCallback callback = nullptr)`
Initialize as DMX receiver.
- `channel`: WiFi channel (must match sender)
- `callback`: Function called when complete frame received
- Returns: `true` if successful

### Sender Methods

#### `void setChannel(uint16_t channel, uint8_t value)`
Set a single DMX channel value.
- `channel`: DMX channel (0-511)
- `value`: DMX value (0-255)

#### `void setChannels(uint16_t startChannel, uint8_t* values, uint16_t count)`
Set multiple consecutive DMX channels.

#### `void sendFrame()`
Manually send DMX frame immediately.

#### `void setRefreshRate(uint16_t hz)`
Enable automatic sending at specified rate.
- `hz`: Refresh rate in Hz (0 = manual mode)

#### `void update()`
Call in `loop()` when using automatic refresh rate.

### Receiver Methods

#### `uint8_t getChannel(uint16_t channel)`
Read a single DMX channel value.

#### `void getChannels(uint16_t startChannel, uint8_t* buffer, uint16_t count)`
Read multiple consecutive DMX channels.

#### `void setFrameCallback(DMXFrameCallback callback)`
Set or change the frame complete callback.

### Statistics

#### `uint32_t getFrameCount()`
Get total frames received.

#### `uint32_t getLostFrames()`
Get number of lost frames.

#### `float getLossRate()`
Get frame loss rate as percentage.

#### `void printStatistics()`
Print statistics to Serial.

#### `void resetStatistics()`
Reset all statistics counters.

### Advanced

#### `uint8_t* getDMXBuffer()`
Get direct access to the DMX data buffer (advanced use).

## Examples

See the `examples` folder for:
- **Sender**: Basic sender with automatic refresh
- **SenderManual**: Manual frame sending with custom timing
- **Receiver**: Basic receiver with callback
- **ReceiverWithLEDs**: Control WS2812B LED strips

## Technical Details

- **Packet Size**: ESP-NOW v1 supports 250 bytes per packet
- **Universe Split**: 512 channels split into 3 packets (170+170+172)
- **Sequence Numbers**: Automatic frame loss detection
- **Broadcast Mode**: Uses FF:FF:FF:FF:FF:FF (no pairing needed)
- **Default Refresh**: 50 Hz (configurable)

## WiFi Channel

Both sender and receiver must be on the **same WiFi channel**. Use channels 1, 6, or 11 for best results in crowded environments.

## Troubleshooting

### No data received
- Check both devices are on same WiFi channel
- Verify Serial output shows MAC addresses
- Check devices are within range (~100m line of sight)

### High frame loss
- Reduce distance between devices
- Change to less crowded WiFi channel
- Check for interference from other 2.4GHz devices

### Compile errors
- Ensure you selected an ESP32 board (not ESP8266)
- Update ESP32 board package to latest version

## License

MIT License - See LICENSE file for details

## Contributing

Contributions welcome! Please open an issue or pull request.

## Support

For issues and questions: https://github.com/yourusername/DMX_ESPNOW/issues
