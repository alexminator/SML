#ifndef SETTINGS_H
#define SETTINGS_H

// ============================================================================
// CONFIGURACIÓN — extraída a archivos especializados en src/config/
// ============================================================================
#include "config/pins.h"     // Pines GPIO
#include "config/config.h"   // Constantes de comportamiento

// ============================================================================
// EFECTO FIRE
// ============================================================================

uint8_t fireCooling = 55;
uint8_t fireSparking = 50;
bool fireReverse = false;

// ============================================================================
// EFECTO SINELON
// ============================================================================

uint8_t sinelonBeat = 23;
uint8_t sinelonFade = 2;

// ============================================================================
// EFECTO TWINKLE
// ============================================================================

uint8_t twinkleSpeed = 8;
uint8_t twinkleIntensity = 160;
bool twinkleRedCool = true;

// ============================================================================
// EFECTO RAINBOW BEAT
// ============================================================================

uint8_t rainbowSpeed = 30;
uint8_t rainbowDelta = 8;

// ============================================================================
// EFECTO MOVING DOT
// ============================================================================

uint8_t movingDotSpeed = 30;
uint8_t movingDotFade = 10;

// ============================================================================
// EFECTO RIPPLE
// ============================================================================

uint8_t rippleSize = 3;
bool rippleMirror = false;

// ============================================================================
// EFECTO COMET
// ============================================================================

uint8_t cometSpeed = 8;
uint8_t cometTrail = 4;
bool cometBlur = false;

// ============================================================================
// EFECTO BREATH
// ============================================================================

uint8_t breathSpeed = 128;

// ============================================================================
// EFECTO COLOR SWEEP
// ============================================================================

uint8_t sweepSpeed = 128;

// ============================================================================
// EFECTO JUGGLE
// ============================================================================

uint8_t juggleDots = 4;
uint8_t juggleSpeed = 64;
uint8_t juggleIntensity = 128;

// ============================================================================
// VARIABLES GLOBALES LEDS Y COLOR
// ============================================================================

CRGB leds[N_PIXELS];
uint8_t myhue = 0;
CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette;

// ============================================================================
// VARIABLES VU METER
// ============================================================================

uint8_t volCount = 0;
int vol[SAMPLES];
int lvl = 0;
uint8_t volCountLeft = 0;
int volLeft[SAMPLES];
int lvlLeft = 0;
int minLvlAvgLeft = 0;
int maxLvlAvgLeft = 512;
bool is_centered = false;

// ============================================================================
// OBJECTOS GLOBALES (definiciones)
// ============================================================================

// DHT Sensor
DHT_Unified dht(DHTPIN, DHTTYPE);
float temp;
float hum;

// Power Switch for Bluetooth Module
bool bt_powerState = false;

// Lamp Switch
bool lampState = false;

// Battery
int readCount = 0;
int lvlCharge;
Battery18650Stats batteryStats(ADC_PIN, CONV_FACTOR, READS, MAXV, MINV);

// WiFi credentials (loaded from Preferences in setup())
char savedSSID[33] = {0};  // SSID max 32 chars + null
char savedPass[65] = {0};  // Password max 64 chars + null

// ============================================================================
// VARIABLES BALLS
// ============================================================================

// vImpact0 definido en main.cpp
extern float vImpact0;

uint8_t ballsCount = 3;
bool ballsRandomColors = false;
float h[NUM_BALLS];
float vImpact[NUM_BALLS];
float tCycle[NUM_BALLS];
int pos[NUM_BALLS];
long tLast[NUM_BALLS];
float COR[NUM_BALLS];

// ============================================================================
// POWER MANAGEMENT STATE MACHINE
// ============================================================================

enum PowerState {
    POWER_AC_MODE,            // AC power connected - full operation
    POWER_BATTERY_ACTIVE,     // Battery + WebSocket client connected
    POWER_BATTERY_SLEEP,      // Battery + no clients (savings mode)
    POWER_BATTERY_CONNECTING   // Battery + attempting connection
};

// State machine variables
PowerState currentPowerState = POWER_AC_MODE;
PowerState previousPowerState = POWER_AC_MODE;
unsigned long lastStateChange = 0;
unsigned long sleepCycleStart = 0;
bool webSocketClientConnected = false;
bool onBatteryPower = false;
bool powerManagementControllingWiFi = false;

// ============================================================================
// GLOBAL EXTERN DECLARATIONS
// ============================================================================

extern float temp;
extern float hum;
extern bool bt_powerState;
extern bool lampState;
extern int lvlCharge;
extern char savedSSID[33];
extern char savedPass[65];

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
// OBJETOS GLOBALES (extern)
// ============================================================================

extern StripLed stripLed;
extern Led onboard_led;
extern Battery batt;
extern DHT_Unified dht;

// ============================================================================
// DECLARACIÓN DE FUNCIONES
// ============================================================================

void readSensor();

#endif // SETTINGS_H
