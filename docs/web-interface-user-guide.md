# SML Web Interface v2.1 - User Guide

**Smart Music Lamp** - Complete web interface control

---

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Tab-by-Tab Guide](#tab-by-tab-guide)
4. [Peek Tab Feature](#peek-tab-feature)
5. [Configuration](#configuration)
6. [Troubleshooting](#troubleshooting)

---

## Introduction

The SML Web Interface v2.1 provides a modern, responsive control panel for your Smart Music Lamp. Accessible from any device with a web browser, it allows you to:

- Control LED effects and colors in real-time
- Monitor battery status and environmental data
- Preview LED animations on-screen
- Configure WiFi and LED hardware settings
- Control Bluetooth playback

**New in v2.1:**
- ✨ **Peek Tab:** Real-time LED visualization on Canvas
- 📱 **Responsive Design:** Mobile, tablet, and desktop optimized
- 🎨 **WLED-Inspired UI:** Modern dark theme with smooth animations
- ⚡ **Performance:** 20 FPS WebSocket LED streaming

---

## Getting Started

### Accessing the Interface

1. **Connect to SML WiFi:**
   - First time: Connect to `SML_Setup` WiFi network
   - Configure your home WiFi in the Config tab
   - Reconnect to your home network

2. **Open Web Browser:**
   - Navigate to: `http://sml.local` (mDNS) or `http://<ESP32-IP>`
   - Example: `http://192.168.1.100`

3. **Interface Loads:**
   - Status bar shows WiFi signal and battery
   - Header displays "Smart Music Lamp" with glowing animation
   - Default tab: Lamp

---

## Tab-by-Tab Guide

### Lamp Tab

**Controls LED strip power, color, and effects.**

#### Power Control
- **Lamp Power:** Toggle main lamp relay (IN1)
- **Neo Power:** Toggle LED strip on/off

#### Color Picker
- Click the color wheel to open iro.js color picker
- Drag to select hue
- Adjust brightness slider below
- LEDs update in real-time

#### Effects Dropdown
Select from 20 different LED effects:

**Static Effects:**
- **Moving Dot:** Single dot travels across strip
- **Comet:** Trailing comet animation
- **Breath:** Smooth pulsing brightness

**Dynamic Effects:**
- **Fire:** Realistic flame effect
- **Rainbow Beat:** Rainbow cycles to beat
- **Ripple:** Water ripple animation
- **Twinkle:** Random star twinkling

**Music-Reactive Effects:**
- **Rainbow VU:** Music visualization with rainbow colors
- **Old Skool VU:** Classic VU meter style
- **Ocean VU:** Blue ocean wave visualization
- **3 Bars VU:** Three-band frequency display

**Temperature/Humidity:**
- **Temp Based:** Color changes with temperature
- **Battery Based:** Color indicates battery level

**Long-press** effect button (desktop) or tap gear icon (mobile) to configure effect parameters:
- Speed (intensity)
- Color palette
- Direction
- Size/count

---

### Music Tab

**Bluetooth audio and VU meter controls.**

#### Bluetooth Power
- Toggle Bluetooth module power relay
- Auto-connects to last paired device

#### VU Meter Selection
Choose from 6 VU meter effects:
- **Rainbow:** Colorful spectrum analyzer
- **Old:** Classic green/red VU meter
- **Hue:** Single color hue cycling
- **Ripple:** Ripple effect to music
- **3 Bars:** Bass, mid, treble separation
- **Ocean:** Blue wave visualization

#### Playback Controls
- **Play/Pause:** Toggle music playback
- **Volume Up/Down:** Adjust Bluetooth volume
- **Fast Forward/Rewind:** Skip tracks (long press)

---

### Peek Tab (NEW!)

**Real-time LED visualization on HTML5 Canvas.**

#### Features
- **Live Preview:** See exact LED colors and animations
- **Strip/Circle Modes:** Switch between layouts
- **LED Count:** Preview up to 60 LEDs (equidistant sampling)
- **Performance Monitor:** FPS counter, effect name, LED count

#### Controls

**Mode Toggle:**
- **Strip:** Horizontal LED arrangement (default)
- **Circle:** Circular LED arrangement (ideal for ring lights)

**LED Count Selector:**
- Options from your real LED count up to 60 LEDs
- Example: 24 real LEDs → 24, 25, 26... 60 options
- Higher count = more detailed preview

**Info Overlay:**
- Effect name (e.g., "Fire", "Rainbow Beat")
- LED count (real vs preview)
- FPS counter (target: 15-20 FPS)

#### Technical Details
- **Max 60 LEDs:** Web preview limited to 60 LEDs
- **Equidistant Sampling:** If you have 100+ LEDs, preview shows every 1.67th LED
- **20 FPS Streaming:** WebSocket updates at 50ms intervals
- **Adaptive FPS:** Static effects update less frequently

---

### Weather Tab

**Environmental monitoring with DHT22 sensor.**

#### Display
- **Temperature:** Current temperature in °C
- **Humidity:** Relative humidity percentage
- **Clock:** Real-time clock with AM/PM indicator

#### Auto-Update
Readings update every 3 seconds via WebSocket.

---

### Battery Tab

**Battery monitoring and charging status.**

#### Display
- **Battery Level:** Percentage (0-100%)
- **Voltage:** Current battery voltage
- **Charging Status:**
  - ⚡ **Charging:** Connected to mains power
  - ✅ **Full:** Battery fully charged
  - 🔋 **Battery:** Running on battery power
  - ⚠️ **Low:** Battery below 20%

#### Battery Animation
- Liquid fill animation shows level
- Bubble animation during charging
- Color changes: Green (full) → Yellow (medium) → Red (low)

---

### Config Tab

**WiFi, LED hardware, and system configuration.**

#### WiFi Configuration

**Current Connection:**
- SSID: Current WiFi network name
- Signal: Signal strength in dBm
- IP: ESP32 IP address

**Change WiFi:**
1. **New SSID (optional):** Leave empty to keep current network
2. **New Password:** Enter new WiFi password (min 8 chars)
3. **Password Strength Indicator:**
   - 🔴 Weak: <8 chars or no numbers/symbols
   - 🟡 Medium: 8+ chars with numbers
   - 🟢 Strong: 8+ chars with numbers and symbols
4. **Save & Restart:** ESP32 reboots, reconnects to new network

**⚠️ Warning:** ESP32 restarts after saving. Wait 30 seconds for reconnection.

---

#### LED Hardware Configuration

**Real LED Count:**
- Actual number of LEDs connected to strip (1-500)
- Default: 24 LEDs
- Used for internal LED array allocation

**Max Web LEDs:**
- Maximum LEDs controllable from web interface (1-500)
- Default: 24 LEDs
- **Max 60** for Peek tab preview (enforced)

**Default Brightness:**
- Initial brightness level (0-255)
- Default: 130 (~50%)
- Can be adjusted from Lamp tab

**Save LED Config:** Applies immediately, no restart required.

---

#### Theme Selection

**Choose your preferred color scheme.**

The SML Web Interface v2.1 includes 3 color themes inspired by WLED:

**Available Themes:**

1. **WLED Classic** (Default)
   - Colors: Cyan and blue
   - Style: Technical LED aesthetic
   - Best for: Programming effects, technical control

2. **Sunset**
   - Colors: Gold and orange
   - Style: Warm lamp ambiance
   - Best for: Daily home use, cozy atmosphere

3. **Ocean**
   - Colors: Blue and purple
   - Style: Musical environment
   - Best for: Music mode, visualizations, parties

**How to Change Theme:**

1. Navigate to **Config Tab**
2. Scroll to **🎨 Appearance** section
3. Click on any theme card to apply
4. Theme saves automatically to your browser

**Theme Persistence:**

- Theme selection is saved in your browser's local storage
- Persists across page reloads and browser restarts
- Each device/browser can have its own theme
- Private/Incognito mode: Theme only for current session

**Visual Preview:**

Each theme card shows a mini preview of the color scheme, helping you see how the interface will look before applying.

---

#### Help & Tips

**FAQ Cards:**

1. **How to update firmware?**
   - Click "Firmware Updates" → "Check for Updates"
   - Or navigate to `http://<ESP32-IP>/update`

2. **Reset to defaults?**
   - Power cycle ESP3 3 times quickly
   - Factory settings restored

3. **LED strip not working?**
   - Check Neo Power is ON
   - Verify 5V power supply connected
   - Check data pin connection (D4)

4. **WiFi not connecting?**
   - Verify SSID and password
   - Check 2.4GHz only (5GHz not supported)
   - Restart ESP32 by power cycling

5. **Battery drains fast?**
   - Check for faulty USB cable
   - Reduce LED brightness
   - Use AC power when possible

---

#### System Info

**Display:**
- **Firmware Version:** Current SML firmware
- **IP Address:** ESP32 local network address
- **MAC Address:** Unique hardware identifier
- **Uptime:** Time since last reboot
- **Free Heap:** Available RAM (bytes)
- **WiFi Mode:** Station (connects to router)

---

## Troubleshooting

### Interface Issues

**Page won't load:**
- Verify you're on the same network as ESP32
- Try `http://<ESP32-IP>` instead of mDNS
- Check ESP32 is powered on (LED blinking)

**WebSocket won't connect:**
- Refresh the page
- Check browser console (F12) for errors
- Verify ESP32 is running (serial monitor)

**Controls not responsive:**
- Wait for WebSocket connection (check status bar)
- Clear browser cache (Ctrl+Shift+Del)
- Try different browser (Chrome, Firefox, Safari)

---

### LED Issues

**LEDs not lighting:**
- Check Neo Power is ON
- Verify brightness >0
- Test different effect (try "Fire")
- Check 5V power supply connection

**Wrong colors:**
- Verify color order in code (GRB for WS2812B)
- Test with solid color (255, 0, 0 = red)
- Check for loose data wire connections

**Stuttering animation:**
- Reduce LED count (Max Web LEDs)
- Lower brightness setting
- Check WiFi signal strength (low signal = lag)

---

### Battery Issues

**Battery percentage stuck:**
- Recalibrate by fully charging
- Drain to 10%, then full charge
- Check CONV_FACTOR in code

**Charging but not full:**
- Verify TP4056 blue LED turns on
- Check battery voltage reaches 4.0V-4.2V
- Replace battery if old (>2 years)

---

### WiFi Issues

**Can't find SML WiFi:**
- Press and hold ESP32 reset button
- Look for "SML_Setup" network
- Factory reset: power cycle 3 times

**Connection drops:**
- Move ESP32 closer to router
- Check for interference (microwave, other WiFi)
- Restart router if needed

---

## Advanced Features

### mDNS Access
Use `http://sml.local` instead of IP address (works on Bonjour-enabled networks).

### OTA Updates
Update firmware wirelessly:
1. Navigate to `http://<ESP32-IP>/update`
2. Upload `.bin` file from PlatformIO build
3. ESP32 restarts automatically

### Serial Monitor
Debug via USB:
- baud rate: 115200
- View LED updates, WiFi status, battery info

---

## Tips & Tricks

1. **Speed Up Effects:** Increase effect speed parameter
2. **Save Battery:** Use lower brightness (50-100)
3. **Better Preview:** Set LED count to 60 for smoother animation
4. **Quick Access:** Bookmark ESP32 IP in browser
5. **Mobile Shortcut:** Add to home screen (iOS: Share → Add to Home Screen)

---

## Support

**Issues? Bug reports? Feature requests?**
- GitHub: [https://github.com/alexminator/SML](https://github.com/alexminator/SML)
- Documentation: `docs/web-interface-api.md` (WebSocket protocol)

**Version:** 2.1.0
**Last Updated:** 2026-05-10

---

**Enjoy your Smart Music Lamp!** 🎵💡
