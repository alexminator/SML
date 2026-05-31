#ifndef SETTINGS_H
#define SETTINGS_H


// ============================================================================
// LED STRIP (NEOPIXEL)
// ============================================================================

#define STRIP_PIN 4
#define N_PIXELS 24
#define VOLTS 5
#define MAX_MILLIAMPS 500
#define COLOR_ORDER GRB
#define LED_TYPE WS2812B

// ============================================================================
// ONBOARD LED
// ============================================================================

#define ONBOARD_LED_PIN LED_BUILTIN

// ============================================================================
// DHT SENSOR (TEMPERATURA Y HUMEDAD)
// ============================================================================

#define DHTPIN 23
#define DHTTYPE DHT22

// ============================================================================
// BLUETOOTH MODULE RELAY
// ============================================================================

#define SWITCH_PIN 18

// ============================================================================
// BLUETOOTH BUTTON EMULATION (MOSFETS)
// ============================================================================

#define VOLUMENUP_PIN 5
#define VOLUMENDOWN_PIN 19
#define PLAY_PIN 21

// ============================================================================
// LAMP RELAY
// ============================================================================

#define LAMP_PIN 32

// ============================================================================
// BATTERY MONITORING (TP4056)
// ============================================================================

#define CHARGE_PIN 34
#define FULL_CHARGE_PIN 35
#define ADC_PIN 33
#define CONV_FACTOR 1.702
#define READS 30
#define MAXV 4.00
#define MINV 3.20
#define BATT_THRESHOLD 30
#define MAX_READS 10
#define FULL_READS 10

// ============================================================================
// VU METER (AUDIO)
// ============================================================================

#define AUDIO_IN_PIN 36
#define DC_OFFSET 0
#define NOISE 30
#define SAMPLES 60
#define TOP (N_PIXELS + 2)
#define PEAK_FALL 20
#define N_PIXELS_HALF (N_PIXELS / 2)
#define BIAS 1850

// ----------------------------------------------------------------------------
// Definition of macros
// ----------------------------------------------------------------------------
// Constants (Magic Numbers)
// ----------------------------------------------------------------------------
// Stack monitoring thresholds
const uint32_t STACK_WARNING_THRESHOLD = 256;      // Stack low warning threshold (bytes)
const uint32_t STACK_CRITICAL_THRESHOLD = 128;     // Stack critical threshold (bytes)

// WiFi connection
const int WIFI_MAX_ATTEMPTS = 40;                 // Max WiFi connection attempts
const unsigned long WIFI_RETRY_DELAY = 500;       // WiFi connection retry delay (ms)
const unsigned long WIFI_MONITOR_INTERVAL = 2000;

// Timing
const unsigned long LED_ERROR_FLASH_CYCLE = 200;  // LED error flash cycle (ms)
const unsigned long LED_ERROR_FLASH_ON = 50;      // LED error flash on time (ms)

// Battery monitoring
const unsigned long BATTERY_CHECK_INTERVAL = 3000; // Battery check interval (ms)

// LittleFS timeout
const unsigned long LITTLEFS_TIMEOUT = 30000;     // LittleFS error timeout (ms)

// WebSocket
const unsigned long WEBSOCKET_UPDATE_INTERVAL = 1000;  // 1 second for responsive UI
const uint8_t WEBSOCKET_STACK_CHECK_CYCLES = 10;       // Check stack every N cycles

// ----------------------------------------------------------------------------
// Definition of global constants
// ----------------------------------------------------------------------------

// DHT Sensor
DHT_Unified dht(DHTPIN, DHTTYPE);
float temp;
float hum;

// Temperature/Humidity sensor monitoring
const unsigned long SENSOR_CHECK_INTERVAL = 5000;  // Sensor check interval (ms)

// Power Switch for Bluetooth Module
bool bt_powerState = false;

// Emulate BT Button
const unsigned long long_delay = 1000; // More than 1s (Volumen + -)
const unsigned long short_delay = 200; // Short time (FF, RW, PLAY and PAUSE)

// Lamp Switch
bool lampState = false;

// Battery
int readCount = 0;
int lvlCharge;
Battery18650Stats batteryStats(ADC_PIN, CONV_FACTOR, READS, MAXV, MINV);

// WiFi credentials (loaded from Preferences in setup())
char savedSSID[33] = {0};  // SSID max 32 chars + null
char savedPass[65] = {0};  // Password max 64 chars + null

//-----------------------------------------------------------------------------
// Power Management State Machine
//-----------------------------------------------------------------------------

// State definitions
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
bool powerManagementControllingWiFi = false;  // Signal to TaskWiFiMonitor

// Timing constants
const unsigned long SLEEP_DURATION = 60000;        // 60 seconds WiFi sleep
const unsigned long AWAKE_DURATION = 10000;         // 10 seconds WiFi awake
const unsigned long POWER_CHANGE_DEBOUNCE = 3000;   // 3 seconds debounce
const unsigned long WS_WAIT_DURATION = 30000;        // 30 seconds wait for WebSocket

// Critical battery threshold
const int BATTERY_CRITICAL_LEVEL = 15;  // 15%

// ============================================================================
// EFECTO BALLS
// ============================================================================

#define GRAVITY -1
#define h0 1
#define NUM_BALLS 3

// vImpact0 definido en main.cpp — necesario para Balls.h
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
// VARIABLES DE ESTADO GLOBALES
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