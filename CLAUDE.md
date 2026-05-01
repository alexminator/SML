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
