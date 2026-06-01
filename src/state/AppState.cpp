// ──────────────────────────────────────────────────────────────────────────────
// AppState.cpp — Definiciones ÚNICAS de todo el estado global + métodos
// ──────────────────────────────────────────────────────────────────────────────
// Cada símbolo definido aquí existe una sola vez en el binario.
// Ningún otro .cpp puede definir estos mismos símbolos (fin del ODR).
// ──────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <FastLED.h>
#include "AppState.h"
#include "../effects/EffectRegistry.h"
#include "config/debug_config.h"

// ============================================================================
// RTOS SYNCHRONIZATION
// ============================================================================

SemaphoreHandle_t dataMutex = NULL;
SemaphoreHandle_t wifiMutex = NULL;

void initMutexes() {
    dataMutex = xSemaphoreCreateMutex();
    wifiMutex = xSemaphoreCreateMutex();

    if (dataMutex == NULL || wifiMutex == NULL) {
#ifdef DEBUG_SYSTEM
        debuglnE("Failed to create mutexes!");
        debuglnE("System may experience race conditions");
#endif
    } else {
#ifdef DEBUG_SYSTEM
        debuglnD("Mutexes initialized successfully");
#endif
    }
}

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
// VU METER VARIABLES (is_centered shared across vu1/vu2/vu3)
// ============================================================================

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

// ============================================================================
// BATTERY METHODS
// ============================================================================

void Battery::battMonitor() {
    int isCharging = digitalRead(CHARGE_PIN);
    chargeState = isCharging == LOW;
    int fullyCharge = digitalRead(FULL_CHARGE_PIN);
    fullBatt = fullyCharge == LOW;
    battVolts = batteryStats.getBatteryVolts();
    battLvl = batteryStats.getBatteryChargeLevel(true);

#ifdef DEBUG_BATTERY
    debugD(chargeState ? "Cargador conectado" : "Cargador desconectado");
    debugD("\n");
    debugD("Estado del pin carga: ");
    debugD(fullyCharge ? "LOW (charging)" : "HIGH (full/not charging)");
    debugD("\n");
    if (!fullBatt && !chargeState) {
        debuglnD("Batería usándose");
    } else if (fullBatt) {
        debuglnD("Batería completamente cargada");
    } else {
        debuglnD("Batería cargándose");
    }
    debugD("Lectura promedio: ");
    debugD_NUM(batteryStats.pinRead(), "%d");
    debugD(", Voltaje: ");
    debugD_NUM((int)(battVolts * 1000), "%d");
    debugD(".");
    debugD_NUM03((int)((battVolts * 1000) % 1000));
    debugD(", Nivel: ");
    debugD_NUM(battLvl, "%d");
    debuglnD("%");
#endif
}

// ============================================================================
// STRIPLED METHODS
// ============================================================================
// Init white color, center brightness and effect 0

StripLed::StripLed() : R(255), G(255), B(255), brightness(130), effectId(0), powerState(false) {}

void StripLed::simpleColor(int ar, int ag, int ab, int brightness) {
    for (int i = 0; i < N_PIXELS; i++) {
        leds[i] = CRGB(ar, ag, ab);
    }
    FastLED.setBrightness(brightness);
    FastLED.show();
}

void StripLed::clear() {
    FastLED.clear();
    FastLED.show();
}

void StripLed::update() {
    if (!powerState) {
        clear();
        return;
    }
    if (effectId == 0) {
        simpleColor(R, G, B, brightness);
        return;
    }
    clear();
    runEffectById(effectId);
}
