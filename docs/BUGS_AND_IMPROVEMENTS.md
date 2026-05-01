# SML Project - Bug Report and Improvements

**Date:** 2026-04-15  
**Analyzed by:** Claude Code  
**Project:** Smart Music Lamp (SML) v1.0

---

## 🔴 Critical Bugs

### 1. **Hardcoded WiFi Credentials Exposed**
**Location:** `src/data.h:2-3`
```cpp
const char *WIFI_SSID = "MyWiFi";
const char *WIFI_PASS = "password123";
```
**Severity:** CRITICAL - Security Risk  
**Impact:** Default credentials exposed in source code, could be committed to git  
**Fix:**
- Remove default credentials from source code
- Use compile-time environment variables or secure configuration
- Add `data.h` to `.gitignore` if credentials must remain
- Implement WiFi credentials management UI on first boot

---

### 2. **Stack Buffer Overflow Risk in WebSocket**
**Location:** `src/main.cpp:795-799`
```cpp
char buffer[768];
size_t len = serializeJson(json, buffer, sizeof(buffer));
if (len >= sizeof(buffer)) {
    Serial.println("ERROR: JSON buffer overflow!");
    return;
}
```
**Severity:** HIGH - Memory Corruption  
**Impact:** Potential buffer overflow, data truncation, crashes  
**Problems:**
- Fixed size buffer (768 bytes) may be insufficient
- Error logging only detects overflow after it happens
- No dynamic sizing or proper bounds checking  
**Fix:**
```cpp
// Use ArduinoJson's calculateSize() first
const size_t bufferSize = json.memoryUsage() + 128; // Safety margin
if (bufferSize > 1024) { // Set reasonable max
    Serial.println("ERROR: JSON too large");
    return;
}
char buffer[bufferSize];
size_t len = serializeJson(json, buffer, sizeof(buffer));
```

---

### 3. **Memory Fragmentation from String Usage**
**Location:** Multiple files
- `src/debug.h:58` - `String debugStr = "";`
- `src/main.cpp:190-192` - Multiple `String()` concatenations
- `src/main.cpp:502-503` - WiFi SSID/password as String objects

**Severity:** HIGH - Memory Issues  
**Impact:** Heap fragmentation, random crashes, instability  
**Problems:**
- String objects cause heap allocation/fragmentation
- ESP32 has limited heap (especially with WiFi/Bluetooth)
- Strings in debug system called frequently  
**Fix:**
- Use `char[]` buffers or `PSTR()`/`F()` macro for constants
- Pre-allocate buffers and use `snprintf()`
- Replace String with fixed-size char arrays in critical paths

---

### 4. **Race Conditions with Global Variables**
**Location:** Multiple RTOS tasks accessing shared globals
```cpp
// TaskWiFiMonitor (line 977)
if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();  // ⚠️ Unsafe restart
}

// TaskBatteryMonitor (line 951)
lvlCharge = batt.battLvl;  // ⚠️ No mutex

// TaskLEDControl (line 962)
brightness = stripLed.brightness;  // ⚠️ No mutex
```
**Severity:** HIGH - Concurrency Issues  
**Impact:** Data corruption, incorrect behavior, crashes  
**Problems:**
- No mutex protection for shared variables
- Multiple tasks read/write same data
- ESP.restart() called without synchronization  
**Fix:**
```cpp
// Create mutex for shared data
SemaphoreHandle_t dataMutex = xSemaphoreCreateMutex();

// In tasks
if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
    lvlCharge = batt.battLvl;
    xSemaphoreGive(dataMutex);
}
```

---

### 5. **Infinite Loop Blocking in WiFi Init**
**Location:** `src/main.cpp:482-487`
```cpp
while (1) {
    onboard_led.on = millis() % 200 < 50;
    onboard_led.update();
}
```
**Severity:** MEDIUM - Boot Failure  
**Impact:** Device becomes unresponsive if LittleFS fails  
**Problems:**
- No way to recover without power cycle
- No error logging
- No web interface available  
**Fix:**
- Add watchdog timer
- Implement safe mode with default settings
- Add error recovery mechanism

---

## 🟡 Medium Priority Issues

### 6. **Debug System Inefficiency**
**Location:** `src/debug.h:58-79`
**Problems:**
- `String debugStr` allocated even when debug disabled
- Complex string operations in hot path
- `traceStampRequired` checked multiple times  
**Impact:** Performance degradation, memory waste  
**Fix:**
- Use compile-time optimization
- Replace String with char buffers
- Inline critical debug functions

---

### 7. **Blocking delay() Calls**
**Location:** `src/Battery.h:47`
```cpp
delay(WaveDelay);  // ⚠️ Blocks RTOS task
```
**Problems:**
- Blocks entire RTOS task
- Prevents other operations
- Wastes CPU cycles  
**Fix:**
- Replace with `vTaskDelay()`
- Use non-blocking timing with `millis()`

---

### 8. **Missing Input Validation**
**Location:** `src/main.cpp:806-894` (WebSocket handler)
```cpp
const int brightness = json["brightness"].as<int>();  // ⚠️ No range check
stripLed.effectId = effectId;  // ⚠️ No bounds checking
```
**Problems:**
- No validation of brightness range (0-255)
- No bounds checking for effectId (0-18)
- No type checking for JSON fields  
**Impact:** Invalid values cause undefined behavior  
**Fix:**
```cpp
if (json.containsKey("brightness")) {
    int brightness = json["brightness"];
    if (brightness >= 0 && brightness <= 255) {
        stripLed.brightness = brightness;
    }
}
```

---

### 9. **WiFi Auto-Restart Loop**
**Location:** `src/main.cpp:977-979`
```cpp
if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();  // ⚠️ Immediate restart
}
```
**Problems:**
- Too aggressive restart policy
- No retry mechanism
- Can cause boot loops  
**Impact:** Device unusable during temporary WiFi outages  
**Fix:**
```cpp
static int disconnectCount = 0;
if (WiFi.status() != WL_CONNECTED) {
    disconnectCount++;
    if (disconnectCount > 5) {
        ESP.restart();
    }
} else {
    disconnectCount = 0;
}
```

---

### 10. **Memory Leaks in JSON Documents**
**Location:** Multiple locations using `JsonDocument`
**Problems:**
- `JsonDocument` on stack can be large
- Multiple simultaneous JSON operations
- No memory pool management  
**Impact:** Stack overflow, heap exhaustion  
**Fix:**
- Use static JsonDocument where possible
- Reuse JSON documents instead of creating new ones
- Monitor stack usage with `uxTaskGetStackHighWaterMark()`

---

## 🟢 Low Priority Improvements

### 11. **Code Duplication in Effect Switch**
**Location:** `src/main.cpp:346-408`
**Problems:**
- 18 case statements doing similar operations
- Each effect creates new object instance  
**Impact:** Code bloat, maintenance difficulty  
**Fix:**
- Use function pointer table
- Implement effect factory pattern
- Reduce to single switch statement

---

### 12. **Inefficient Serial Output**
**Location:** `src/main.cpp:514-580`
**Problems:**
- Unconditional Serial.print in WiFi init
- No debug level checks  
**Impact:** Performance degradation, wasted flash  
**Fix:**
- Use debug macros consistently
- Add compile-time option to disable Serial

---

### 13. **Missing Error Handling**
**Locations:**
- LittleFS operations (no error checks)
- DHT sensor reads (retry exists but no fallback)
- JSON deserialization (handled but limited)  
**Impact:** Silent failures, undefined behavior  
**Fix:**
- Add error return values
- Implement default fallbacks
- Log errors appropriately

---

### 14. **Hardcoded Magic Numbers**
**Location:** Throughout codebase
```cpp
vTaskDelay(pdMS_TO_TICKS(3000));  // ⚠️ What is 3000?
```
**Problems:**
- No named constants
- Difficult to maintain  
**Fix:**
```cpp
#define WEBSOCKET_UPDATE_INTERVAL_MS 3000
vTaskDelay(pdMS_TO_TICKS(WEBSOCKET_UPDATE_INTERVAL_MS));
```

---

### 15. **Unused Variables and Dead Code**
**Location:** Multiple files
**Problems:**
- Commented-out code blocks
- Unused includes
- Variables set but never read  
**Impact:** Code bloat, confusion  
**Fix:**
- Remove unused code
- Clean up includes
- Use compiler warnings to identify

---

## 📊 Summary Statistics

| Category | Count |
|----------|-------|
| Critical Bugs | 5 |
| Medium Priority | 5 |
| Low Priority | 5 |
| **Total Issues** | **15** |

---

## 🎯 Recommended Action Plan

### Phase 1: Critical Fixes (Week 1)
1. Remove hardcoded WiFi credentials
2. Fix buffer overflow in WebSocket
3. Replace String with char buffers
4. Add mutex for shared variables
5. Implement WiFi error recovery

### Phase 2: Stability Improvements (Week 2)
6. Optimize debug system
7. Replace delay() with vTaskDelay()
8. Add input validation
9. Implement WiFi retry logic
10. Fix JSON memory management

### Phase 3: Code Quality (Week 3)
11. Refactor effect system
12. Optimize Serial output
13. Add comprehensive error handling
14. Define named constants
15. Clean up dead code

---

## 🛠️ Development Recommendations

### Build Configuration
- Enable compiler warnings: `-Wall -Wextra`
- Use stack overflow protection
- Enable asserts in debug builds
- Monitor stack watermarks

### Testing Strategy
- Unit tests for JSON parsing
- Integration tests for WiFi
- Stress tests for WebSocket
- Memory leak detection
- Long-running stability tests

### Code Review Checklist
- [ ] No hardcoded credentials
- [ ] All shared variables protected by mutex
- [ ] No String allocations in critical paths
- [ ] Input validation on all external inputs
- [ ] Proper error handling
- [ ] No blocking delays in RTOS tasks
- [ ] Stack size adequate for all tasks

---

## 📝 Notes

- Many issues stem from mixing Arduino-style code with RTOS tasks
- Consider migrating to pure ESP-IDF for better control
- Memory monitoring shows ~40KB heap free with WiFi+BT active
- Current stack sizes appear adequate but should be monitored
- Code quality is generally good but needs hardening for production

---

**END OF REPORT**
