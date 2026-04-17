# Critical Bugs Fix - Design Document

**Project:** Smart Music Lamp (SML)  
**Date:** 2026-04-16  
**Status:** Approved  
**Author:** Claude Code  
**Approach:** Minimally Invasive Patches (Enfoque B)

---

## Executive Summary

This document describes the design for fixing 5 critical bugs in the SML project using a minimally invasive approach. Each bug will be fixed individually with separate commits, maintaining all existing functionality and web interface design.

**Order of Implementation (by complexity):**
1. Bug #5: Infinite Loop in WiFi Init (15 min)
2. Bug #1: Hardcoded WiFi Credentials (30 min)
3. Bug #2: WebSocket Buffer Overflow (45 min)
4. Bug #4: Race Conditions in Global Variables (60 min)
5. Bug #3: Memory Fragmentation from String Usage (90 min)

**Total Estimated Time:** 4 hours

---

## Architecture Overview

### Design Philosophy
- **Minimally invasive patches:** Fix bugs with the smallest possible changes
- **No structural changes:** Maintain current file structure and API
- **Preserve functionality:** All existing features must work identically
- **Web interface unchanged:** No visual changes without approval
- **Easily reversible:** Each fix is independent and can be rolled back

### File Structure (No Changes)
- `src/main.cpp` - Main application file (1,084 lines)
- `src/data.h` - WiFi configuration
- `src/debug.h` - Debug logging system
- `platformio.ini` - Build configuration
- All other files remain unchanged

### Testing Strategy
- Manual testing on ESP32 hardware after each fix
- Individual commits for each bug
- Verify 0 compiler errors/warnings
- Integration testing after all fixes complete

---

## Bug #5: Infinite Loop in WiFi Init

### Problem
**Location:** `src/main.cpp:482-487`

Current code blocks indefinitely if LittleFS fails to mount:
```cpp
while (1) {
    onboard_led.on = millis() % 200 < 50;
    onboard_led.update();
}
```

### Solution
Replace infinite loop with timeout and fallback to default WiFi credentials:

**Changes in `main.cpp`:**
```cpp
// Replace infinite loop with:
unsigned long errorStartTime = millis();
const unsigned long MAX_ERROR_TIME = 30000; // 30 seconds

while (millis() - errorStartTime < MAX_ERROR_TIME) {
    onboard_led.on = millis() % 200 < 50;
    onboard_led.update();
    vTaskDelay(pdMS_TO_TICKS(100));
}

// Continue with setup using default WiFi credentials
debuglnE("LittleFS mount failed, using default WiFi credentials");
debugW("Device will continue with limited functionality");
```

### Testing Criteria
- [ ] LittleFS mounts correctly in normal operation
- [ ] If LittleFS fails, system continues after 30s timeout
- [ ] LED flashes differently during error (50ms on, 150ms off)
- [ ] Debug messages appear at correct levels
- [ ] WiFi connects with default credentials

---

## Bug #1: Hardcoded WiFi Credentials

### Problem
**Location:** `src/data.h:2-3`

Default WiFi credentials exposed in source code:
```cpp
const char *WIFI_SSID = "MyWiFi";
const char *WIFI_PASS = "password123";
```

Current system uses Preferences (NV memory) first, then falls back to these defaults. The issue is that default credentials are visible in source code.

### Solution
Move default credentials to build-time configuration using PlatformIO build flags.

**Changes in `platformio.ini`:**
```ini
[env:esp32doit-devkit-v1]
; ... existing config ...
build_flags = 
    -DELEGENTOTA_USE_ASYNC_WEBSERVER=1
    -DDEFAULT_WIFI_SSID=\""$(WIFI_SSID)"\" 
    -DDEFAULT_WIFI_PASS=\""$(WIFI_PASS)"\"
```

**Changes in `src/data.h`:**
```cpp
// WiFi credentials - set via build flags
// For development: create .env file with WIFI_SSID and WIFI_PASS
#ifndef DEFAULT_WIFI_SSID
#define DEFAULT_WIFI_SSID ""  
#endif
#ifndef DEFAULT_WIFI_PASS
#define DEFAULT_WIFI_PASS ""  
#endif

const char *WIFI_SSID = DEFAULT_WIFI_SSID;
const char *WIFI_PASS = DEFAULT_WIFI_PASS;
```

**New file `.env` (add to .gitignore):**
```bash
WIFI_SSID=YourRealWiFi
WIFI_PASS=YourRealPassword
```

**Changes in `.gitignore`:**
```
.env
```

**Debug messages in `initWiFi()`:**
```cpp
debuglnD("Attempting WiFi connection...");
if (savedSSID.length() > 0) {
    debuglnD("Using saved credentials from Preferences");
} else {
    debuglnD("Using default credentials from build configuration");
}
```

### Testing Criteria
- [ ] Project compiles with no errors
- [ ] Credentials from .env are used at compile time
- [ ] Preferences system still works (reads saved credentials first)
- [ ] Web interface `/save-wifi` still works to save new credentials
- [ ] No credentials visible in source code
- [ ] Build fails gracefully if .env is missing (uses empty strings)

---

## Bug #2: WebSocket Buffer Overflow

### Problem
**Location:** `src/main.cpp:795-799`

Fixed-size buffer (768 bytes) can overflow:
```cpp
char buffer[768];
size_t len = serializeJson(json, buffer, sizeof(buffer));
if (len >= sizeof(buffer)) {
    Serial.println("ERROR: JSON buffer overflow!");
    return;
}
```

### Solution
Calculate required size first, use dynamic buffer with safety limits.

**Changes in `notifyClients()` function:**
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

**Changes in `TaskWebSocket()`:**
```cpp
void TaskWebSocket(void *pvParameters) {
    UBaseType_t stackHighWaterMark;
    
    while (true) {
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

### Testing Criteria
- [ ] WebSocket connects without errors
- [ ] JSON messages sent correctly
- [ ] Debug messages show payload size
- [ ] Oversized JSON rejected gracefully
- [ ] No crashes or resets
- [ ] Stack monitoring works

---

## Bug #4: Race Conditions in Global Variables

### Problem
**Locations:** Multiple RTOS tasks accessing shared globals without synchronization

```cpp
// TaskBatteryMonitor (line 951)
lvlCharge = batt.battLvl;  // ⚠️ No mutex

// TaskLEDControl (line 962)  
brightness = stripLed.brightness;  // ⚠️ No mutex

// TaskWiFiMonitor (line 977)
if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();  // ⚠️ Unsafe restart
}

// notifyClients() accesses: stripLed, batt, lampState, bt_powerState
```

### Solution
Add mutex protection for shared data and improve WiFi restart logic.

**Changes at beginning of `main.cpp`:**
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

**Changes in `TaskBatteryMonitor()`:**
```cpp
void TaskBatteryMonitor(void *pvParameters) {
    while (true) {
        batt.battMonitor();
        
        // Protect shared variable access
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            lvlCharge = batt.battLvl;
            xSemaphoreGive(dataMutex);
        } else {
            debuglnW("Failed to acquire data mutex in BatteryMonitor");
        }
        
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
```

**Changes in `TaskLEDControl()`:**
```cpp
void TaskLEDControl(void *pvParameters) {
    while (true) {
        if (stripLed.powerState) {
            // Protect brightness access
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                brightness = stripLed.brightness;
                xSemaphoreGive(dataMutex);
            }
            
            stripLed.update();
        } else {
            stripLed.clear();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
```

**Changes in `notifyClients()`:**
```cpp
void notifyClients() {
    // Take mutex for reading shared data
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        JsonDocument json;
        // ... existing JSON code ...
        
        char buffer[calculateBufferSize(json)];
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

**Changes in `TaskWiFiMonitor()`:**
```cpp
void TaskWiFiMonitor(void *pvParameters) {
    static int disconnectCount = 0;
    const int MAX_DISCONNECTS = 5;
    
    while (true) {
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

**Changes in `setup()`:**
```cpp
void setup() {
    // ... existing code ...
    
    Serial.begin(115200);
    initMutexes();  // ← Add after Serial.begin()
    
    // ... rest of setup ...
}
```

### Testing Criteria
- [ ] LED control works smoothly (no flicker)
- [ ] Battery level updates correctly
- [ ] WebSocket updates show consistent data
- [ ] WiFi reconnect works (router power cycle test)
- [ ] No mutex error messages in serial
- [ ] No data corruption or crashes

---

## Bug #3: Memory Fragmentation from String Usage

### Problem
**Locations:** Multiple files using String class causing heap fragmentation

**1. `debug.h:58` - String in hot path:**
```cpp
String debugStr = "";  // ⚠️ Heap allocation on every debug
```

**2. `main.cpp:190-192` - String concatenations:**
```cpp
debuglnD("Estado del pin carga: " + String(fullyCharge));  // ⚠️ Heap
debuglnD("Lectura promedio: " + String(battery.pinRead()) + ", ...");  // ⚠️ Heap
```

**3. `main.cpp:502-503` - WiFi credentials as String:**
```cpp
String savedSSID = preferences.getString("ssid", "");  // ⚠️ Heap
String savedPass = preferences.getString("password", "");  // ⚠️ Heap
```

### Solution
Replace String with char arrays and stack-based buffers.

**Changes in `debug.h`:**
```cpp
// Replace String with static buffer
char debugStr[128];  // Sufficient buffer for messages
bool foundNL = false;

// In traceStamp macro, replace:
// debugStr = y;  ← BEFORE
strncpy(debugStr, y.c_str(), sizeof(debugStr) - 1);  // ← AFTER
debugStr[sizeof(debugStr) - 1] = '\0';  // Null termination
```

**Changes in `main.cpp` (battery monitoring):**
```cpp
#ifdef BATTERY
    // Replace String concatenations
    debugD("Estado del pin carga: ");
    debugD(fullyCharge ? "LOW" : "HIGH");
    debugD("\n");
    
    // Conditional messages without String
    if (!fullBatt && !chargeState) {
        debuglnD("Batería usándose");
    } else if (fullBatt) {
        debuglnD("Batería completamente cargada");
    } else {
        debuglnD("Batería cargándose");
    }
    
    // Numeric values with snprintf
    char battMsg[128];
    snprintf(battMsg, sizeof(battMsg), 
             "Lectura promedio: %d, Voltaje: %.3f, Nivel: %d%%",
             battery.pinRead(), battVolts, battLvl);
    debuglnD(battMsg);
#endif
```

**Changes in `initWiFi()`:**
```cpp
// Replace String with char arrays
Preferences preferences;
preferences.begin("wifi", false);

char savedSSID[33] = {0};  // SSID max 32 chars + null
char savedPass[65] = {0};  // Password max 64 chars + null

preferences.getString("ssid", savedSSID, sizeof(savedSSID));
preferences.getString("password", savedPass, sizeof(savedPass));
preferences.end();

// Use directly:
if (strlen(savedSSID) > 0 && strlen(savedPass) > 0) {
    debugD("Trying SAVED credentials...\n");
    debugD("SSID: ");
    debugD(savedSSID);
    debugD("\n");
    WiFi.begin(savedSSID, savedPass);
    // ... rest of code unchanged
}
```

**Add helper macros at end of `debug.h`:**
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

// Usage:
// debugD_NUM(battVolts, "%.3f");  // Debug float
// debugD_NUM(battLvl, "%d");      // Debug int
```

**Note:** `notifyClients()` already uses char arrays correctly (lines 753-761), no changes needed.

### Testing Criteria
- [ ] System runs 30+ minutes without crashes
- [ ] Heap free remains stable (monitor with ESP.getFreeHeap())
- [ ] Debug messages work correctly
- [ ] WiFi connection/reconnection works
- [ ] No performance degradation
- [ ] Memory fragmentation < 10%

---

## Testing Strategy

### Individual Bug Testing Process
For each bug (in order 5→1→2→4→3):
1. Make code changes
2. Compile: `pio run`
3. Verify: 0 errors, 0 warnings
4. Upload: `pio run --target upload`
5. Monitor: `pio device monitor`
6. Test affected functionality
7. If OK → individual commit

### Integration Testing (After All Bugs)
- Stress test: 1 hour continuous operation
- Verify stack high water marks
- Verify heap fragmentation
- WiFi reconnection test (power cycle router)
- Multiple WebSocket clients test

### Success Criteria
- ✅ Compiles with 0 errors, 0 warnings
- ✅ All existing functionality 100% operational
- ✅ System runs 1+ hour without crashes
- ✅ Heap fragmentation < 10%
- ✅ Stack usage < 80% in all tasks

---

## Implementation Plan

### Phase 1: Bug #5 - Infinite Loop (15 min)
- [ ] Modify `initLittleFS()` error handling
- [ ] Add debug messages
- [ ] Test LittleFS failure scenario
- [ ] Commit: "Fix #5: Replace infinite loop in WiFi init with timeout"

### Phase 2: Bug #1 - WiFi Credentials (30 min)
- [ ] Create `.env` file
- [ ] Modify `platformio.ini` build flags
- [ ] Update `data.h`
- [ ] Update `.gitignore`
- [ ] Add debug messages
- [ ] Test build with .env
- [ ] Test Preferences system still works
- [ ] Commit: "Fix #1: Move WiFi credentials to build-time configuration"

### Phase 3: Bug #2 - Buffer Overflow (45 min)
- [ ] Modify `notifyClients()` buffer logic
- [ ] Add size calculation before allocation
- [ ] Add stack monitoring to `TaskWebSocket()`
- [ ] Test with normal JSON payloads
- [ ] Test with oversized JSON
- [ ] Commit: "Fix #2: Prevent WebSocket buffer overflow with dynamic sizing"

### Phase 4: Bug #4 - Race Conditions (60 min)
- [ ] Add mutex initialization
- [ ] Protect `TaskBatteryMonitor()` shared data
- [ ] Protect `TaskLEDControl()` shared data
- [ ] Protect `notifyClients()` shared data
- [ ] Improve `TaskWiFiMonitor()` with retry logic
- [ ] Test LED smoothness
- [ ] Test WiFi reconnection
- [ ] Commit: "Fix #4: Add mutex protection for shared variables"

### Phase 5: Bug #3 - Memory Fragmentation (90 min)
- [ ] Optimize `debug.h` String usage
- [ ] Replace String concatenations in battery monitoring
- [ ] Replace String in WiFi credentials
- [ ] Add debug helper macros
- [ ] Test 30+ minute stability
- [ ] Monitor heap fragmentation
- [ ] Commit: "Fix #3: Replace String with char arrays to prevent fragmentation"

### Phase 6: Integration Testing (30 min)
- [ ] Stress test 1 hour
- [ ] Verify all functionality
- [ ] Check stack usage
- [ ] Check heap fragmentation
- [ ] Final commit: "Phase 1 complete: All critical bugs fixed"

---

## Risk Assessment

### Low Risk
- Bug #5: Simple timeout addition, isolated change
- Bug #1: Build configuration change, logic unchanged
- Bug #2: Buffer sizing improvement, existing logic preserved

### Medium Risk
- Bug #4: Introduces mutex synchronization, requires testing
- Bug #3: String replacements across multiple files

### Mitigation Strategies
- Individual commits allow easy rollback
- Manual testing on hardware after each fix
- Debug messages help identify issues
- Stack/heap monitoring catches problems early

---

## Post-Implementation

### Monitoring
- Add temporary heap/stack monitoring to `TaskOnboardLED()`
- Remove monitoring after 1 week of stable operation

### Documentation
- Update CLAUDE.md with new build configuration
- Update README.md with .env file instructions
- Consider adding troubleshooting guide

### Future Improvements (Out of Scope)
- Consider migrating to ESP-IDF for better RTOS control
- Implement proper error recovery system
- Add comprehensive unit tests
- Consider implementing OTA updates for bug fixes

---

**END OF DESIGN DOCUMENT**
