// ──────────────────────────────────────────────────────────────────────────────
// PowerMgr.cpp — Power Management State Machine
// ──────────────────────────────────────────────────────────────────────────────
#include "PowerMgr.h"
#include "../state/AppState.h"
#include "net/WebSocket.h"

#include "config/debug_config.h"

#include <WiFi.h>

// ============================================================================
// Transition to new power state with logging
// ============================================================================

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

// ============================================================================
// Apply hardware configuration for each state
// ============================================================================

void applyStateConfiguration(PowerState state) {
    switch (state) {
        case POWER_AC_MODE:
            // Full power mode - everything works normally
            WiFi.setSleep(false);
            setCpuFrequencyMhz(240);
            powerManagementControllingWiFi = false;  // TaskWiFiMonitor handles WiFi
            // Neopixel enabled by default in AC mode
            // ESP32 LEDs: ON (built-in, always on)
            break;

        case POWER_BATTERY_ACTIVE:
            // Battery with active WebSocket user
            WiFi.setSleep(false);
            setCpuFrequencyMhz(240);
            powerManagementControllingWiFi = true;  // Power management controls WiFi
            // Neopixel: OFF (user requirement)
            // ESP32 LEDs: ON (built-in, always on)
            FastLED.clear();
            FastLED.show();
            break;

        case POWER_BATTERY_SLEEP:
            // Maximum power savings
            WiFi.setSleep(true);
            setCpuFrequencyMhz(80);
            powerManagementControllingWiFi = true;  // Power management controls WiFi
            // Neopixel: OFF
            // ESP32 LEDs: ON (built-in, always on)
            break;

        case POWER_BATTERY_CONNECTING:
            // Attempting to connect + wait for client
            WiFi.setSleep(false);
            setCpuFrequencyMhz(240);
            powerManagementControllingWiFi = true;  // Power management controls WiFi
            // Neopixel: OFF
            // ESP32 LEDs: ON (built-in, always on)
            break;
    }
}

// ============================================================================
// Power Management Event Handlers
// ============================================================================

// Handle power source changes with debounce
void onPowerSourceChanged(bool nowOnBattery) {
    static unsigned long lastPowerChangeNotice = 0;

    // Debounce: ignore rapid changes (< 3 seconds)
    if (millis() - lastPowerChangeNotice < POWER_CHANGE_DEBOUNCE) {
        #ifdef DEBUG_POWER_MANAGEMENT
        debuglnW("Power source change ignored (debounce)");
        #endif
        return;
    }

    lastPowerChangeNotice = millis();
    onBatteryPower = nowOnBattery;

    if (nowOnBattery) {
        #ifdef DEBUG_POWER_MANAGEMENT
        debuglnE("⚡ AC POWER LOST - Switching to battery mode");
        #endif
        transitionToState(POWER_BATTERY_CONNECTING);
    } else {
        #ifdef DEBUG_POWER_MANAGEMENT
        debuglnD("🔌 AC POWER RESTORED - Switching to AC mode");
        #endif
        transitionToState(POWER_AC_MODE);
    }
}

// Handle critically low battery level
void onCriticalBatteryLevel() {
    #ifdef DEBUG_POWER_MANAGEMENT
    debugE("⚠️ CRITICAL BATTERY: ");
    debugD_NUM(batt.battLvl, "%d");
    debuglnE("%");
    #endif

    // Force maximum power savings
    WiFi.setSleep(true);
    setCpuFrequencyMhz(80);

    // Ensure Neopixel strip is off
    FastLED.clear();
    FastLED.show();

    // Stay in current state but with aggressive settings
    // State machine will handle sleep cycles
}

// ============================================================================
// State Handlers
// ============================================================================

// Handle BATTERY_CONNECTING state: Try WiFi, wait 30s for WebSocket client
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
                #ifdef DEBUG_POWER_MANAGEMENT
                debugD("WiFi attempt ");
                debugD_NUM(connectionAttempts, "%d");
                debugD("/3\n");
                #endif
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
            #ifdef DEBUG_POWER_MANAGEMENT
            debuglnD("🎉 WebSocket client connected - entering ACTIVE mode");
            #endif
            transitionToState(POWER_BATTERY_ACTIVE);
            return;
        }

        #ifdef DEBUG_POWER_MANAGEMENT
        static unsigned long lastWaitNotice = 0;
        if (now - lastWaitNotice > 10000) {
            debugD("⏳ Waiting for WebSocket client... ");
            debugD_NUM((40000 - elapsedInState) / 1000, "%d");
            debuglnD("s remaining");
            lastWaitNotice = now;
        }
        #endif
    }
    // Phase 3: Timeout - no client connected
    else {
        #ifdef DEBUG_POWER_MANAGEMENT
        debuglnD("⏱️ Timeout - no WebSocket client, entering sleep mode");
        #endif
        transitionToState(POWER_BATTERY_SLEEP);
    }
}

// Handle BATTERY_SLEEP state: 60s WiFi OFF / 10s WiFi ON cycle
void handleBatterySleepState() {
    unsigned long now = millis();
    unsigned long elapsedInCycle = now - sleepCycleStart;

    static bool sleepNotified = false;
    static bool awakeNotified = false;

    if (elapsedInCycle < SLEEP_DURATION) {
        // === PHASE 1: SLEEP (0-60 seconds) ===
        #ifdef DEBUG_POWER_MANAGEMENT
        if (!sleepNotified) {
            debuglnD("😴 Entering WiFi sleep (60s)");
            sleepNotified = true;
        }
        #endif

        // WiFi in modem sleep mode
        if (WiFi.getSleep() == false) {
            WiFi.setSleep(true);
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
        if (!awakeNotified) {
            debuglnD("👀 Waking up WiFi (10s)");
            awakeNotified = true;
        }
        #endif

        // Enable WiFi for connection attempts
        WiFi.setSleep(false);
        setCpuFrequencyMhz(240);

        // Attempt to connect if not connected
        if (WiFi.status() != WL_CONNECTED) {
            static int connectionAttempts = 0;
            if (connectionAttempts < 3) {
                WiFi.begin(savedSSID, savedPass);
                connectionAttempts++;
                #ifdef DEBUG_POWER_MANAGEMENT
                debugD("WiFi connection attempt ");
                debugD_NUM(connectionAttempts, "%d");
                debugD("/3\n");
                #endif
            }
        } else {
            // WiFi connected! Go to CONNECTING state (wait 30s for client)
            #ifdef DEBUG_POWER_MANAGEMENT
            debuglnD("✅ WiFi connected - checking for WebSocket client");
            #endif
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

// ============================================================================
// Main state machine update function - call from loop()
// ============================================================================

void updatePowerStateMachine() {
    // Check for critical battery first
    if (onBatteryPower && batt.battLvl < BATTERY_CRITICAL_LEVEL) {
        onCriticalBatteryLevel();
        return;  // Handle critical case separately
    }

    switch (currentPowerState) {
        case POWER_AC_MODE:
            // AC power: full operation, no restrictions
            // WiFi, CPU, Neopixel all at full capacity
            if (WiFi.getSleep() == true) {
                WiFi.setSleep(false);
            }
            if (getCpuFrequencyMhz() != 240) {
                setCpuFrequencyMhz(240);
            }
            break;

        case POWER_BATTERY_ACTIVE:
            // Battery with active WebSocket user
            // Full WiFi/CPU for responsiveness, Neopixel OFF
            if (WiFi.getSleep() == true) {
                WiFi.setSleep(false);
            }
            if (getCpuFrequencyMhz() != 240) {
                setCpuFrequencyMhz(240);
            }

            // Ensure Neopixel is OFF on battery
            FastLED.clear();
            FastLED.show();

            // If client disconnects, go back to CONNECTING (wait 30s)
            if (!webSocketClientConnected) {
                #ifdef DEBUG_POWER_MANAGEMENT
                debuglnD("🔌 WebSocket disconnected - waiting 30s for reconnect");
                #endif
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

// ============================================================================
// Periodic verification of WebSocket client connections
// ============================================================================

void checkWebSocketClients() {
    static unsigned long lastCheck = 0;

    if (millis() - lastCheck > 5000) {  // Every 5 seconds
        bool actuallyConnected = (ws.count() > 0);

        if (actuallyConnected != webSocketClientConnected) {
            #ifdef DEBUG_POWER_MANAGEMENT
            debugD("WebSocket status changed: ");
            debugD(webSocketClientConnected ? "connected" : "disconnected");
            debugD(" → ");
            debuglnD(actuallyConnected ? "connected" : "disconnected");
            #endif

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
