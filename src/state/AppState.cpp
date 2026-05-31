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
// POWER MANAGEMENT STATE
// ============================================================================

PowerState currentPowerState = POWER_AC_MODE;
PowerState previousPowerState = POWER_AC_MODE;
unsigned long lastStateChange = 0;
unsigned long sleepCycleStart = 0;
bool webSocketClientConnected = false;
bool onBatteryPower = false;
bool powerManagementControllingWiFi = false;
