# SML Web Interface v2.1 - WebSocket API Reference

**Smart Music Lamp** - WebSocket communication protocol

---

## Overview

The SML Web Interface uses WebSocket for real-time bidirectional communication between the web client and ESP32.

**Connection Details:**
- **URL:** `ws://<ESP32-IP>/ws` (or `wss://` for HTTPS)
- **Protocol:** WebSocket (RFC 6455)
- **Format:** JSON messages
- **Update Rate:**
  - Status updates: 1 Hz (every 1 second)
  - LED preview data: 20 Hz (every 50ms)
  - Adaptive based on effect type

---

## Message Format

All messages are JSON-encoded strings.

### Client → Server (Actions)

```json
{
  "action": "actionName",
  "param1": "value1",
  "param2": "value2"
}
```

### Server → Client (Updates)

```json
{
  "key1": "value1",
  "key2": "value2",
  "key3": "value3"
}
```

**Special Case:** LED preview data has different format (see below).

---

## Client → Server Actions

### Toggle LED Strip Power

**Action:** `toggle`

**Parameters:** None

**Example:**
```json
{"action": "toggle"}
```

**Response:** None (state updates via status messages)

---

### Set Solid Color

**Action:** `solid`

**Parameters:**
- `r`: Red channel (0-255)
- `g`: Green channel (0-255)
- `b`: Blue channel (0-255)

**Example:**
```json
{
  "action": "solid",
  "r": 255,
  "g": 0,
  "b": 0
}
```

**Response:** LEDs set to solid red

---

### Set Effect

**Action:** `effect`

**Parameters:**
- `effectId`: Effect number (1-20)

**Effect ID Mapping:**
1. Moving Dot
2. Rainbow Beat
3. Red White Blue
4. Ripple
5. Fire
6. Twinkle
7. Bouncing Balls
8. Juggle
9. Sinelon
10. Comet
11. Breath
12. Color Sweep
13. Rainbow VU
14. Old Skool VU
15. Rainbow Hue VU
16. Ripple VU
17. 3 Bars VU
18. Ocean VU
19. Temperature
20. Battery

**Example:**
```json
{
  "action": "effect",
  "effectId": 5
}
```

**Response:** Fire effect starts

---

### Set Brightness

**Action:** `brightness`

**Parameters:**
- `brightness`: Brightness level (0-255)

**Example:**
```json
{
  "action": "brightness",
  "brightness": 180
}
```

**Response:** Brightness updated to 180

---

### Toggle Lamp Power

**Action:** `lamp`

**Parameters:** None

**Example:**
```json
{"action": "lamp"}
```

**Response:** Main lamp relay toggles

---

### Toggle Bluetooth Power

**Action:** `bluetooth`

**Parameters:** None

**Example:**
```json
{"action": "bluetooth"}
```

**Response:** Bluetooth module relay toggles

---

### Set VU Meter Effect

**Action:** `vu1` through `vu6`

**Parameters:** None

**Example:**
```json
{"action": "vu1"}
```

**Response:** Rainbow VU effect activated

---

### Bluetooth Playback Controls

**Actions:**
- `play`: Play/Pause toggle
- `volup`: Volume Up (short press) / Fast Forward (long press)
- `voldown`: Volume Down (short press) / Rewind (long press)

**Example:**
```json
{"action": "play"}
```

**Response:** Bluetooth playback toggles

---

## Server → Client Updates

### General Status Update

**Frequency:** Every 1 second (when Peek tab NOT active)

**Format:**
```json
{
  "bars": "waveStrength-4",
  "battVoltage": 3.85,
  "level": 75,
  "charging": false,
  "fullbatt": false,
  "temperature": 23.5,
  "humidity": 45.2,
  "lampstatus": "on",
  "neostatus": "on",
  "btstatus": "on",
  "neobrightness": 150,
  "ssid": "MyWiFi",
  "ip": "192.168.1.100",
  "rssi": -45,
  "color": {
    "r": 255,
    "g": 128,
    "b": 0
  },
  "fireStatus": "off",
  "movingdotStatus": "on",
  // ... (all 20 effects)
}
```

**Field Descriptions:**

| Field | Type | Description |
|-------|------|-------------|
| `bars` | string | WiFi signal icon class |
| `battVoltage` | float | Battery voltage (V) |
| `level` | int | Battery percentage (0-100) |
| `charging` | bool | True if charging |
| `fullbatt` | bool | True if fully charged |
| `temperature` | float | Temperature (°C) |
| `humidity` | float | Humidity (%) |
| `lampstatus` | string | "on" or "off" |
| `neostatus` | string | "on" or "off" |
| `btstatus` | string | "on" or "off" |
| `neobrightness` | int | Brightness (0-255) |
| `ssid` | string | WiFi network name |
| `ip` | string | ESP32 IP address |
| `rssi` | int | WiFi signal strength (dBm) |
| `color` | object | Current RGB color |
| `*Status` | string | Each effect status ("on"/"off") |

---

### LED Preview Data (Peek Tab)

**Frequency:** Every 50ms (20 FPS)

**Detection:** Message contains `leds` array

**Format:**
```json
{
  "leds": [
    {"r": 255, "g": 0, "b": 0},
    {"r": 200, "g": 50, "b": 0},
    {"r": 150, "g": 100, "b": 0},
    // ... (up to 60 LEDs)
  ],
  "realCount": 24,
  "previewCount": 24,
  "effect": "Fire"
}
```

**Field Descriptions:**

| Field | Type | Description |
|-------|------|-------------|
| `leds` | array | LED colors (max 60) |
| `realCount` | int | Actual LED count (1-500) |
| `previewCount` | int | Preview LED count (≤60) |
| `effect` | string | Current effect name |

**LED Array Format:**
Each element is an object with `r`, `g`, `b` values (0-255).

**Sampling Logic:**
- If `realCount ≤ 60`: All LEDs sent
- If `realCount > 60`: Equidistant sampling
  - Formula: `ledIndex = (i * realCount) / previewCount`
  - Example: 100 real LEDs → Preview shows every 1.67th LED

---

## WebSocket Lifecycle

### Connection

1. **Client connects:** `ws://<ESP32-IP>/ws`
2. **ESP32 accepts:** Connection established
3. **Client receives:** Immediate status update
4. **Periodic updates:** Begin every 1 second

### Disconnection

1. **Client detects close event**
2. **Status bar shows:** "Disconnected"
3. **Auto-reconnect:** Exponential backoff (1s, 2s, 4s, 8s, 15s)
4. **Max attempts:** 10 before manual reconnect

### Reconnection

**Backoff Intervals:**
- Attempt 1: 1 second
- Attempt 2: 2 seconds
- Attempt 3: 4 seconds
- Attempt 4: 8 seconds
- Attempt 5+: 15 seconds

**After Max Attempts:**
- Status: "Disconnected - Max attempts reached"
- User action: Refresh page required

---

## Error Handling

### Client-Side Errors

**Malformed JSON:**
- Logged to console
- Message ignored
- No reconnection triggered

**Missing Fields:**
- Default values used
- Example: Missing `brightness` → 130

### Server-Side Errors

**Buffer Overflow:**
- Payload >4KB → Message dropped
- Logged to serial monitor
- Client continues normally

**Mutex Timeout:**
- LED data access blocked
- Status update skipped
- Next attempt in 1 second

---

## Performance Considerations

### Bandwidth Usage

**Status Updates (1 Hz):**
- Size: ~800 bytes
- Bandwidth: 0.8 KB/s

**LED Updates (20 Hz):**
- Size (60 LEDs): ~2KB
- Bandwidth: 40 KB/s

**Total:**
- Idle (no Peek): 0.8 KB/s
- Active (Peek tab): 40.8 KB/s

### CPU Usage (ESP32)

**WebSocket Processing:**
- JSON serialization: ~5ms
- Network send: ~10ms
- LED data sampling: ~2ms

**Total:** ~17ms per update (20 FPS achievable)

### Client-Side Performance

**Canvas Rendering:**
- Target: 60 FPS (desktop), 30 FPS (mobile)
- Monitor via FPS counter in Peek tab

**WebSocket Handler:**
- JSON parsing: <1ms
- LEDPreview update: <5ms
- UI updates: <2ms

---

## Security Considerations

### Authentication
**Current:** No authentication (open network)

**Recommendations:**
- Use WPA2/WPA3 WiFi encryption
- Disable AP mode after setup
- Implement API token (future)

### Input Validation

**Server-Side:**
- Effect ID: Range check (1-20)
- Brightness: Range check (0-255)
- RGB values: Range check (0-255)
- LED count: Range check (1-500)

**Client-Side:**
- HTML form validation
- Password strength checker
- XSS protection (text content only)

---

## Debugging

### Enable WebSocket Logging

**Client-Side (Browser Console):**
```javascript
window.DEBUG_MODE = true;
```

**Server-Side (ESP32):**
Already enabled in `main.cpp`:
```cpp
#define DEBUG_WEBSOCKET
```

### Serial Monitor Output

**LED Update Message:**
```
LED update sent: 60 LEDs, 2048 bytes
```

**WebSocket Connection:**
```
WebSocket client #0 connected
```

**Error Messages:**
```
Failed to acquire mutex for WebSocket update
JSON payload too large for WebSocket
```

---

## Future Enhancements

### Planned Features

1. **Binary Protocol:** MessagePack for 50% size reduction
2. **Compression:** WebSocket per-message compression
3. **Authentication:** API token for secure access
4. **OTA Updates:** WebSocket-triggered firmware updates
5. **Config Sync:** Save/load settings via WebSocket

### Performance Improvements

1. **Adaptive FPS:** Lower frequency for static effects
2. **Delta Updates:** Send only changed LED states
3. **Batching:** Combine multiple status updates

---

## Example Client Implementation

### JavaScript (Browser)

```javascript
const ws = new WebSocket('ws://192.168.1.100/ws');

ws.onmessage = (event) => {
  const data = JSON.parse(event.data);

  if (data.leds) {
    // LED preview data
    updatePreview(data.leds, data.effect);
  } else {
    // Status update
    updateStatus(data);
  }
};

// Send action
function setEffect(effectId) {
  ws.send(JSON.stringify({
    action: 'effect',
    effectId: effectId
  }));
}
```

### Python (Script)

```python
import websocket
import json

def on_message(ws, message):
    data = json.loads(message)
    print(data)

def on_error(ws, error):
    print(error)

def on_open(ws):
    # Set Fire effect
    ws.send(json.dumps({
        "action": "effect",
        "effectId": 5
    }))

ws = websocket.WebSocketApp("ws://192.168.1.100/ws",
    on_message=on_message,
    on_error=on_error,
    on_open=on_open)

ws.run_forever()
```

---

**Version:** 2.1.0
**Last Updated:** 2026-05-10
**Protocol Version:** 1.0

---

**For user guide:** See `docs/web-interface-user-guide.md`
