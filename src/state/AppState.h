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
    void runFire();
    void runMovingDot();
    void runRainbowBeat();
    void runRedWhiteBlue();
    void runRipple();
    void runTwinkle();
    void runBalls();
    void runJuggle();
    void runSinelon();
    void runComet();
    void runBreath();
    void runColorSweep();
    void runRainbowVU();
    void runOldVU();
    void runRainbowHueVU();
    void runRippleVU();
    void runThreebarsVU();
    void runOceanVU();
    void runTemperature();
    void runBattery();
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

// ============================================================================
// EFECTO FIRE PARAMS
// ============================================================================

extern uint8_t fireCooling;
extern uint8_t fireSparking;
extern bool fireReverse;

// ============================================================================
// EFECTO SINELON PARAMS
// ============================================================================

extern uint8_t sinelonBeat;
extern uint8_t sinelonFade;

// ============================================================================
// EFECTO TWINKLE PARAMS
// ============================================================================

extern uint8_t twinkleSpeed;
extern uint8_t twinkleIntensity;
extern bool twinkleRedCool;

// ============================================================================
// EFECTO RAINBOW BEAT PARAMS
// ============================================================================

extern uint8_t rainbowSpeed;
extern uint8_t rainbowDelta;

// ============================================================================
// EFECTO MOVING DOT PARAMS
// ============================================================================

extern uint8_t movingDotSpeed;
extern uint8_t movingDotFade;

// ============================================================================
// EFECTO RIPPLE PARAMS
// ============================================================================

extern uint8_t rippleSize;
extern bool rippleMirror;

// ============================================================================
// EFECTO COMET PARAMS
// ============================================================================

extern uint8_t cometSpeed;
extern uint8_t cometTrail;
extern bool cometBlur;

// ============================================================================
// EFECTO BREATH PARAMS
// ============================================================================

extern uint8_t breathSpeed;

// ============================================================================
// EFECTO COLOR SWEEP PARAMS
// ============================================================================

extern uint8_t sweepSpeed;

// ============================================================================
// EFECTO JUGGLE PARAMS
// ============================================================================

extern uint8_t juggleDots;
extern uint8_t juggleSpeed;
extern uint8_t juggleIntensity;

// ============================================================================
// VU METER VARIABLES
// ============================================================================

extern uint8_t volCount;
extern int vol[SAMPLES];
extern int lvl;
extern uint8_t volCountLeft;
extern int volLeft[SAMPLES];
extern int lvlLeft;
extern int minLvlAvgLeft;
extern int maxLvlAvgLeft;
extern bool is_centered;

// ============================================================================
// BALLS VARIABLES
// ============================================================================

extern float vImpact0;

extern uint8_t ballsCount;
extern bool ballsRandomColors;
extern float h[NUM_BALLS];
extern float vImpact[NUM_BALLS];
extern float tCycle[NUM_BALLS];
extern int pos[NUM_BALLS];
extern long tLast[NUM_BALLS];
extern float COR[NUM_BALLS];

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

