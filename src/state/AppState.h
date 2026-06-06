// ──────────────────────────────────────────────────────────────────────────────
// AppState.h — Declaraciones extern de todo el estado global
// ──────────────────────────────────────────────────────────────────────────────
// Las definiciones REALES están en AppState.cpp (única definición, fin del ODR).
// Los structs y enums se definen aquí (los tipos sí van en headers).
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include <FastLED.h>
#include <DHT_U.h>
#include <Battery18650Stats.h>
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
// STRUCT: BATTERY
// ============================================================================

struct Battery {
    double battVolts;
    int battLvl;
    bool fullBatt;
    bool chargeState;

    Battery() : battVolts(0), battLvl(0), fullBatt(false), chargeState(false) {}

    void battMonitor();
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

extern PowerState currentPowerState;
extern PowerState previousPowerState;
extern unsigned long lastStateChange;
extern unsigned long sleepCycleStart;
extern bool webSocketClientConnected;
extern bool onBatteryPower;
extern bool powerManagementControllingWiFi;

