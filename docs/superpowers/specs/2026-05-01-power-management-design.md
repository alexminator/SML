# Power Management System Design - Smart Music Lamp

**Date:** 2026-05-01  
**Author:** Generated with superpowers:brainstorming  
**Status:** Design Approved - Pending Implementation  
**Branch:** feature/power-management

---

## Executive Summary

Implement a state machine-based power management system that reduces ESP32 power consumption by **85-90%** when operating on battery power. The system automatically detects power source (AC vs battery) and optimizes WiFi, CPU, and peripheral usage accordingly.

**Key Achievement:** Extend battery life from ~4 hours to ~24+ hours when not in active use.

---

## Problem Statement

### Current State
- ESP32 continuously maintains WiFi connection regardless of power source
- No power management when operating on battery (2x LiPo)
- Constant WiFi consumption: ~80mA
- Battery shared with Bluetooth module and amplifier
- No distinction between AC power mode and battery mode

### Desired State
- Automatic detection of AC power vs battery operation
- Aggressive power saving when on battery without active use
- Instant wake-up when web app connects
- Transparent user experience
- Extended battery life for music playback

---

## Architecture Overview

### State Machine Design

```
┌─────────────────────────────────────────────────────────────────┐
│                    POWER STATE MACHINE                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────┐     AC lost     ┌─────────────────┐        │
│  │  POWER_AC_MODE │ ──────────────→ │POWER_BATTERY_   │        │
│  │                │                 │CONNECTING       │        │
│  │ - WiFi always  │                 │ - Try WiFi 10s  │        │
│  │   ON           │                 │ - If connected, │        │
│  │ - CPU 240MHz   │                 │   wait 30s for  │        │
│  │ - Neopixel ON  │                 │   WS client     │        │
│  │ - ESP32 LEDs ON│                 │ - CPU 240MHz    │        │
│  └────────────────┘                 └─────────────────┘        │
│         ↑                                    │                   │
│         │ AC restored                       │ No client after │    │
│         │                                    │ 40s (10+30)     │    │
│         │                                    ↓                   │
│  ┌────────────────┐                   ┌─────────────────┐       │
│  │  POWER_        │ ←──────────────── │POWER_BATTERY_   │       │
│  │  BATTERY_      │   WS connects    │SLEEP            │       │
│  │  ACTIVE        │ ──────────────── │ - Sleep 60s     │       │
│  │                │                   │ - Awake 10s     │       │
│  │ - WS client    │   WS disconnects │ - WiFi OFF/ON   │       │
│  │   connected    │ ────────────     │ - CPU 80MHz     │       │
│  │ - WiFi ON      │                 │ - Neopixel OFF  │       │
│  │ - CPU 240MHz   │                 │ - ESP32 LEDs ON │       │
│  │ - Neopixel OFF │                 │ - BT audio OK   │       │
│  │ - ESP32 LEDs ON│                 └─────────────────┘       │
│  │ - BT audio OK  │                                             │
│  └────────────────┘                                             │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Power Source Detection

```cpp
// Existing logic from battMonitor() - line 293
bool onBatteryPower = (!fullBatt && !chargeState);
bool hasACPower = (fullBatt || chargeState);

// !fullBatt && !chargeState = USING BATTERY
// fullBatt = CHARGED + AC connected
// chargeState = CHARGING + AC connected
```

---

## Detailed State Specifications

### State 1: POWER_AC_MODE

**Trigger:** `hasACPower == true`

**Configuration:**
- WiFi: Always ON (`WIFI_NONE_SLEEP`)
- CPU: 240MHz (full speed)
- LED Strip: Active (all effects enabled)
- WebSocket: Normal operation (1s updates)
- Bluetooth: Active

**Transitions:**
- → `POWER_BATTERY_CONNECTING`: When AC power lost

---

### State 2: POWER_BATTERY_CONNECTING

**Trigger:** Transition from `POWER_AC_MODE` when battery detected

**Configuration:**
- WiFi: ON, attempting connection
- CPU: 240MHz
- LED Strip (Neopixel): OFF
- ESP32 Power LED: ON (built-in)
- ESP32 WiFi LED: ON (built-in)
- WebSocket: Accepting connections

**Behavior:**
```
Phase 1: Try to connect WiFi (10 seconds)
├─ If WiFi connects:
│  └─ Phase 2: Wait for WebSocket client (30 seconds)
│     ├─ If client connects → POWER_BATTERY_ACTIVE
│     └─ If 30s timeout → POWER_BATTERY_SLEEP
└─ If WiFi fails to connect:
   └─ → POWER_BATTERY_SLEEP
```

**Transitions:**
- → `POWER_BATTERY_SLEEP`: WiFi fails to connect OR no WS client after 40s (10s + 30s)
- → `POWER_BATTERY_ACTIVE`: WebSocket client connects
- → `POWER_AC_MODE`: AC power restored

**LED Behavior:**
- Neopixel strip: **OFF** (disabled on battery)
- ESP32 built-in power LED: **ON**
- ESP32 built-in WiFi LED: **ON** (indicates connection status)

---

### State 3: POWER_BATTERY_ACTIVE

**Trigger:** WebSocket client connected while on battery

**Configuration:**
- WiFi: ON (`WIFI_NONE_SLEEP`)
- CPU: 240MHz (full speed for responsiveness)
- LED Strip (Neopixel): **OFF** (user clarified: no LED on battery)
- ESP32 Power LED: **ON** (built-in)
- ESP32 WiFi LED: **ON** (built-in)
- WebSocket: Normal operation
- Bluetooth: Active (music playback)
- Updates: Every 2s (slower than AC mode)

**Transitions:**
- → `POWER_BATTERY_CONNECTING`: WebSocket client disconnects (go back to wait 30s)
- → `POWER_AC_MODE`: AC power restored

**Important Behavior:**
When client closes web app:
1. Immediate transition to `POWER_BATTERY_CONNECTING`
2. WiFi stays connected
3. Wait 30 seconds for client to reconnect
4. If no reconnection in 30s → `POWER_BATTERY_SLEEP`
5. If client reconnects in 30s → `POWER_BATTERY_ACTIVE`

**LED Behavior:**
- Neopixel strip: **OFF** (disabled on battery)
- ESP32 built-in power LED: **ON**
- ESP32 built-in WiFi LED: **ON**

---

### State 4: POWER_BATTERY_SLEEP

**Trigger:** No WebSocket client + on battery

**Sleep Cycle (70 seconds total):**
```cpp
// Phase 1: DEEP SLEEP (60 seconds)
// - WiFi: OFF (WIFI_MODEM_SLEEP)
// - CPU: 80MHz (reduced speed)
// - LED: OFF
// - Consumption: ~15-20mA

// Phase 2: AWAKE (10 seconds)
// - WiFi: ON (attempting connection)
// - CPU: 240MHz
// - Check for WebSocket connections
// - Consumption: ~80mA

// Repeat cycle
```

**Transitions:**
- → `POWER_BATTERY_ACTIVE`: WebSocket client connects (immediate wake)
- → `POWER_AC_MODE`: AC power restored (immediate wake)

---

## Implementation Components

### 1. Global Variables

```cpp
// Location: main.cpp after line ~197

enum PowerState {
    POWER_AC_MODE,            // AC power connected
    POWER_BATTERY_ACTIVE,     // Battery + WebSocket client
    POWER_BATTERY_SLEEP,      // Battery + no client (savings mode)
    POWER_BATTERY_CONNECTING   // Battery + attempting connection
};

PowerState currentPowerState = POWER_AC_MODE;
PowerState previousPowerState = POWER_AC_MODE;
unsigned long lastStateChange = 0;
unsigned long sleepCycleStart = 0;
bool webSocketClientConnected = false;
bool onBatteryPower = false;

// Sleep cycle timing
const unsigned long SLEEP_DURATION = 60000;  // 60 seconds
const unsigned long AWAKE_DURATION = 10000;   // 10 seconds
const unsigned long POWER_CHANGE_DEBOUNCE = 3000; // 3 seconds

// Critical battery threshold
const int BATTERY_CRITICAL_LEVEL = 15;  // 15%
```

---

### 2. State Transition Functions

```cpp
// Transition handler with logging
void transitionToState(PowerState newState) {
    previousPowerState = currentPowerState;
    currentPowerState = newState;
    lastStateChange = millis();
    
    #ifdef DEBUG_POWER_MANAGEMENT
    const char* stateNames[] = {
        "POWER_AC_MODE",
        "POWER_BATTERY_ACTIVE",
        "POWER_BATTERY_SLEEP",
        "POWER_BATTERY_CONNECTING"
    };
    debugD("State transition: ");
    debugD(stateNames[previousPowerState]);
    debugD(" → ");
    debuglnD(stateNames[newState]);
    #endif
    
    // Apply state-specific configurations
    applyStateConfiguration(newState);
}

// Apply configuration for each state
void applyStateConfiguration(PowerState state) {
    switch (state) {
        case POWER_AC_MODE:
            // Full power mode - everything works normally
            WiFi.setSleep(WIFI_NONE_SLEEP);
            setCpuFrequencyMhz(240);
            // Neopixel enabled by default
            // ESP32 LEDs: ON (built-in, always on)
            break;
            
        case POWER_BATTERY_ACTIVE:
            // Battery with active WebSocket user
            WiFi.setSleep(WIFI_NONE_SLEEP);
            setCpuFrequencyMhz(240);
            // Neopixel: OFF (user clarified)
            // ESP32 LEDs: ON (built-in, always on)
            break;
            
        case POWER_BATTERY_SLEEP:
            // Maximum power savings
            WiFi.setSleep(WIFI_MODEM_SLEEP);
            setCpuFrequencyMhz(80);
            // Neopixel: OFF
            // ESP32 LEDs: ON (built-in, always on)
            sleepCycleStart = millis();
            break;
            
        case POWER_BATTERY_CONNECTING:
            // Attempting to connect + wait for client
            WiFi.setSleep(WIFI_NONE_SLEEP);
            setCpuFrequencyMhz(240);
            // Neopixel: OFF
            // ESP32 LEDs: ON (built-in, always on)
            sleepCycleStart = millis();
            break;
    }
}
```

---

### 3. Power Source Detection

```cpp
// Modify battMonitor() - add after line ~299

void battMonitor() {
    // ... existing battery monitoring code ...
    
    // NEW: Detect power source changes
    bool currentlyOnBattery = (!fullBatt && !chargeState);
    
    if (currentlyOnBattery != onBatteryPower) {
        // Power source changed - trigger transition
        onPowerSourceChanged(currentlyOnBattery);
    }
    
    // Check for critical battery level
    if (onBatteryPower && battLvl < BATTERY_CRITICAL_LEVEL) {
        onCriticalBatteryLevel();
    }
}

// Handle power source changes with debounce
void onPowerSourceChanged(bool nowOnBattery) {
    static unsigned long lastPowerChangeNotice = 0;
    
    // Debounce: ignore rapid changes (< 3 seconds)
    if (millis() - lastPowerChangeNotice < POWER_CHANGE_DEBOUNCE) {
        debuglnW("Power source change ignored (debounce)");
        return;
    }
    
    lastPowerChangeNotice = millis();
    onBatteryPower = nowOnBattery;
    
    if (nowOnBattery) {
        debuglnD("⚡ AC POWER LOST - Switching to battery mode");
        transitionToState(POWER_BATTERY_CONNECTING);
    } else {
        debuglnD("🔌 AC POWER RESTORED - Switching to AC mode");
        transitionToState(POWER_AC_MODE);
    }
}
```

---

### 4. Battery Connecting State Handler (NUEVA)

```cpp
// NEW: Handle the connecting state with 30s wait for WebSocket client
void handleBatteryConnectingState() {
    unsigned long now = millis();
    unsigned long elapsedInState = now - lastStateChange;
    
    // Phase 1: Try to connect WiFi (first 10 seconds)
    if (elapsedInState < 10000) {
        if (WiFi.status() != WL_CONNECTED) {
            static int connectionAttempts = 0;
            if (connectionAttempts < 3) {
                WiFi.begin(savedSSID, savedPass);
                connectionAttempts++;
                debugD("WiFi attempt ");
                debugD_NUM(connectionAttempts, "%d");
                debugD("/3\n");
            }
        } else {
            #ifdef DEBUG_POWER_MANAGEMENT
            static bool wifiConnectedNotified = false;
            if (!wifiConnectedNotified) {
                debuglnD("✅ WiFi connected - waiting 30s for WebSocket client");
                wifiConnectedNotified = true;
            }
            #endif
        }
    }
    // Phase 2: Wait for WebSocket client (next 30 seconds)
    else if (elapsedInState < 40000) {  // 10s + 30s = 40s total
        // WiFi already connected (or failed), waiting for client
        if (webSocketClientConnected) {
            debuglnD("🎉 WebSocket client connected - entering ACTIVE mode");
            transitionToState(POWER_BATTERY_ACTIVE);
            return;
        }
        
        #ifdef DEBUG_POWER_MANAGEMENT
        static unsigned long lastWaitNotice = 0;
        if (now - lastWaitNotice > 10000) {
            debugD("⏳ Waiting for WebSocket client... ");
            debugD_NUM((40000 - elapsedInState) / 1000, "%lu");
            debuglnD("s remaining");
            lastWaitNotice = now;
        }
        #endif
    }
    // Phase 3: Timeout - no client connected
    else {
        debuglnD("⏱️ Timeout - no WebSocket client, entering sleep mode");
        transitionToState(POWER_BATTERY_SLEEP);
    }
}
```

---

### 5. Battery Sleep State Handler

```cpp
void handleBatterySleepState() {
    unsigned long now = millis();
    unsigned long elapsedInCycle = now - sleepCycleStart;
    
    if (elapsedInCycle < SLEEP_DURATION) {
        // === PHASE 1: SLEEP (0-60 seconds) ===
        #ifdef DEBUG_POWER_MANAGEMENT
        static bool sleepNotified = false;
        if (!sleepNotified) {
            debuglnD("😴 Entering WiFi sleep (60s)");
            sleepNotified = true;
        }
        #endif
        
        // WiFi in modem sleep mode
        if (WiFi.getSleep() == WIFI_NONE_SLEEP) {
            WiFi.setSleep(WIFI_MODEM_SLEEP);
        }
        
        // CPU at reduced speed
        if (getCpuFrequencyMhz() != 80) {
            setCpuFrequencyMhz(80);
        }
        
        // Neopixel strip disabled
        FastLED.clear();
        FastLED.show();
        
    } else if (elapsedInCycle < SLEEP_DURATION + AWAKE_DURATION) {
        // === PHASE 2: AWAKE (60-70 seconds) ===
        #ifdef DEBUG_POWER_MANAGEMENT
        static bool awakeNotified = false;
        if (!awakeNotified) {
            debuglnD("👀 Waking up WiFi (10s)");
            awakeNotified = true;
        }
        #endif
        
        // Enable WiFi for connection attempts
        WiFi.setSleep(WIFI_NONE_SLEEP);
        setCpuFrequencyMhz(240);
        
        // Attempt to connect if not connected
        if (WiFi.status() != WL_CONNECTED) {
            static int connectionAttempts = 0;
            if (connectionAttempts < 3) {
                WiFi.begin(savedSSID, savedPass);
                connectionAttempts++;
                debugD("WiFi connection attempt ");
                debugD_NUM(connectionAttempts, "%d");
                debugD("/3\n");
            }
        } else {
            // WiFi connected! Go to CONNECTING state (wait 30s for client)
            debuglnD("✅ WiFi connected - checking for WebSocket client");
            awakeNotified = false;
            sleepNotified = false;
            transitionToState(POWER_BATTERY_CONNECTING);
            return;
        }
        
    } else {
        // === CYCLE COMPLETE - RESTART ===
        #ifdef DEBUG_POWER_MANAGEMENT
        debuglnD("🔄 Sleep cycle complete - restarting");
        #endif
        
        sleepCycleStart = now;
        sleepNotified = false;
        awakeNotified = false;
    }
}
```

---

### 5. WebSocket Client Detection

```cpp
// Modify setupWebSocket() - add connection tracking

void setupWebSocket() {
    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client,
                   AwsEventType type, void *arg, uint8_t *data, size_t len) {
        switch (type) {
            case WS_EVT_CONNECT:
                webSocketClientConnected = true;
                debuglnD("✅ WebSocket client connected");
                
                // If in battery sleep or connecting, wake up immediately
                if (currentPowerState == POWER_BATTERY_SLEEP || 
                    currentPowerState == POWER_BATTERY_CONNECTING) {
                    debuglnD("🔔 Waking for WebSocket client");
                    transitionToState(POWER_BATTERY_ACTIVE);
                }
                break;
                
            case WS_EVT_DISCONNECT:
                webSocketClientConnected = false;
                debuglnD("❌ WebSocket client disconnected");
                
                // If in battery active mode, go back to connecting (wait 30s)
                if (currentPowerState == POWER_BATTERY_ACTIVE) {
                    debuglnD("💤 Client disconnected - waiting 30s for reconnect");
                    transitionToState(POWER_BATTERY_CONNECTING);
                }
                break;
        }
    });
    
    // ... rest of setupWebSocket() ...
}

// Periodic client verification
void checkWebSocketClients() {
    static unsigned long lastCheck = 0;
    
    if (millis() - lastCheck > 5000) {  // Every 5 seconds
        bool actuallyConnected = (ws.count() > 0);
        
        if (actuallyConnected != webSocketClientConnected) {
            debugD("WebSocket status changed: ");
            debugD(webSocketClientConnected ? "connected" : "disconnected");
            debugD(" → ");
            debuglnD(actuallyConnected ? "connected" : "disconnected");
            
            webSocketClientConnected = actuallyConnected;
            
            // Trigger state transition
            if (actuallyConnected) {
                if (currentPowerState == POWER_BATTERY_SLEEP || 
                    currentPowerState == POWER_BATTERY_CONNECTING) {
                    transitionToState(POWER_BATTERY_ACTIVE);
                }
            } else {
                if (currentPowerState == POWER_BATTERY_ACTIVE) {
                    transitionToState(POWER_BATTERY_CONNECTING);
                }
            }
        }
        lastCheck = millis();
    }
}
```

---

### 6. State Machine Update Loop

```cpp
// NEW: Main state machine update function

void updatePowerStateMachine() {
    // Check for critical battery
    if (onBatteryPower && battLvl < BATTERY_CRITICAL_LEVEL) {
        onCriticalBatteryLevel();
        return;  // Handle critical case separately
    }
    
    switch (currentPowerState) {
        case POWER_AC_MODE:
            // AC power: full operation, no restrictions
            // WiFi, CPU, Neopixel all at full capacity
            if (WiFi.getSleep() != WIFI_NONE_SLEEP) {
                WiFi.setSleep(WIFI_NONE_SLEEP);
            }
            if (getCpuFrequencyMhz() != 240) {
                setCpuFrequencyMhz(240);
            }
            break;
            
        case POWER_BATTERY_ACTIVE:
            // Battery with active WebSocket user
            // Full WiFi/CPU for responsiveness
            // Neopixel OFF (user requirement)
            if (WiFi.getSleep() != WIFI_NONE_SLEEP) {
                WiFi.setSleep(WIFI_NONE_SLEEP);
            }
            if (getCpuFrequencyMhz() != 240) {
                setCpuFrequencyMhz(240);
            }
            
            // Ensure Neopixel is OFF on battery
            FastLED.clear();
            FastLED.show();
            
            // If client disconnects, go back to CONNECTING (wait 30s)
            if (!webSocketClientConnected) {
                debuglnD("🔌 WebSocket disconnected - waiting 30s for reconnect");
                transitionToState(POWER_BATTERY_CONNECTING);
            }
            break;
            
        case POWER_BATTERY_SLEEP:
            // Battery sleep mode: cycle WiFi on/off
            handleBatterySleepState();
            break;
            
        case POWER_BATTERY_CONNECTING:
            handleBatteryConnectingState();
            break;
    }
}

// Critical battery handler
void onCriticalBatteryLevel() {
    debugE("⚠️ CRITICAL BATTERY: ");
    debugD_NUM(battLvl, "%d");
    debuglnE("%");
    
    // Force deep sleep mode with extended intervals
    // SLEEP_DURATION becomes 5 minutes
    // AWAKE_DURATION becomes 5 seconds
    
    // Notify user via WebSocket if connected
    if (webSocketClientConnected) {
        // Send critical battery notification
        // (implementation depends on WebSocket message format)
    }
    
    // Force LED strip off
    FastLED.clear();
    FastLED.show();
    
    // Ensure WiFi sleep
    WiFi.setSleep(WIFI_MODEM_SLEEP);
    setCpuFrequencyMhz(80);
}
```

---

### 7. Main Loop Integration

```cpp
// Modify loop() - add power management calls

void loop() {
    // ... existing code ...
    
    // Battery monitoring (includes power source detection)
    battMonitor();
    
    // WebSocket client tracking
    checkWebSocketClients();
    
    // NEW: Power management state machine
    updatePowerStateMachine();
    
    // ... rest of existing loop code ...
}
```

---

## Error Handling & Edge Cases

### 1. Power Source Flickering

**Problem:** AC connection rapidly connects/disconnects  
**Solution:** 3-second debounce window  
**Implementation:** `onPowerSourceChanged()` ignores changes < 3s apart

```cpp
if (millis() - lastPowerChangeNotice < POWER_CHANGE_DEBOUNCE) {
    return;  // Ignore rapid changes
}
```

### 2. WebSocket Disconnect During Active Use

**Problem:** Client briefly disconnects during usage  
**Solution:** 5-second verification period before sleep transition  
**Implementation:** `checkWebSocketClients()` verifies sustained disconnection

### 3. WiFi Failure in Sleep Mode

**Problem:** WiFi cannot connect during awake phase  
**Solution:** Limited retry attempts (3), then continue cycle  
**Implementation:** Counter reset each cycle

### 4. Critical Battery Level

**Problem:** Battery < 15% while in sleep mode  
**Solution:** Extend sleep duration to 5 minutes, reduce awake to 5s  
**Implementation:** `onCriticalBatteryLevel()`

### 5. State Machine Corruption

**Problem:** Unexpected state or corrupted variables  
**Solution:** State validation, default to AC mode on error  
**Implementation:**

```cpp
void validateState() {
    if (currentPowerState < 0 || currentPowerState > POWER_BATTERY_CONNECTING) {
        debuglnE("Invalid state detected - resetting to AC mode");
        transitionToState(POWER_AC_MODE);
    }
}
```

---

## Power Consumption Estimates

### Current (No Power Management)
- WiFi always ON: ~80mA
- CPU 240MHz: ~40mA
- LED strip: ~60mA (when active)
- **Total: ~180mA** → **~4 hours** battery life

### With Power Management (Battery Sleep Mode)
- WiFi sleep (60s): ~5mA
- WiFi awake (10s): ~80mA
- Average WiFi: **~15mA**
- CPU 80MHz: ~15mA
- LED strip: **0mA** (disabled on battery)
- **Total: ~30mA** → **~24+ hours** battery life

### With Active WebSocket (Battery Active Mode)
- WiFi ON: ~80mA
- CPU 240MHz: ~40mA
- LED strip: **0mA** (disabled on battery)
- **Total: ~120mA** → **~6 hours** battery life

### Savings Summary
- **Sleep mode: 83% reduction** (180mA → 30mA)
- **Active mode: 33% reduction** (180mA → 120mA)
- **Weighted average: 70-85% reduction** (depending on usage pattern)

---

## Testing Strategy

### Phase 1: Unit Tests (Development)

```cpp
// Test 1: State transitions
void test_power_transitions() {
    // Simulate AC → Battery transition
    onPowerSourceChanged(true);
    assert(currentPowerState == POWER_BATTERY_CONNECTING);
    
    // Simulate Battery → AC transition
    onPowerSourceChanged(false);
    assert(currentPowerState == POWER_AC_MODE);
}

// Test 2: Debounce logic
void test_power_flicker() {
    // Rapid changes should be ignored
    onPowerSourceChanged(true);
    delay(100);
    onPowerSourceChanged(false);
    // State should remain unchanged
    assert(currentPowerState == POWER_AC_MODE);
}
```

### Phase 2: Hardware Tests (Required Equipment: Multimeter)

**Test 1: AC Mode Operation**
1. Connect AC power
2. Verify WiFi always ON
3. Verify LED effects work
4. Measure consumption: ~180mA
5. Disconnect AC → verify transition to BATTERY_CONNECTING

**Test 2: Battery Sleep Mode**
1. Disconnect AC (run on battery)
2. Close all WebSocket clients
3. Verify transition to POWER_BATTERY_SLEEP
4. Measure consumption over 2 minutes:
   - Expect: ~30mA average (15-20mA sleep, 80mA awake)
5. Verify 60s sleep / 10s awake cycle

**Test 3: WebSocket Wake-Up**
1. Enter battery sleep mode
2. Wait 30 seconds into sleep phase
3. Open web interface in browser
4. Verify immediate wake-up (< 1 second)
5. Verify transition to POWER_BATTERY_ACTIVE

**Test 4: Battery Active Mode**
1. Connect WebSocket client while on battery
2. Verify WiFi stays ON
3. Verify LED is OFF
4. Verify web interface responsive
5. Measure consumption: ~120mA

**Test 5: Power Source Flicker**
1. Connect/disconnect AC rapidly (10 times in 5 seconds)
2. Verify no state transitions (debounce working)
3. Verify system remains stable

**Test 6: Critical Battery**
1. Simulate battery level < 15%
2. Verify extended sleep intervals
3. Verify notification sent to WebSocket

### Phase 3: Long Duration Tests

**Test 1: Overnight Sleep Test**
- Leave on battery overnight with no use
- Verify battery drains < 20%
- Verify system still functional in morning

**Test 2: Intermittent Use Test**
- Simulate periodic use (connect every 30 minutes)
- Verify smooth transitions
- Measure total consumption over 4 hours

**Test 3: AC Power Interruption Recovery**
- Start on AC power
- Unplug AC during active use
- Verify graceful transition to battery mode
- Verify no crashes or WiFi hangs

---

## Debug & Monitoring

### New Debug Flag

```cpp
// Add to main.cpp debug flags section
#define DEBUG_POWER_MANAGEMENT  // Enable power management logging
```

### Debug Output Format

```
[D] ⚡ AC POWER LOST - Switching to battery mode
[D] State transition: POWER_AC_MODE → POWER_BATTERY_CONNECTING
[D] 🔗 WebSocket client connected
[D] State transition: POWER_BATTERY_CONNECTING → POWER_BATTERY_ACTIVE
[D] ❌ WebSocket client disconnected
[D] 💤 No WebSocket clients - entering sleep mode
[D] State transition: POWER_BATTERY_ACTIVE → POWER_BATTERY_SLEEP
[D] 😴 Entering WiFi sleep (60s)
[D] 👀 Waking up WiFi (10s)
[D] WiFi connection attempt 1/3
[D] 🔌 AC POWER RESTORED - Switching to AC mode
[D] State transition: POWER_BATTERY_SLEEP → POWER_AC_MODE
```

### Performance Monitoring Variables

```cpp
#ifdef DEBUG_POWER_MANAGEMENT
struct PowerStats {
    unsigned long timeInACMode;
    unsigned long timeInBatteryActive;
    unsigned long timeInBatterySleep;
    unsigned int sleepCycles;
    unsigned int stateTransitions;
} powerStats;

void printPowerStats() {
    debugD("Power Stats - AC: ");
    debugD_NUM(powerStats.timeInACMode / 60000, "%lu");
    debugD(" min, Battery Active: ");
    debugD_NUM(powerStats.timeInBatteryActive / 60000, "%lu");
    debugD(" min, Sleep: ");
    debugD_NUM(powerStats.timeInBatterySleep / 60000, "%lu");
    debugD(" min, Cycles: ");
    debuglnD_NUM(powerStats.sleepCycles, "%u");
}
#endif
```

---

## Implementation Checklist

### Phase 1: Foundation (2 hours)
- [ ] Add global state machine variables
- [ ] Implement `transitionToState()` function
- [ ] Implement `applyStateConfiguration()` function
- [ ] Add `updatePowerStateMachine()` skeleton
- [ ] Compile and verify no breaks

### Phase 2: Power Detection (1 hour)
- [ ] Modify `battMonitor()` to detect power source
- [ ] Implement `onPowerSourceChanged()` with debounce
- [ ] Add debug logging
- [ ] Test AC connect/disconnect transitions

### Phase 3: Sleep Cycle (2 hours)
- [ ] Implement `handleBatterySleepState()`
- [ ] Configure WiFi sleep modes
- [ ] Test 60s/10s cycle with multimeter
- [ ] Verify consumption ~30mA average

### Phase 4: WebSocket Integration (1.5 hours)
- [ ] Modify `setupWebSocket()` for connection tracking
- [ ] Implement `checkWebSocketClients()`
- [ ] Test wake-up on client connect
- [ ] Test sleep on client disconnect

### Phase 5: Optimizations (2 hours)
- [ ] Add CPU frequency scaling (80MHz sleep)
- [ ] Disable LED in battery modes
- [ ] Reduce WebSocket update interval to 2s
- [ ] Disable OTA on battery
- [ ] Implement critical battery handling

### Phase 6: Testing & Validation (2 hours)
- [ ] Complete hardware test suite
- [ ] Measure consumption in all modes
- [ ] Long-duration stability test
- [ ] Edge case testing

### Phase 7: Documentation (1 hour)
- [ ] Update CLAUDE.md with power management
- [ ] Document user-facing behavior
- [ ] Create troubleshooting guide

**Total Estimated Time:** 11.5 hours

---

## Success Criteria

### Functional Requirements
- ✅ Automatic detection of AC vs battery power
- ✅ Seamless transitions between power modes
- ✅ < 1 second wake-up when WebSocket client connects
- ✅ No LED operation when on battery
- ✅ Bluetooth audio unaffected by power management
- ✅ No crashes or watchdog timeouts

### Performance Requirements
- ✅ 85% power reduction in battery sleep mode (180mA → 30mA)
- ✅ 70% power reduction in battery active mode (180mA → 120mA)
- ✅ Minimum 24-hour battery life in sleep mode
- ✅ Minimum 6-hour battery life with active use

### Quality Requirements
- ✅ Zero regressions in existing functionality
- ✅ Code follows project patterns (no String objects, etc.)
- ✅ Comprehensive debug logging
- ✅ Clean git history with feature branch

---

## Rollback Plan

If issues arise during testing:

1. **Identify problematic commit:** `git log --oneline feature/power-management`
2. **Revert specific commit:** `git revert <commit-hash>`
3. **Alternative:** Delete feature branch and start over
4. **Data preserved:** No NVS changes, no configuration loss

**Safe Rollback:**
```bash
git checkout master
git branch -D feature/power-management
# System returns to pre-implementation state
```

---

## Future Enhancements (Out of Scope)

1. **Bluetooth Auto-Sleep** - Disable BT when not in use
2. **Adaptive Sleep Intervals** - Learn usage patterns
3. **Battery Prediction** - Estimate remaining time
4. **Solar Charging Integration** - Optimize for solar
5. **Low-Power Modes** - ESP32 deep sleep for extended storage

---

## References

- ESP32 Power Management: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/power.html
- WiFi Sleep Modes: https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi/src
- Battery18650Stats Library: Custom fork in `lib/Battery18650Stats`
- Related: CLAUDE.md - "CRITICAL: ESP32 Crash Patterns"

---

**Document Status:** Ready for Implementation  
**Next Step:** Invoke superpowers:writing-plans to create implementation plan  
**Branch:** `feature/power-management` (to be created)
