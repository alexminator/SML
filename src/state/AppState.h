// ──────────────────────────────────────────────────────────────────────────────
// AppState.h — Declaraciones extern de todo el estado global
// ──────────────────────────────────────────────────────────────────────────────
// Las definiciones REALES están en AppState.cpp (única definición, fin del ODR).
// Los structs y enums se definen aquí (los tipos sí van en headers).
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <DHT_U.h>
#include <Battery18650Stats.h>
#include <vector>
#include "config/pins.h"
#include "config/config.h"

// ============================================================================
// STRUCT: ONBOARD LED
// ============================================================================

struct Led {
    uint8_t pin;
    bool on;
    void update() {
        digitalWrite(pin, on ? HIGH : LOW);
    }
};

// ============================================================================
// STRUCT: BATTERY LOG ENTRY (for LittleFS persistence)
// ============================================================================

struct BattLogEntry {
    uint32_t uptime;    // millis() / 1000 at reading time
    float voltage;      // battery voltage
    uint8_t level;      // battery level percent (0-100)
};

// ============================================================================
// STRUCT: BATTERY
// ============================================================================

struct Battery {
    double battVolts;
    double filteredVolts;  // EMA-smoothed voltage for logging (filters ADC noise)
    int battLvl;
    bool fullBatt;
    bool chargeState;

    // Battery history log (circular buffer, LittleFS-backed)
    BattLogEntry battLog[BATT_LOG_SIZE];
    int battLogCount;         // total entries logged so far (capped at BATT_LOG_SIZE)
    int battLogHead;          // next write position (circular)
    double lastLoggedVoltage; // last voltage written to log (for delta detection)
    uint32_t lastLogUptime;   // uptime of last write (for time-based fallback)
    int writesSinceSave;      // counter, triggers LittleFS write every BATT_LOG_SAVE_INTERVAL
    bool logLoaded;           // true after loadBatteryLog() has been called

    Battery()
      : battVolts(0), filteredVolts(0), battLvl(0), fullBatt(false), chargeState(false),
        battLogCount(0), battLogHead(0),
        lastLoggedVoltage(0.0), lastLogUptime(0), writesSinceSave(0),
        logLoaded(false)
    {}

    void battMonitor();
    void logBatteryReading();
    void saveBatteryLog();
    void loadBatteryLog();
    void getBatteryLog(BattLogEntry* outBuf, int* outCount);
};

// ============================================================================
// STRUCT: STRIPLED
// ============================================================================

struct StripLed {
    int R;
    int G;
    int B;
    int brightness;
    int effectId;
    bool powerState;

    StripLed();
    void simpleColor(int ar, int ag, int ab, int brightness);
    void update();
    void clear();
};

// ============================================================================
// POWER MANAGEMENT STATE MACHINE
// ============================================================================

enum PowerState {
    POWER_AC_MODE,            // AC power connected - full operation
    POWER_BATTERY_ACTIVE,     // Battery + WebSocket client connected
    POWER_BATTERY_SLEEP,      // Battery + no clients (savings mode)
    POWER_BATTERY_CONNECTING   // Battery + attempting connection
};

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

extern CRGB leds[N_PIXELS];
extern StripLed stripLed;
extern Led onboard_led;
extern Battery batt;
extern DHT_Unified dht;
extern Battery18650Stats batteryStats;

// ============================================================================
// GLOBAL STATE VARIABLES
// ============================================================================

extern float temp;
extern float hum;
extern bool bt_powerState;
extern bool lampState;
extern int readCount;
extern int lvlCharge;
extern char savedSSID[33];
extern char savedPass[65];
extern uint8_t myhue;
extern CRGBPalette16 currentPalette;
extern CRGBPalette16 targetPalette;

// ============================================================================
// RTOS SYNCHRONIZATION
// ============================================================================

extern SemaphoreHandle_t dataMutex;
extern SemaphoreHandle_t wifiMutex;

/// Initialize RTOS mutexes (dataMutex, wifiMutex)
void initMutexes();

// ============================================================================
// DIRTY FLAG — evita notificaciones innecesarias
// ============================================================================

extern uint32_t stateGeneration;

// ============================================================================
// VU METER VARIABLES
// ============================================================================

extern bool is_centered;

// ============================================================================
// POWER MANAGEMENT STATE
// ============================================================================

extern uint8_t randomMode;  // 0=off, 1=randomFX, 2=randomVU

// ── Random FX config (shared across all clients) ──
extern String randomFXMode;                // "all", "category", "playlist"
extern int randomFXDuration;               // seconds between cycles
extern std::vector<int> randomFXPool;      // flat effect ID pool for cycling
extern std::vector<int> randomFXCategories; // category indices (0-4), only for category mode
extern unsigned long lastRandomSwitch;     // millis() of last switch during cycling
extern int randomPlaylistIndex;            // current index in playlist mode

// ── Random VU config (shared across all clients) ──
extern int randomVUDuration;               // seconds between VU cycles
extern std::vector<int> randomVUPool;      // VU effect ID pool for cycling

extern PowerState currentPowerState;
extern PowerState previousPowerState;
extern unsigned long lastStateChange;
extern unsigned long sleepCycleStart;
extern bool webSocketClientConnected;
extern bool onBatteryPower;
extern bool powerManagementControllingWiFi;

