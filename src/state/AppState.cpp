// ──────────────────────────────────────────────────────────────────────────────
// AppState.cpp — Definiciones ÚNICAS de todo el estado global
// ──────────────────────────────────────────────────────────────────────────────
// Cada símbolo definido aquí existe una sola vez en el binario.
// Ningún otro .cpp puede definir estos mismos símbolos (fin del ODR).
// ──────────────────────────────────────────────────────────────────────────────
#include "AppState.h"

// ============================================================================
// RTOS SYNCHRONIZATION
// ============================================================================

SemaphoreHandle_t dataMutex = NULL;
SemaphoreHandle_t wifiMutex = NULL;

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

CRGB leds[N_PIXELS];
StripLed stripLed;
Led onboard_led = {ONBOARD_LED_PIN, false};
Battery batt;
DHT_Unified dht(DHTPIN, DHTTYPE);
Battery18650Stats batteryStats(ADC_PIN, CONV_FACTOR, READS, MAXV, MINV);

// ============================================================================
// GLOBAL STATE VARIABLES
// ============================================================================

float temp = 0.0f;
float hum = 0.0f;
bool bt_powerState = false;
bool lampState = false;
int readCount = 0;
int lvlCharge = 0;
char savedSSID[33] = {0};
char savedPass[65] = {0};
uint8_t myhue = 0;
CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette;

// ============================================================================
// EFECTO FIRE PARAMS
// ============================================================================

uint8_t fireCooling = 55;
uint8_t fireSparking = 50;
bool fireReverse = false;

// ============================================================================
// EFECTO SINELON PARAMS
// ============================================================================

uint8_t sinelonBeat = 23;
uint8_t sinelonFade = 2;

// ============================================================================
// EFECTO TWINKLE PARAMS
// ============================================================================

uint8_t twinkleSpeed = 8;
uint8_t twinkleIntensity = 160;
bool twinkleRedCool = true;

// ============================================================================
// EFECTO RAINBOW BEAT PARAMS
// ============================================================================

uint8_t rainbowSpeed = 30;
uint8_t rainbowDelta = 8;

// ============================================================================
// EFECTO MOVING DOT PARAMS
// ============================================================================

uint8_t movingDotSpeed = 30;
uint8_t movingDotFade = 10;

// ============================================================================
// EFECTO RIPPLE PARAMS
// ============================================================================

uint8_t rippleSize = 3;
bool rippleMirror = false;

// ============================================================================
// EFECTO COMET PARAMS
// ============================================================================

uint8_t cometSpeed = 8;
uint8_t cometTrail = 4;
bool cometBlur = false;

// ============================================================================
// EFECTO BREATH PARAMS
// ============================================================================

uint8_t breathSpeed = 128;

// ============================================================================
// EFECTO COLOR SWEEP PARAMS
// ============================================================================

uint8_t sweepSpeed = 128;

// ============================================================================
// EFECTO JUGGLE PARAMS
// ============================================================================

uint8_t juggleDots = 4;
uint8_t juggleSpeed = 64;
uint8_t juggleIntensity = 128;

// ============================================================================
// VU METER VARIABLES
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
// BALLS VARIABLES
// ============================================================================

float vImpact0 = sqrt(-2 * GRAVITY * h0);

uint8_t ballsCount = 3;
bool ballsRandomColors = false;
float h[NUM_BALLS];
float vImpact[NUM_BALLS];
float tCycle[NUM_BALLS];
int pos[NUM_BALLS];
long tLast[NUM_BALLS];
float COR[NUM_BALLS];

// ============================================================================
// POWER MANAGEMENT STATE
// ============================================================================

PowerState currentPowerState = POWER_AC_MODE;
PowerState previousPowerState = POWER_AC_MODE;
unsigned long lastStateChange = 0;
unsigned long sleepCycleStart = 0;
bool webSocketClientConnected = false;
bool onBatteryPower = false;
bool powerManagementControllingWiFi = false;
