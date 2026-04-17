# Critical Bugs Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix 5 critical bugs in SML project using minimally invasive patches, maintaining all existing functionality

**Architecture:** Individual commits for each bug fix, preserving current code structure and web interface

**Tech Stack:** ESP32, PlatformIO, ArduinoJson, FreeRTOS (mutexes), ESPAsyncWebServer

---

## Task 1: Bug #5 - Replace Infinite Loop in WiFi Init with Timeout

**Files:**
- Modify: `src/main.cpp:482-487` (initLittleFS function)
- Modify: `src/main.cpp:477` (initLittleFS function start)

- [ ] **Step 1: Locate initLittleFS() function**

Read `src/main.cpp` and find the initLittleFS() function around line 477. Identify the infinite loop `while (1)` that blocks when LittleFS fails to mount.

- [ ] **Step 2: Replace infinite loop with timeout**

Find this code in `initLittleFS()`:
```cpp
if (!LittleFS.begin())
{
    debuglnD("Cannot mount LittleFS volume...");
    while (1)
    {
        onboard_led.on = millis() % 200 < 50;
        onboard_led.update();
    }
}
```

Replace with:
```cpp
if (!LittleFS.begin())
{
    debuglnD("Cannot mount LittleFS volume...");
    
    // Timeout instead of infinite loop
    unsigned long errorStartTime = millis();
    const unsigned long MAX_ERROR_TIME = 30000; // 30 seconds
    
    while (millis() - errorStartTime < MAX_ERROR_TIME)
    {
        onboard_led.on = millis() % 200 < 50;
        onboard_led.update();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Continue with setup
    debuglnE("LittleFS mount failed, using default WiFi credentials");
    debugW("Device will continue with limited functionality");
}
```

- [ ] **Step 3: Verify compilation**

Run: `pio run`
Expected: SUCCESS with 0 errors, 0 warnings

- [ ] **Step 4: Upload to ESP32**

Run: `pio run --target upload`
Expected: Firmware uploaded successfully

- [ ] **Step 5: Monitor serial output**

Run: `pio device monitor`
Expected: Boot messages appear, device continues even if LittleFS fails

- [ ] **Step 6: Test normal operation**

Verify: LittleFS mounts successfully on normal boot
Expected: LED does not flash error pattern, system continues normally

- [ ] **Step 7: Commit changes**

```bash
git add src/main.cpp
git commit -m "Fix #5: Replace infinite loop in WiFi init with timeout

- Replace infinite while(1) with 30-second timeout
- Add debug messages for LittleFS failure
- Allow system to continue with default WiFi credentials
- Prevents device from hanging on LittleFS failure"
```

---

## Task 2: Bug #1 - Move WiFi Credentials to Build-Time Configuration

**Files:**
- Create: `.env`
- Modify: `platformio.ini`
- Modify: `src/data.h`
- Modify: `.gitignore`
- Modify: `src/main.cpp` (initWiFi function debug messages)

- [ ] **Step 1: Create .env file with WiFi credentials**

Create file `.env` in project root:
```bash
WIFI_SSID=YourWiFiSSID
WIFI_PASS=YourWiFiPassword
```

Note: Replace with your actual WiFi credentials

- [ ] **Step 2: Add .env to .gitignore**

Add to `.gitignore`:
```
# Environment variables with sensitive data
.env
```

- [ ] **Step 3: Update platformio.ini with build flags**

Find the `build_flags` section in `platformio.ini` (around line 17).

Replace:
```ini
build_flags = -DELEGANTOTA_USE_ASYNC_WEBSERVER=1
```

With:
```ini
build_flags = 
    -DELEGANTOTA_USE_ASYNC_WEBSERVER=1
    -DDEFAULT_WIFI_SSID=\""$(WIFI_SSID)"\" 
    -DDEFAULT_WIFI_PASS=\""$(WIFI_PASS)"\"
```

- [ ] **Step 4: Update data.h to use build-time credentials**

Replace entire content of `src/data.h`:
```cpp
// WiFi credentials - set via build flags from .env file
// For development: create .env file with WIFI_SSID and WIFI_PASS
#ifndef DEFAULT_WIFI_SSID
#define DEFAULT_WIFI_SSID ""  
#endif
#ifndef DEFAULT_WIFI_PASS
#define DEFAULT_WIFI_PASS ""  
#endif

const char *WIFI_SSID = DEFAULT_WIFI_SSID;
const char *WIFI_PASS = DEFAULT_WIFI_PASS;
const char *WEB_NAME = "sml";
```

- [ ] **Step 5: Add debug messages to initWiFi()**

Find the `initWiFi()` function in `src/main.cpp` (around line 494).

Add these debug messages after WiFi connection attempts:

After line 526 (if WiFi connected with saved credentials):
```cpp
debuglnD("Using saved credentials from Preferences");
```

After line 548 (if WiFi connected with default credentials):
```cpp
debuglnD("Using default credentials from build configuration");
```

- [ ] **Step 6: Test compilation with .env**

Run: `pio run`
Expected: SUCCESS with 0 errors, 0 warnings

- [ ] **Step 7: Upload and test WiFi connection**

Run: `pio run --target upload`
Then: `pio device monitor`

Verify: Device connects to WiFi using credentials from .env
Expected: WiFi connects successfully, debug messages show which credentials were used

- [ ] **Step 8: Test web interface still works**

Open browser: `http://<esp32-ip>`
Verify: Web interface loads and works
Expected: All controls functional

- [ ] **Step 9: Test Preferences system still works**

Use web interface to change WiFi credentials
Verify: New credentials saved to NV memory
Expected: `/save-wifi` endpoint still works

- [ ] **Step 10: Commit changes**

```bash
git add .env .gitignore platformio.ini src/data.h src/main.cpp
git commit -m "Fix #1: Move WiFi credentials to build-time configuration

- Move default WiFi credentials from source to .env file
- Use PlatformIO build flags to inject credentials at compile time
- Add .env to .gitignore for security
- Maintain Preferences system for runtime credential changes
- Add debug messages to show which credentials are being used"
```

---

## Task 3: Bug #2 - Prevent WebSocket Buffer Overflow with Dynamic Sizing

**Files:**
- Modify: `src/main.cpp:795-799` (notifyClients function)
- Modify: `src/main.cpp:936-944` (TaskWebSocket function)

- [ ] **Step 1: Locate notifyClients() buffer allocation**

Find the `notifyClients()` function in `src/main.cpp` (around line 748).
Locate the fixed-size buffer declaration around line 795:
```cpp
char buffer[768];
size_t len = serializeJson(json, buffer, sizeof(buffer));
```

- [ ] **Step 2: Replace with dynamic buffer sizing**

Replace lines 795-799 (from `char buffer[768];` to `return;`):
```cpp
// Calculate required size first
const size_t requiredSize = json.memoryUsage();
const size_t safetyMargin = 128;
const size_t bufferSize = requiredSize + safetyMargin;

// Verify reasonable limit
if (bufferSize > 1024) {
    debuglnE("JSON payload too large for WebSocket");
    debugE("Required size: ");
    debugE(String(bufferSize));
    debugE(" bytes\n");
    return;
}

// Dynamic buffer with exact needed size
char buffer[bufferSize];
size_t len = serializeJson(json, buffer, sizeof(buffer));

if (len >= sizeof(buffer)) {
    debuglnE("JSON serialization failed - buffer too small");
    return;
}

debugD("WebSocket payload size: ");
debugD(String(len));
debugD(" bytes\n");
```

- [ ] **Step 3: Add stack monitoring to TaskWebSocket()**

Find the `TaskWebSocket()` function (around line 936).

Replace entire function with:
```cpp
void TaskWebSocket(void *pvParameters)
{
    UBaseType_t stackHighWaterMark;
    
    while (true)
    {
        ws.cleanupClients();
        notifyClients();
        
        // Monitor stack every 10 cycles
        static uint8_t cycleCount = 0;
        if (++cycleCount >= 10) {
            stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            if (stackHighWaterMark < 256) {
                debuglnW("WebSocket task stack running low!");
                debugE("Stack free: ");
                debugE(String(stackHighWaterMark));
                debugE(" bytes\n");
            }
            cycleCount = 0;
        }
        
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
```

- [ ] **Step 4: Verify compilation**

Run: `pio run`
Expected: SUCCESS with 0 errors, 0 warnings

- [ ] **Step 5: Upload to ESP32**

Run: `pio run --target upload`
Expected: Firmware uploaded successfully

- [ ] **Step 6: Test WebSocket functionality**

Open web interface and verify controls work
Expected: All buttons and sliders work, WebSocket updates appear

- [ ] **Step 7: Monitor payload size messages**

Check serial monitor for debug messages
Expected: Messages showing "WebSocket payload size: XXX bytes"

- [ ] **Step 8: Test normal operation for 5 minutes**

Let device run and monitor for errors
Expected: No buffer overflow errors, no crashes

- [ ] **Step 9: Commit changes**

```bash
git add src/main.cpp
git commit -m "Fix #2: Prevent WebSocket buffer overflow with dynamic sizing

- Calculate JSON size before buffer allocation
- Add safety margin of 128 bytes
- Reject payloads larger than 1KB
- Add stack monitoring to WebSocket task
- Add debug messages for payload size tracking"
```

---

## Task 4: Bug #4 - Add Mutex Protection for Shared Variables

**Files:**
- Modify: `src/main.cpp` (add mutex declarations after global variables)
- Modify: `src/main.cpp` (add initMutexes function)
- Modify: `src/main.cpp` (setup function)
- Modify: `src/main.cpp` (TaskBatteryMonitor function)
- Modify: `src/main.cpp` (TaskLEDControl function)
- Modify: `src/main.cpp` (notifyClients function)
- Modify: `src/main.cpp` (TaskWiFiMonitor function)

- [ ] **Step 1: Add mutex declarations**

Find the global variables section (around line 120, after `Battery18650Stats` declaration).

Add after line 122:
```cpp
// ----------------------------------------------------------------------------
// RTOS Mutex Protection
// ----------------------------------------------------------------------------
SemaphoreHandle_t dataMutex = NULL;
SemaphoreHandle_t wifiMutex = NULL;

void initMutexes() {
    dataMutex = xSemaphoreCreateMutex();
    wifiMutex = xSemaphoreCreateMutex();
    
    if (dataMutex == NULL || wifiMutex == NULL) {
        debuglnE("Failed to create mutexes!");
        debuglnE("System may experience race conditions");
    } else {
        debuglnD("Mutexes initialized successfully");
    }
}
```

- [ ] **Step 2: Call initMutexes() in setup()**

Find the `setup()` function (around line 1010).
Find `Serial.begin(115200);` (around line 1033).

Add after `Serial.begin(115200);`:
```cpp
initMutexes();
```

- [ ] **Step 3: Protect TaskBatteryMonitor() shared data**

Find `TaskBatteryMonitor()` function (around line 946).

Replace line 951:
```cpp
lvlCharge = batt.battLvl;
```

With:
```cpp
// Protect shared variable access
if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    lvlCharge = batt.battLvl;
    xSemaphoreGive(dataMutex);
} else {
    debuglnW("Failed to acquire data mutex in BatteryMonitor");
}
```

- [ ] **Step 4: Protect TaskLEDControl() shared data**

Find `TaskLEDControl()` function (around line 956).

Replace line 962:
```cpp
brightness = stripLed.brightness;
```

With:
```cpp
// Protect brightness access
if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    brightness = stripLed.brightness;
    xSemaphoreGive(dataMutex);
}
```

- [ ] **Step 5: Protect notifyClients() shared data**

Find `notifyClients()` function (around line 748).

Add at the beginning of the function (after line 748, before `JsonDocument json;`):
```cpp
// Take mutex for reading shared data
if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
```

Add `xSemaphoreGive(dataMutex);` before the final `ws.textAll(buffer, len);` call (around line 803).

The structure should be:
```cpp
void notifyClients()
{
    // Take mutex for reading shared data
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        JsonDocument json;
        // ... all the JSON building code ...
        
        char buffer[bufferSize];
        size_t len = serializeJson(json, buffer, sizeof(buffer));
        
        if (len < sizeof(buffer)) {
            ws.textAll(buffer, len);
        } else {
            debuglnE("WebSocket buffer overflow prevented");
        }
        
        xSemaphoreGive(dataMutex);
    } else {
        debuglnW("Failed to acquire mutex for WebSocket update");
    }
}
```

- [ ] **Step 6: Improve TaskWiFiMonitor() with retry logic**

Find `TaskWiFiMonitor()` function (around line 973).

Replace entire function with:
```cpp
void TaskWiFiMonitor(void *pvParameters)
{
    static int disconnectCount = 0;
    const int MAX_DISCONNECTS = 5;
    
    while (true)
    {
        // Protect WiFi status check
        if (xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (WiFi.status() != WL_CONNECTED) {
                disconnectCount++;
                debugW("WiFi disconnect count: ");
                debugW(String(disconnectCount));
                debugW("\n");
                
                if (disconnectCount >= MAX_DISCONNECTS) {
                    debuglnE("Max WiFi disconnects reached, restarting...");
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    ESP.restart();
                }
            } else {
                if (disconnectCount > 0) {
                    debugD("WiFi reconnected after ");
                    debugD(String(disconnectCount));
                    debugD(" disconnects\n");
                }
                disconnectCount = 0;
            }
            xSemaphoreGive(wifiMutex);
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

- [ ] **Step 7: Verify compilation**

Run: `pio run`
Expected: SUCCESS with 0 errors, 0 warnings

- [ ] **Step 8: Upload to ESP32**

Run: `pio run --target upload`
Expected: Firmware uploaded successfully

- [ ] **Step 9: Test LED control smoothness**

Use web interface to change LED colors and effects
Expected: Smooth transitions, no flicker

- [ ] **Step 10: Test battery monitoring**

Check serial monitor for battery level updates
Expected: Consistent battery readings, no corruption

- [ ] **Step 11: Test WiFi reconnection**

Power cycle your WiFi router
Expected: Device reconnects without restart

- [ ] **Step 12: Monitor for mutex errors**

Check serial monitor for mutex error messages
Expected: No "Failed to acquire mutex" messages

- [ ] **Step 13: Commit changes**

```bash
git add src/main.cpp
git commit -m "Fix #4: Add mutex protection for shared variables

- Add dataMutex and wifiMutex for synchronization
- Protect TaskBatteryMonitor shared variable access
- Protect TaskLEDControl brightness access
- Protect notifyClients() shared data reading
- Improve TaskWiFiMonitor with retry logic (5 disconnects)
- Add mutex initialization in setup()
- Add debug messages for mutex failures"
```

---

## Task 5: Bug #3 - Replace String with Char Arrays to Prevent Fragmentation

**Files:**
- Modify: `src/debug.h`
- Modify: `src/main.cpp` (battery monitoring debug messages)
- Modify: `src/main.cpp` (initWiFi function)
- Modify: `src/main.cpp` (notifyClients function - verify no changes needed)

- [ ] **Step 1: Replace String in debug.h**

Find line 58 in `src/debug.h`:
```cpp
String debugStr = "";
```

Replace with:
```cpp
char debugStr[128];  // Buffer for debug messages
```

- [ ] **Step 2: Update traceStamp macro to use char buffer**

Find the `traceStamp` macro definition (around line 62).

Replace line 74:
```cpp
debugStr = y;
```

With:
```cpp
strncpy(debugStr, y.c_str(), sizeof(debugStr) - 1);
debugStr[sizeof(debugStr) - 1] = '\0';
```

- [ ] **Step 3: Add debug helper macros**

Add at the end of `src/debug.h` (after line 127):
```cpp
// Helper macros for debugging with numbers
#define debugD_NUM(val, format) \
    do { \
        char _buf[32]; \
        snprintf(_buf, sizeof(_buf), format, val); \
        debugD(_buf); \
    } while(0)

#define debuglnD_NUM(val, format) \
    do { \
        char _buf[32]; \
        snprintf(_buf, sizeof(_buf), format, val); \
        debuglnD(_buf); \
    } while(0)
```

- [ ] **Step 4: Replace String in battery monitoring**

Find the battery monitoring debug messages in `src/main.cpp` (around line 190, inside the Battery struct).

Replace lines 189-193:
```cpp
#ifdef BATTERY
        debuglnD(chargeState ? "Cargador conectado" : "Cargador desconectado");
        debuglnD("Estado del pin carga: " + String(fullyCharge));
        debuglnD(!fullBatt && !chargeState ? "Batería usándose" : (fullBatt ? "Batería completamente cargada" : "Batería cargándose"));
        debuglnD("Lectura promedio del pin: " + String(battery.pinRead()) + ", Voltaje: " + String(battVolts) + ", Nivel de carga: " + String(battLvl));
#endif
```

With:
```cpp
#ifdef BATTERY
        debugD(chargeState ? "Cargador conectado" : "Cargador desconectado");
        debugD("\n");
        
        debugD("Estado del pin carga: ");
        debugD(fullyCharge ? "LOW (charging)" : "HIGH (full/not charging)");
        debugD("\n");
        
        if (!fullBatt && !chargeState) {
            debuglnD("Batería usándose");
        } else if (fullBatt) {
            debuglnD("Batería completamente cargada");
        } else {
            debuglnD("Batería cargándose");
        }
        
        char battMsg[128];
        snprintf(battMsg, sizeof(battMsg), 
                 "Lectura promedio: %d, Voltaje: %.3f, Nivel: %d%%",
                 battery.pinRead(), battVolts, battLvl);
        debuglnD(battMsg);
#endif
```

- [ ] **Step 5: Replace String in initWiFi()**

Find `initWiFi()` function in `src/main.cpp` (around line 494).

Replace lines 502-503:
```cpp
String savedSSID = preferences.getString("ssid", "");
String savedPass = preferences.getString("password", "");
```

With:
```cpp
char savedSSID[33] = {0};  // SSID max 32 chars + null
char savedPass[65] = {0};  // Password max 64 chars + null

preferences.getString("ssid", savedSSID, sizeof(savedSSID));
preferences.getString("password", savedPass, sizeof(savedPass));
```

Replace line 513:
```cpp
if (savedSSID.length() > 0 && savedPass.length() > 0) {
```

With:
```cpp
if (strlen(savedSSID) > 0 && strlen(savedPass) > 0) {
```

Replace line 515:
```cpp
Serial.printf("SSID: %s\n", savedSSID.c_str());
WiFi.begin(savedSSID.c_str(), savedPass.c_str());
```

With:
```cpp
debugD("SSID: ");
debugD(savedSSID);
debugD("\n");
WiFi.begin(savedSSID, savedPass);
```

- [ ] **Step 6: Verify notifyClients() already uses char arrays**

Check `notifyClients()` function (around line 753)
Verify: Lines 753-761 already use `snprintf` with char arrays
Expected: No changes needed, already optimized

- [ ] **Step 7: Verify compilation**

Run: `pio run`
Expected: SUCCESS with 0 errors, 0 warnings

- [ ] **Step 8: Upload to ESP32**

Run: `pio run --target upload`
Expected: Firmware uploaded successfully

- [ ] **Step 9: Test debug messages**

Check serial monitor
Expected: All debug messages appear correctly

- [ ] **Step 10: Test WiFi connection**

Verify WiFi connects using saved or default credentials
Expected: WiFi connects successfully

- [ ] **Step 11: Run 30-minute stability test**

Let device run for 30 minutes
Monitor: Heap free memory should remain stable
Expected: No crashes, heap degradation < 10%

- [ ] **Step 12: Check heap fragmentation**

Add temporary monitoring code to see heap stability:
```cpp
// In TaskOnboardLED, add temporarily:
debugD("Free heap: ");
debugD_NUM(ESP.getFreeHeap(), "%u");
debugD(" bytes\n");
```

Expected: Heap free remains stable (variation < 10%)

- [ ] **Step 13: Commit changes**

```bash
git add src/debug.h src/main.cpp
git commit -m "Fix #3: Replace String with char arrays to prevent fragmentation

- Replace String debugStr with char array buffer in debug.h
- Update traceStamp macro to use strncpy instead of String
- Add helper macros debugD_NUM/debuglnD_NUM for numeric debug
- Replace String concatenations in battery monitoring with snprintf
- Replace String objects in initWiFi() with char arrays
- Verify notifyClients() already uses char arrays (no changes)
- Reduces heap allocations, prevents memory fragmentation"
```

---

## Task 6: Integration Testing and Final Verification

**Files:**
- No file modifications
- Testing only

- [ ] **Step 1: Perform 1-hour stress test**

Run device for 1 hour continuous operation
Monitor: Serial output for errors
Expected: No crashes, no resets, no error messages

- [ ] **Step 2: Verify all functionality works**

Test checklist:
- [ ] LED color picker works
- [ ] LED effects work (all 18 effects)
- [ ] Brightness slider works
- [ ] Lamp on/off works
- [ ] Bluetooth controls work (play/pause, volume, skip)
- [ ] Battery monitoring displays correctly
- [ ] Temperature/humidity display correctly
- [ ] WebSocket updates work
- [ ] All web interface controls functional

Expected: All features working correctly

- [ ] **Step 3: Check stack high water marks**

Add temporary monitoring to see stack usage:
```cpp
// Add to each task temporarily:
UBaseType_t stackMin = uxTaskGetStackHighWaterMark(NULL);
Serial.printf("Task %s stack min: %u\n", pcTaskGetTaskName(NULL), stackMin);
```

Expected: All tasks have stack usage < 80%

- [ ] **Step 4: Check heap fragmentation**

Monitor heap over 1 hour:
```cpp
debugD("Free heap: ");
debugD_NUM(ESP.getFreeHeap(), "%u");
debugD(" bytes\n");
```

Expected: Heap fragmentation < 10%, stable free heap

- [ ] **Step 5: Test WiFi reconnection**

Power cycle WiFi router
Verify: Device reconnects automatically
Expected: Reconnects within 30 seconds, continues normal operation

- [ ] **Step 6: Test multiple WebSocket clients**

Connect 2-3 browser windows simultaneously
Expected: All clients receive updates, no performance degradation

- [ ] **Step 7: Verify no memory leaks**

Run device for 1 hour, check heap stability
Expected: Heap free remains stable (no continuous decrease)

- [ ] **Step 8: Remove temporary monitoring code**

Remove any temporary debug/monitoring code added for testing
Recompile and upload final version

- [ ] **Step 9: Final commit with summary**

```bash
git add .
git commit -m "Phase 1 complete: All 5 critical bugs fixed

✅ Bug #5: Infinite loop replaced with timeout
✅ Bug #1: WiFi credentials moved to build-time config
✅ Bug #2: WebSocket buffer overflow prevented
✅ Bug #4: Race conditions fixed with mutexes
✅ Bug #3: Memory fragmentation eliminated

Testing:
- 1+ hour stress test completed
- All functionality verified working
- Stack usage < 80% in all tasks
- Heap fragmentation < 10%
- WiFi reconnection working
- Multiple WebSocket clients supported

System is stable and production-ready."
```

- [ ] **Step 10: Update documentation**

Update `CLAUDE.md` with new build configuration:
```markdown
## Build Configuration

The project uses PlatformIO with environment variables for WiFi credentials.

### Development Setup

1. Copy `.env.example` to `.env`
2. Add your WiFi credentials to `.env`:
   ```
   WIFI_SSID=YourWiFi
   WIFI_PASS=YourPassword
   ```
3. Build: `pio run`
4. Upload: `pio run --target upload`
```

Update `README.md` with build instructions

Expected: Documentation updated and committed

---

## Success Criteria Verification

- [ ] ✅ Compiles with 0 errors, 0 warnings
- [ ] ✅ All existing functionality 100% operational
- [ ] ✅ System runs 1+ hour without crashes
- [ ] ✅ Heap fragmentation < 10%
- [ ] ✅ Stack usage < 80% in all tasks
- [ ] ✅ WiFi reconnection working
- [ ] ✅ No mutex errors or race conditions
- [ ] ✅ WebSocket buffer overflow prevented
- [ ] ✅ LittleFS failure no longer hangs system
- [ ] ✅ No credentials exposed in source code

---

**END OF IMPLEMENTATION PLAN**
