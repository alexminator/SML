# Smart Music Lamp (SML) - Project Documentation

## Project Overview

**SML (Smart Music Lamp)** is an IoT project based on ESP32 that transforms a conventional lamp into a smart device with multiple features:

- **WiFi Control**: Control lamp power via web interface from mobile devices
- **Bluetooth Audio**: Integrated Bluetooth speaker for music playback
- **RGB Lighting**: WS2812B LED strip with customizable effects and music visualization
- **Environmental Monitoring**: DHT22 temperature and humidity sensor
- **Dual Power Supply**: Mains power + Li-ion battery backup with automatic switching
- **Battery Management**: Real-time battery monitoring and charging status
- **Web Interface**: Embedded async web server with real-time WebSocket updates

## Hardware Configuration

### ESP32 Pin Assignments

| Pin | Function | Description |
|-----|----------|-------------|
| D4 (4) | STRIP_PIN | WS2812B LED strip data line |
| D5 (5) | VOLUMENUP_PIN | Bluetooth Volume Up / Fast Forward |
| D18 (18) | SWITCH_PIN | Bluetooth module power relay |
| D19 (19) | VOLUMENDOWN_PIN | Bluetooth Volume Down / Rewind |
| D21 (21) | PLAY_PIN | Bluetooth Play/Pause |
| D23 (23) | DHTPIN | DHT22 temperature/humidity sensor |
| D32 (32) | LAMP_PIN | Lamp power relay (IN1) |
| D33 (33) | ADC_PIN | Battery voltage monitoring |
| D34 (34) | CHARGE_PIN | TP4056 charging status (active LOW) |
| D35 (35) | FULL_CHARGE_PIN | TP4056 full charge status (active LOW) |
| D36 (VP) | AUDIO_IN_PIN | Audio input for VU effects |

### Important Hardware Notes

- **ADC2 pins cannot be used for analog reads** when WiFi is enabled
- All control buttons use MOSFETs to ground (active LOW logic)
- Battery charging uses TP4056 module with status LEDs
- Audio input uses voltage divider to ESP32 ADC
- Load sharing circuit prevents battery drain when mains power is connected

## Project Structure

```
SML/
├── src/                    # Main source code
│   ├── main.cpp           # Main application file
│   ├── common.h           # Common definitions and utilities
│   ├── data.h             # Data structures and WebSocket handling
│   ├── debug.h            # Debug logging utilities
│   ├── *.h                # Effect headers (MovingDot, Fire, Twinkle, etc.)
│   └── vu*.h              # VU meter effect implementations (vu1-vu6)
├── data/                   # Web interface files (SPIFFS/LittleFS)
│   ├── index.html         # Main web interface
│   ├── css/               # Stylesheets
│   ├── js/                # JavaScript files
│   ├── fonts/             # Font files
│   └── img/               # Images and icons
├── include/               # Additional include files
├── lib/                   # Project-specific libraries
├── platformio.ini         # PlatformIO configuration
├── README_es.md          # Spanish documentation
└── README.md             # English documentation
```

## Web Interface v2.1 Architecture (2026 Redesign)

### Overview
SML Web Interface v2.1 is a complete rewrite inspired by WLED, featuring a modern responsive design with real-time LED visualization.

### File Structure (data/)

```
data/
├── index.html              # Main HTML (463 lines, 6 tabs)
├── css/
│   ├── wled-theme.css      # CSS variables & base styles (125 lines)
│   ├── sml-custom.css      # SML-specific styles (242 lines)
│   ├── responsive-nav.css  # Mobile/tablet/desktop navigation (235 lines)
│   ├── effects-config.css  # Effect configuration panels (193 lines)
│   ├── config-tab.css      # Config tab styles (450+ lines)
│   └── peek-preview.css    # Peek tab canvas styles (186 lines)
├── js/
│   ├── tabs-manager.js     # Tab switching & sidebar (262 lines)
│   ├── effects-handler.js  # Effect selection & config (669 lines)
│   ├── peek-render.js      # LED preview Canvas rendering (293 lines)
│   ├── config-manager.js   # WiFi & LED config (390+ lines)
│   └── main.js             # WebSocket routing (365 lines)
└── fonts/                  # FontAwesome icons
```

### Responsive Navigation Breakpoints

**Mobile (< 768px):**
- Bottom navigation bar
- 6 tabs: Lamp, Music, Peek, Weather, Battery, Config
- Full-width tab content
- Compact status bar (44px)

**Tablet (768-1023px):**
- Collapsed sidebar (60px width)
- Icon-only navigation
- Toggle button to expand
- Adjusted header centering

**Desktop (≥1024px):**
- Expanded sidebar (200px width)
- Icon + text navigation
- Collapsible via toggle button
- Maximum canvas space for Peek tab

### Tab Architecture

**Tab Order (Sidebar):**
1. **Lamp:** Power, color picker, effects dropdown, effect config
2. **Music:** Bluetooth power, VU meter selection, playback controls
3. **Peek:** Real-time LED visualization (NEW in v2.1)
4. **Weather:** Temperature, humidity, clock
5. **Battery:** Battery level, voltage, charging status
6. **Config:** WiFi setup, LED hardware config, Help & Tips, System info

### Peek Tab Implementation

**Purpose:** Real-time LED strip visualization on HTML5 Canvas

**Technical Details:**
- **Rendering:** Canvas 2D API with requestAnimationFrame (60 FPS)
- **Modes:** Strip (horizontal) or Circle (circular) arrangement
- **LED Count:** Selector from real LED count up to 60 LEDs
- **Data Source:** WebSocket streaming at 20 FPS (50ms interval)
- **Sampling:** Equidistant sampling for >60 real LEDs

**MAX 60 LED Limit:**
- Web preview capped at 60 LEDs to prevent ESP32 overload
- If `N_PIXELS ≤ 60`: Shows all LEDs
- If `N_PIXELS > 60`: Shows every `(N_PIXELS / 60)`th LED
- Example: 100 real LEDs → Preview shows LEDs 0, 2, 4, 6... (equidistant)

**WebSocket LED Data Format:**
```json
{
  "leds": [
    {"r": 255, "g": 0, "b": 0},
    {"r": 200, "g": 50, "b": 0},
    // ... (max 60 LEDs)
  ],
  "realCount": 24,
  "previewCount": 24,
  "effect": "Fire"
}
```

**ESP32 Implementation (main.cpp):**
```cpp
#define MAX_WEB_LEDS 60  // Maximum LEDs for web preview

void sendLEDUpdate() {
  // Equidistant sampling formula
  const uint16_t realLedIndex = (i * realCount) / previewCount;

  // Create JSON with LED array
  JsonArray ledArray = json["leds"].to<JsonArray>();
  // Add RGB values...

  // Send via WebSocket
  ws.textAll(buffer, len);
}
```

### WebSocket Communication

**Protocol:** WebSocket (RFC 6455) over HTTP
**URL:** `ws://<ESP32-IP>/ws`
**Format:** JSON messages

**Update Rates:**
- Status updates: 1 Hz (WiFi, battery, temperature, etc.)
- LED preview data: 20 Hz (when Peek tab active)
- Adaptive based on effect type (static effects = lower frequency)

**Message Types:**

1. **Client → Server (Actions):**
```json
{"action": "toggle"}        // Toggle LED power
{"action": "solid", "r": 255, "g": 0, "b": 0}  // Set color
{"action": "effect", "effectId": 5}  // Set effect
{"action": "brightness", "brightness": 180}  // Set brightness
```

2. **Server → Client (Status Updates):**
```json
{
  "bars": "waveStrength-4",
  "battVoltage": 3.85,
  "level": 75,
  "charging": false,
  "temperature": 23.5,
  "humidity": 45.2,
  "lampstatus": "on",
  "neostatus": "on",
  "neobrightness": 150,
  "color": {"r": 255, "g": 128, "b": 0}
}
```

3. **Server → Client (LED Preview Data):**
```json
{
  "leds": [{"r": 255, "g": 0, "b": 0}, ...],
  "realCount": 24,
  "previewCount": 24,
  "effect": "Fire"
}
```

### Effect System

**20 Effects Total:**

**ID 1-10 (Standard Effects):**
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

**ID 11-12 (Breath Effects):**
11. Breath
12. Color Sweep

**ID 13-18 (VU Meter Effects):**
13. Rainbow VU
14. Old Skool VU
15. Rainbow Hue VU
16. Ripple VU
17. 3 Bars VU
18. Ocean VU

**ID 19-20 (Sensor-Based):**
19. Temperature (color based on temp)
20. Battery (color based on charge level)

**Effect Configuration:**
- Desktop: Long-press effect button → Side panel slides in
- Mobile: Tap gear icon → Bottom sheet slides up
- Parameters: Speed, intensity, color, size, direction

### Performance Optimizations

**Target Metrics:**
- Canvas FPS: ≥30 (desktop), ≥20 (mobile)
- WebSocket CPU: <10% ESP32 time
- Page load: <2s First Contentful Paint
- Mobile touch: <50ms response

**Implemented:**
- Exponential backoff WebSocket reconnection (1s, 2s, 4s, 8s, 15s)
- Adaptive LED update frequency (10-20 FPS based on effect)
- requestAnimationFrame for smooth Canvas rendering
- Equidistant LED sampling for large arrays
- CSS transitions for smooth UI animations

**Planned (Future):**
- Canvas gradient caching
- Compact JSON format (30% smaller)
- Gzip compression (75% size reduction)
- FontAwesome subsetting (100KB → 10KB)

### Preserved SML Branding

**Retained from v1.0:**
- ✅ "Handmade" font for "Smart Music Lamp" text
- ✅ SML glowing animation (RGB color cycling)
- ✅ Slider styling
- ✅ Thermometer animation
- ✅ Battery liquid animation
- ✅ Bluetooth SVG player controls

**Removed from v1.0:**
- ❌ Butterfly decoration (completely removed)

**New in v2.1:**
- ✨ WLED-inspired dark theme
- ✨ Responsive navigation (mobile/tablet/desktop)
- ✨ Peek tab with real-time LED visualization
- ✨ Centralized Config tab
- ✨ Effect configuration panels
- ✨ MAX 60 LED preview limit

## Key Configuration Variables

### LED Strip Settings (main.cpp)
```cpp
#define N_PIXELS 24              // Number of LEDs in strip
#define STRIP_PIN 4              // LED strip data pin
int brightness = 130;           // Initial brightness (0-255)
#define MAX_MILLIAMPS 500       // Max current draw
```

### Battery Settings (main.cpp)
```cpp
#define MAXV 4.00               // Fully charged battery voltage
#define MINV 3.20               // Minimum safe voltage
#define CONV_FACTOR 1.702       // ADC to voltage conversion
#define BATT_THRESHOLD 30       // Low battery threshold (%)
```

### Web Server Settings (main.cpp)
```cpp
#define HTTP_PORT 80            // HTTP server port
const unsigned long refresh = 3000; // WebSocket update interval (ms)
```

### Debug Settings (main.cpp)
```cpp
#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING  // Enable/disable debug
// #define DHT              // Uncomment for DHT debug messages
// #define BATTERY          // Uncomment for battery debug messages
```

## Build and Flash

### Using PlatformIO
```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor

# Clean build files
pio run --target clean
```

### Upload Filesystem Image
```bash
# Upload web interface files to SPIFFS/LittleFS
pio run --target uploadfs
```

## Effect System

The project includes multiple LED effects organized by ID:

- **0**: Moving Dot
- **1**: Rainbow Beat
- **2**: Red White Blue
- **3**: Ripple
- **4**: Fire
- **5**: Twinkle
- **6**: Bouncing Balls
- **7**: Juggle
- **8**: Sinelon
- **9**: Comet
- **10**: Temperature-based color
- **11-16**: VU Meter effects (music visualization)

## Web Interface Features

- **Real-time updates**: WebSocket communication for instant status updates
- **Color picker**: Custom color selection using iro.js
- **Effect controls**: Select and configure LED effects
- **Battery monitoring**: Real-time battery level and charging status
- **Temperature/Humidity**: Environmental data display
- **Audio controls**: Bluetooth playback controls (Play/Pause, Volume, FF/Rew)
- **Lamp control**: Power toggle for main lamp
- **Bluetooth power**: Toggle Bluetooth module on/off
- **OTA updates**: ElegantOTA for over-the-air firmware updates

## Important Libraries

### PlatformIO Dependencies (platformio.ini)
- `ArduinoJson` - JSON serialization
- `ESPAsyncWebServer` - Async web server
- `AsyncTCP` - Async TCP for WebSockets
- `FastLED` - LED strip control
- `Battery18650Stats` - Custom battery monitoring (forked)
- `DHT sensor library` - Temperature/humidity sensor
- `ElegantOTA` - OTA updates

### Custom Libraries
- **lib/Battery18650Stats**: Modified version with configurable max/min voltages

## Common Issues and Solutions

### Battery Reading Issues
- Calibrate MAXV by measuring battery voltage when TP4056 shows full charge (blue LED)
- Set MINV to 3.2V for safe operation (DC-DC minimum input)
- Adjust CONV_FACTOR based on your voltage divider

### WiFi Connection Problems
- ESP32 operates in Station mode (not AP mode)
- Ensure credentials are correctly set in code
- Check that ADC2 pins are not used for analog reads when WiFi is active

### LED Strip Issues
- Check data pin connection (D4)
- Verify 5V power supply can handle LED current (500mA max in code)
- Ensure correct COLOR_ORDER (GRB for WS2812B)

### Audio Input Issues
- BIAS value (1850) may need adjustment for your ADC
- Set NOISE level to filter interference
- Check voltage divider connection to D36 (VP)

### OTA Updates
- Access OTA at: `http://<esp32-ip>/update`
- Ensure sufficient flash space for OTA partition
- Use ElegantOTA for web-based updates

## Development Notes

### CRITICAL: ESP32 Crash Patterns to Avoid

**⚠️ snprintf() with Format Specifiers CAUSES CRASHES**

**ROOT CAUSE:** Using `snprintf()` with format specifiers (`%d`, `%u`, `%.1f`, `%.3f`, etc.) causes ESP32 to crash with "Interrupt wdt timeout on CPU0" immediately after WiFi initialization.

**PROHIBITED PATTERNS:**
```cpp
// ❌ NEVER DO THIS - Causes crash
char msg[64];
snprintf(msg, sizeof(msg), "Value: %d", someInt);
debuglnD(msg);

// ❌ NEVER DO THIS - Causes crash
snprintf(buffer, sizeof(buffer), "%.3f", voltage);

// ❌ NEVER DO THIS - Causes crash
snprintf(msg, sizeof(msg), "RGB: %d, %d, %d", r, g, b);
```

**SAFE ALTERNATIVES:**

1. **For debug messages:** Use `debugD_NUM()` macro
```cpp
// ✅ SAFE - Use debugD_NUM macro
debugD("Value: ");
debuglnD_NUM(someInt, "%d");

// ✅ SAFE - Split into multiple calls
debugD("RGB: ");
debugD_NUM(r, "%d");
debugD(", ");
debugD_NUM(g, "%d");
debugD(", ");
debuglnD_NUM(b, "%d");
```

2. **For float to string:** Manual conversion without snprintf
```cpp
// ✅ SAFE - Manual float conversion
int volts_int = (int)voltage;
int volts_frac = (int)((voltage - volts_int) * 1000);
debugD("Voltage: ");
debugD_NUM(volts_int, "%d");
debugD(".");
debugD_NUM(volts_frac, "%03d");
debuglnD("V");
```

3. **For integer to string:** Use `itoa()` or direct assignment
```cpp
// ✅ SAFE - Use itoa()
itoa(value, buffer, 10);

// ✅ SAFE - Direct numeric in JSON
json["value"] = someInt;  // ArduinoJson handles conversion safely
```

4. **For WebSocket/JSON:** Use numeric values directly
```cpp
// ✅ SAFE - Let ArduinoJson handle formatting
json["temperature"] = tempValue;  // Not tempStr
json["voltage"] = battVolts;      // Not battVoltageStr
```

**WHY THIS HAPPENS:**
- ESP32 snprintf() implementation has issues with format string parsing
- Floating point formatting (%.1f, %.3f) is particularly unstable
- Crash occurs during startup when snprintf() is called in early initialization
- Error appears as "Interrupt wdt timeout on CPU0" after WiFi connects

**ENFORCEMENT:**
- **ALWAYS** grep for `snprintf` before committing changes
- **NEVER** use snprintf() with format specifiers in this codebase
- **USE** debugD_NUM() macro for all debug output with numbers
- **USE** ArduinoJson's native numeric handling for JSON
- **USE** itoa() for simple integer-to-string conversion

**History:**
- 2026-04-16: Multiple snprintf() calls caused crash after WiFi init (commit 81ec832 fixed)
- User explicitly warned about DHT sensor snprintf crash, but lesson was not applied globally

### Debugging
- Set `DEBUGLEVEL_DEBUGGING` for console output
- Uncomment `#define DHT` for sensor debugging
- Uncomment `#define BATTERY` for battery monitoring debugging
- Serial baud rate: 115200

### Adding New Effects
1. Create header file in `src/` (e.g., `MyEffect.h`)
2. Include in `main.cpp`
3. Add effect ID in web interface (`data/js/main.js`)
4. Implement effect function following existing patterns

### Web Interface Modifications
- HTML: `data/index.html`
- CSS: `data/css/styles.css`
- JavaScript: `data/js/main.js`
- Upload filesystem changes after modifications: `pio run --target uploadfs`

## Power Management

### Automatic Power Management System (2026-05-01)

The ESP32 implements an intelligent state machine-based power management system that automatically detects power source and optimizes power consumption.

**Power States:**
- **AC_MODE** (POWER_AC_MODE): Full operation - WiFi always ON, CPU 240MHz, Neopixel LED active
- **BATTERY_CONNECTING** (POWER_BATTERY_CONNECTING): Try WiFi 10s, wait 30s for WebSocket client
- **BATTERY_ACTIVE** (POWER_BATTERY_ACTIVE): Client connected - WiFi ON, CPU 240MHz, Neopixel OFF
- **BATTERY_SLEEP** (POWER_BATTERY_SLEEP): Power savings - 60s WiFi OFF / 10s WiFi ON cycle, CPU 80MHz

**State Transitions:**
- AC lost → Try connect 10s → Wait 30s for client → Sleep if no client
- Client connects → Immediate wake-up from any state
- Client disconnects → Wait 30s for reconnect → Sleep
- AC restored → Immediate return to full operation
- Battery < 15% → Force maximum power savings

**Power Detection:**
- Uses existing battery monitoring logic (fullBatt/chargeState from TP4056)
- 3-second debounce prevents rapid state changes from power flickering
- Automatic detection: AC power = charging OR full battery

**Hardware Optimization:**
- WiFi: `WiFi.setSleep(false)` for AC/ACTIVE, `WiFi.setSleep(true)` for SLEEP
- CPU: 240MHz (AC/ACTIVE), 80MHz (SLEEP)
- Neopixel: ON (AC only), OFF (battery modes)
- ESP32 LEDs: Always ON (built-in indicators)

**Expected Performance:**
- AC Mode: ~180mA (unchanged from original)
- Battery Active: ~120mA (33% reduction)
- Battery Sleep: ~25mA average (86% reduction from 180mA)
- Wake-up Time: < 1 second when client connects

**Debug Logging:**
- Enable with `#define DEBUG_POWER_MANAGEMENT` in `src/debug.h`
- Logs state transitions, power source changes, WiFi connection attempts
- Use `#ifdef DEBUG_POWER_MANAGEMENT` blocks for custom debug output

### Load Sharing Circuit
- Automatic switching between mains and battery power
- Schottky diode prevents backfeed to mains
- P-channel MOSFET disconnects battery when mains is present
- Battery charges normally when mains power is connected

### Battery Protection
- TP4056 provides overcharge protection
- DC-DC boost converter has low-voltage cutoff (~2V input)
- Software monitoring prevents deep discharge
- Automatic shutdown at low battery threshold

## Bluetooth Control

The ESP32 emulates button presses using MOSFETs:
- **Short press** (200ms): Play/Pause, Next/Prev track
- **Long press** (1000ms): Volume Up/Down, Fast Forward/Rewind

## Performance Considerations

- WebSocket supports up to 8 simultaneous connections
- Async web server handles multiple clients efficiently
- LED effects optimized for ESP32 performance
- Audio sampling uses efficient ring buffer
- Filesystem uses LittleFS for better flash management

## License

© 2023 Alexminator
See LICENSE file for details
