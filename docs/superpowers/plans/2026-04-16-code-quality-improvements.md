# Code Quality Improvements and Bug Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix 8 code quality issues and bugs found in comprehensive code review, improving memory safety, thread synchronization, and error handling.

**Architecture:** Minimally invasive patches following existing code patterns, individual commits for each fix, maintaining all existing functionality.

**Tech Stack:** ESP32, PlatformIO, ArduinoJson, FreeRTOS (mutexes), ESPAsyncWebServer, FastLED, DHT sensor library

---

## File Structure

**Files to be modified:**
- `src/debug.h` - Fix buffer overflow risk
- `src/main.cpp` - Memory leaks, race conditions, error handling improvements
- `src/data.h` - Review (may need changes)
- `src/Balls.h` - Array bounds checking
- PlatformIO configuration (if needed)

**No new files to create**

---

## Task 1: Fix Debug Buffer Overflow Risk (CRITICAL)

**Files:**
- Modify: `src/debug.h:58-70` (copyToDebugStr functions)

**IMPORTANT:** Do NOT use `snprintf()` with format specifiers as this causes ESP32 crash at startup (learned from DHT sensor crash). Use simple character copy with length limit instead.

- [ ] **Step 1: Read current debug buffer implementation**

Read `src/debug.h` lines 58-70 to understand current buffer implementation using strncpy with 128-byte buffer.

- [ ] **Step 2: Replace strncpy with safer character copy**

Replace unsafe `strncpy()` with manual character copy and explicit length limit to prevent buffer overflow while avoiding snprintf crash:

```cpp
// Helper function to copy string to debug buffer (SAFE version - no snprintf)
inline void copyToDebugStr(const char* src) {
    if (src == nullptr) {
        debugStr[0] = '\0';
        foundNL = false;
        return;
    }
    
    // Safe copy with explicit length limit
    size_t i = 0;
    while (i < sizeof(debugStr) - 1 && src[i] != '\0') {
        debugStr[i] = src[i];
        i++;
    }
    debugStr[i] = '\0';  // Always null terminate
    foundNL = (strstr(debugStr, "\n") != NULL);
}

// Helper function for String objects
inline void copyToDebugStr(const String& src) {
    // Safe copy with explicit length limit
    size_t i = 0;
    const char* cstr = src.c_str();
    
    while (i < sizeof(debugStr) - 1 && cstr[i] != '\0') {
        debugStr[i] = cstr[i];
        i++;
    }
    debugStr[i] = '\0';  // Always null terminate
    foundNL = (strstr(debugStr, "\n") != NULL);
}
```

- [ ] **Step 3: Verify compilation**

Run: `pio run`
Expected: SUCCESS with 0 errors, 0 warnings

- [ ] **Step 4: Test debug messages still work**

Upload to ESP32 and verify debug messages appear correctly in serial monitor.
Expected: All debug messages display without truncation, no crash at startup

- [ ] **Step 5: Commit**

```bash
git add src/debug.h
git commit -m "Fix: Debug buffer overflow risk - safe character copy without snprintf

Replace unsafe strncpy() with explicit character copy and length limit.
AVOID snprintf() which causes ESP32 crash at startup.
Prevents buffer overflow with long debug messages.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Task 2: Fix Memory Leak in WiFi Credentials Handler (HIGH)

**Files:**
- Modify: `src/main.cpp:798-820` (saveWifiHandler)

- [ ] **Step 1: Read current WiFi credential saving code**

Read `src/main.cpp` lines 798-820 to understand String object usage in saveWifiHandler.

- [ ] **Step 2: Replace String objects with char arrays**

Find these lines:
```cpp
String newSSID = request->getParam("ssid", true)->value();
String newPassword = request->getParam("password", true)->value();

if (newSSID.length() == 0) {
    newSSID = WiFi.SSID();
}
```

Replace with:
```cpp
char newSSID[33] = {0};  // SSID max 32 chars + null
char newPassword[65] = {0};  // Password max 64 chars + null

String tempSSID = request->getParam("ssid", true)->value();
String tempPass = request->getParam("password", true)->value();

// Copy String to char array safely
if (tempSSID.length() > 0) {
    strncpy(newSSID, tempSSID.c_str(), sizeof(newSSID) - 1);
}
if (tempPass.length() > 0) {
    strncpy(newPassword, tempPass.c_str(), sizeof(newPassword) - 1);
}

// If SSID is empty, use current SSID
if (strlen(newSSID) == 0) {
    strncpy(newSSID, WiFi.SSID().c_str(), sizeof(newSSID) - 1);
}
```

- [ ] **Step 3: Verify compilation**

Run: `pio run`
Expected: SUCCESS with 0 errors, 0 warnings

- [ ] **Step 4: Test WiFi credential saving**

Upload to ESP32, open web interface, save new WiFi credentials.
Expected: Credentials saved successfully, no memory fragmentation

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "Fix: Memory leak in WiFi credential handler - replace String with char arrays

Prevent memory fragmentation from String objects in saveWifiHandler.
Use fixed-size char arrays with proper bounds checking.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Task 3: Add Mutex Protection for WiFi Credentials (HIGH)

**Files:**
- Modify: `src/main.cpp:798-846` (saveWifiHandler entire function)

- [ ] **Step 1: Review current WiFi credential saving without mutex**

Read the saveWifiHandler function to understand it accesses Preferences without mutex protection.

- [ ] **Step 2: Add mutex protection around Preferences operations**

Wrap the Preferences operations in saveWifiHandler with mutex protection:

```cpp
#ifdef DEBUG_WIFI
      debuglnD("Saving WiFi credentials...");
#endif

      // Protect WiFi credential operations with mutex
      if (xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
          // Save to Preferences
          Preferences preferences;
          preferences.begin("wifi", false);
          preferences.putString("ssid", newSSID);
          preferences.putString("password", newPassword);
          preferences.end();

#ifdef DEBUG_WIFI
          debuglnD("Saved. Restarting.");
#endif
          xSemaphoreGive(wifiMutex);

          // Send quick response
          request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Saved\"}");

          // Restart immediately
          ESP.restart();
      } else {
#ifdef DEBUG_WIFI
          debuglnE("Failed to acquire WiFi mutex for saving credentials");
#endif
          request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Mutex busy\"}");
      }
```

- [ ] **Step 3: Verify compilation**

Run: `pio run`
Expected: SUCCESS with 0 errors, 0 warnings

- [ ] **Step 4: Test concurrent WiFi credential access**

Upload to ESP32, try saving WiFi credentials while WiFi is connecting/reconnecting.
Expected: No corruption, mutex prevents conflicts

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "Fix: Add mutex protection for WiFi credential saving

Prevent race conditions when saving WiFi credentials while WiFi
is active. Use wifiMutex to protect Preferences operations.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Task 4: Improve LittleFS Error Recovery (HIGH)

**Files:**
- Modify: `src/main.cpp:541-564` (initLittleFS function)

- [ ] **Step 1: Review current LittleFS error handling**

Read initLittleFS function to understand current timeout mechanism.

- [ ] **Step 2: Add fallback mechanism when LittleFS fails**

Add better error recovery and user notification:

```cpp
void initLittleFS()
{
    if (!LittleFS.begin())
    {
#ifdef DEBUG_WEB
        debuglnD("Cannot mount LittleFS volume...");
#endif

        // Timeout instead of infinite loop
        unsigned long errorStartTime = millis();
        const unsigned long MAX_ERROR_TIME = 30000; // 30 seconds

        // Overflow-safe elapsed time calculation
        unsigned long elapsed;
        do
        {
            elapsed = millis() - errorStartTime;
            onboard_led.on = millis() % 200 < 50; // LED flashes, lighting for 50 ms and turning off for 150 ms in a 200 ms cycle. Indicates error when mounting volume
            onboard_led.update();
            vTaskDelay(pdMS_TO_TICKS(100));
        } while (elapsed < MAX_ERROR_TIME);

        // Continue with setup - system will work with reduced functionality
#ifdef DEBUG_SYSTEM
        debuglnE("LittleFS mount failed - web interface unavailable");
        debuglnW("Device will continue with limited functionality:");
        debuglnW("- Web interface will not be available");
        debuglnW("- Default WiFi credentials will be used");
        debuglnW("- OTA updates will not work");
#endif
    }
#ifdef DEBUG_WEB
    else {
        debuglnD("LittleFS mounted successfully");
    }
#endif
}
```

- [ ] **Step 3: Verify compilation**

Run: `pio run`
Expected: SUCCESS with 0 errors, 0 warnings

- [ ] **Step 4: Test LittleFS failure recovery**

Upload to ESP32 with corrupted LittleFS (disconnect power during write).
Expected: System continues with limited functionality, clear debug messages

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "Fix: Improve LittleFS error recovery with detailed fallback

Add better error messages and fallback behavior when LittleFS fails.
System continues with reduced functionality instead of hanging.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Task 5: Define Constants for Magic Numbers (MEDIUM)

**Files:**
- Modify: `src/main.cpp` (add constants section)
- Modify: `src/main.cpp:1003, 1080, 1112` (replace magic numbers)

- [ ] **Step 1: Add constants section at top of file**

Add after line 57 (after debug category definitions):

```cpp
// ----------------------------------------------------------------------------
// Constants (Magic Numbers)
// ----------------------------------------------------------------------------
// Stack monitoring thresholds
const uint32_t STACK_WARNING_THRESHOLD = 256;      // Stack low warning threshold (bytes)
const uint32_t STACK_CRITICAL_THRESHOLD = 128;     // Stack critical threshold (bytes)

// WiFi connection
const int WIFI_MAX_ATTEMPTS = 40;                 // Max WiFi connection attempts
const unsigned long WIFI_RETRY_DELAY = 500;       // WiFi connection retry delay (ms)

// Timing
const unsigned long LED_ERROR_FLASH_CYCLE = 200;  // LED error flash cycle (ms)
const unsigned long LED_ERROR_FLASH_ON = 50;       // LED error flash on time (ms)

// Battery monitoring
const unsigned long BATTERY_CHECK_INTERVAL = 3000; // Battery check interval (ms)
const unsigned long SENSOR_CHECK_INTERVAL = 5000;  // Sensor check interval (ms)
const int BATTERY_MAX_READS = 10;                  // Max battery reads when threshold reached
const int BATTERY_FULL_READS = 10;                // Max battery reads when full

// LittleFS timeout
const unsigned long LITTLEFS_TIMEOUT = 30000;     // LittleFS error timeout (ms)

// WebSocket
const unsigned long WEBSOCKET_UPDATE_INTERVAL = 3000;  // WebSocket update interval (ms)
const uint8_t WEBSOCKET_STACK_CHECK_CYCLES = 10;       // Check stack every N cycles
```

- [ ] **Step 2: Replace magic number in TaskWebSocket (line 1003)**

Find:
```cpp
if (stackHighWaterMark < 256) {
```

Replace with:
```cpp
if (stackHighWaterMark < STACK_WARNING_THRESHOLD) {
```

- [ ] **Step 3: Replace magic number in TaskBatteryMonitor**

Find all instances of `vTaskDelay(pdMS_TO_TICKS(3000))` in battery monitoring and replace with `vTaskDelay(pdMS_TO_TICKS(BATTERY_CHECK_INTERVAL))`.

- [ ] **Step 4: Verify compilation**

Run: `pio run`
Expected: SUCCESS with 0 errors, 0 warnings

- [ ] **Step 5: Test functionality**

Upload to ESP32 and verify all timing still works correctly.
Expected: All functionality identical, code more maintainable

- [ ] **Step 6: Commit**

```bash
git add src/main.cpp
git commit -m "Refactor: Define constants for magic numbers

Replace hardcoded values with named constants for better
maintainability and code clarity.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Task 6: Fix Integer Overflow Risk in LED Flash Timing (MEDIUM)

**Files:**
- Modify: `src/main.cpp:572` (onboard LED timing in initLittleFS)

- [ ] **Step 1: Review LED timing calculation**

Find line with `millis() % 200 < 50` calculation.

- [ ] **Step 2: Add overflow-safe timing calculation**

Replace with overflow-safe version:

```cpp
// Overflow-safe LED flash timing
uint32_t flashCycle = millis() / LED_ERROR_FLASH_CYCLE;
uint32_t flashOnTime = millis() % LED_ERROR_FLASH_CYCLE;
onboard_led.on = flashOnTime < LED_ERROR_FLASH_ON;
onboard_led.update();
```

- [ ] **Step 3: Verify compilation**

Run: `pio run`
Expected: SUCCESS with 0 errors, 0 warnings

- [ ] **Step 4: Test LED error flash**

Upload to ESP32, force LittleFS failure.
Expected: LED flashes correctly without timing issues after ~49 days

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "Fix: Integer overflow risk in LED error flash timing

Use overflow-safe arithmetic for LED timing to prevent issues
after millis() overflow (~49 days).

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Task 7: Ensure Proper Resource Cleanup (MEDIUM)

**Files:**
- Review: All Preferences usage in `src/main.cpp`

- [ ] **Step 1: Search for all Preferences.begin() calls**

Use grep: `grep -n "preferences.begin" src/main.cpp`

Expected: Find all instances where Preferences is used

- [ ] **Step 2: Verify each Preferences has matching end()**

For each Preferences.begin(), verify there's a corresponding preferences.end() call.

- [ ] **Step 3: Add RAII-style wrapper if needed**

If any Preferences operations don't have proper cleanup, add error handling with guaranteed cleanup:

```cpp
Preferences preferences;
preferences.begin("wifi", false);
// ... operations ...
preferences.end();
```

- [ ] **Step 4: Verify compilation**

Run: `pio run`
Expected: SUCCESS with 0 errors, 0 warnings

- [ ] **Step 5: Monitor for resource leaks**

Run ESP32 for 30 minutes and monitor heap stability.
Expected: No resource accumulation, heap remains stable

- [ ] **Step 6: Commit (if changes needed)**

If cleanup was missing:
```bash
git add src/main.cpp
git commit -m "Fix: Ensure proper Preferences resource cleanup

Verify all Preferences operations have matching end() calls
to prevent resource leaks.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Task 8: Review and Add Bounds Checking for Balls Effect (MEDIUM)

**Files:**
- Review: `src/Balls.h`
- Modify: If bounds checking is needed

- [ ] **Step 1: Read Balls.h array definitions**

Read `src/Balls.h` to understand array access patterns.

- [ ] **Step 2: Check for bounds violations**

Look for array access like `h[NUM_BALLS]`, `vImpact[NUM_BALLS]`, etc.

- [ ] **Step 3: Add bounds checking if needed**

If arrays are accessed with loop indices, verify loop limits match array sizes.

- [ ] **Step 4: Verify compilation**

Run: `pio run`
Expected: SUCCESS with 0 errors, 0 warnings

- [ ] **Step 5: Test Balls effect**

Upload to ESP32, select Balls effect.
Expected: No array corruption, no crashes

- [ ] **Step 6: Commit (if changes needed)**

Only commit if actual bounds violations were found and fixed.

---

## Success Criteria Verification

After completing all tasks:

- [ ] ✅ Compiles with 0 errors, 0 warnings
- [ ] ✅ All existing functionality 100% operational
- [ ] ✅ No buffer overflow risks in debug system
- [ ] ✅ No memory leaks from String objects
- [ ] ✅ No race conditions in credential handling
- [ ] ✅ Better error recovery for filesystem failures
- [ ] ✅ Code more maintainable with named constants
- [ ] ✅ No integer overflow risks
- [ ] ✅ All resources properly cleaned up

---

## Testing Strategy

1. **Unit Testing:** Each fix tested individually
2. **Integration Testing:** Test WiFi + WebSocket + Battery simultaneously
3. **Stress Testing:** Run for 1+ hour to verify no memory leaks
4. **Edge Case Testing:** Test filesystem failure, WiFi failure scenarios

---

## Risk Assessment

**Low Risk:**
- Constants definition (Task 5) - Refactoring only
- LED timing fix (Task 6) - Well-contained change

**Medium Risk:**
- Resource cleanup (Task 7) - Requires monitoring
- Bounds checking (Task 8) - Depends on findings

**High Risk:**
- Debug buffer fix (Task 1) - Core system change
- WiFi credential fixes (Tasks 2, 3) - Affects critical functionality
- LittleFS recovery (Task 4) - Error handling changes

**Mitigation:**
- Individual commits allow easy rollback
- Comprehensive testing after each fix
- Debug messages help identify issues quickly

---

## Estimated Time

- Task 1: 15 minutes
- Task 2: 30 minutes
- Task 3: 20 minutes
- Task 4: 30 minutes
- Task 5: 45 minutes
- Task 6: 15 minutes
- Task 7: 30 minutes
- Task 8: 20 minutes

**Total: ~3 hours**

---

**END OF IMPLEMENTATION PLAN**
