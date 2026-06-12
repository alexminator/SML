// ──────────────────────────────────────────────────────────────────────────────
// AppState.cpp — Definiciones ÚNICAS de todo el estado global + métodos
// ──────────────────────────────────────────────────────────────────────────────
// Cada símbolo definido aquí existe una sola vez en el binario.
// Ningún otro .cpp puede definir estos mismos símbolos (fin del ODR).
// ──────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <FastLED.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
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
uint32_t stateGeneration = 0;
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

uint8_t randomMode = 0;

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

    // Exponential Moving Average filter (α=0.3) — suaviza ruido ADC para logging
    if (filteredVolts == 0.0) {
        filteredVolts = battVolts;
    } else {
        filteredVolts = 0.3 * battVolts + 0.7 * filteredVolts;
    }

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

    // Log reading to LittleFS-backed circular buffer (smart delta-based storage)
    logBatteryReading();
}

// ============================================================================
// BATTERY LOGGING — Smart delta-based storage to LittleFS
// ============================================================================
// Only saves when voltage changes by >= BATT_LOG_DELTA (0.01V). Circular buffer
// of BATT_LOG_SIZE (200) entries persists to LittleFS every BATT_LOG_SAVE_INTERVAL
// writes. Loaded on first call to make boot fast; file I/O happens lazily.
// ============================================================================

void Battery::logBatteryReading() {
    // Lazy load on first call (avoids LittleFS delay during boot setup)
    if (!logLoaded) {
        loadBatteryLog();
        logLoaded = true;
        // If loadBatteryLog found no entries, prime tracker to current reading
        if (battLogCount == 0) {
            lastLoggedVoltage = filteredVolts;
        }
        Serial.print("[BATT] log loaded, count=");
        Serial.print(battLogCount);
        Serial.print(", lastV=");
        Serial.print(lastLoggedVoltage, 3);
        Serial.print(", lastT=");
        Serial.println(lastLogUptime);
    }

    uint32_t now = millis() / 1000;

    // Smart delta: only log when voltage changed >= BATT_LOG_DELTA, or if first entry,
    // or if 30 minutes have passed since last entry (time reference for stable periods)
    double vDiff = filteredVolts - lastLoggedVoltage;
    if (vDiff < 0) vDiff = -vDiff;
    bool voltageChanged = (vDiff >= BATT_LOG_DELTA);
    bool isFirstEntry = (battLogCount == 0);
    bool timeElapsed = (battLogCount > 0) && (now - lastLogUptime >= BATT_LOG_TIMEOUT);

    if (!voltageChanged && !isFirstEntry && !timeElapsed) {
        return;  // Skip — no meaningful change
    }

    // Write entry to circular buffer
    battLog[battLogHead].uptime = now;
    battLog[battLogHead].voltage = (float)filteredVolts;
    battLog[battLogHead].level = (uint8_t)constrain(battLvl, 0, 100);

    Serial.print("[BATT] WRITE entry #");
    Serial.print(battLogCount);
    Serial.print(" V=");
    Serial.print(filteredVolts, 3);
    Serial.print(" t=");
    Serial.println(now);

    // Advance head (circular)
    battLogHead = (battLogHead + 1) % BATT_LOG_SIZE;
    if (battLogCount < BATT_LOG_SIZE) battLogCount++;

    // Update delta tracker
    lastLoggedVoltage = filteredVolts;
    lastLogUptime = now;

    // Save to LittleFS: force on first write, then every N writes
    writesSinceSave++;
    if (writesSinceSave >= BATT_LOG_SAVE_INTERVAL || battLogCount == 1) {
        writesSinceSave = 0;
        saveBatteryLog();
    }
}

void Battery::saveBatteryLog() {
    if (battLogCount == 0) return;

    // Build JSON document: {"battLog":[{"t":12345,"v":3.85,"l":65},...]}
    // Capacity: JSON_ARRAY_SIZE(N) + N*JSON_OBJECT_SIZE(3) + overhead
    size_t cap = JSON_ARRAY_SIZE(battLogCount)
               + battLogCount * JSON_OBJECT_SIZE(3)
               + 64;
    DynamicJsonDocument doc(cap);

    JsonArray arr = doc["battLog"].to<JsonArray>();
    int idx = (battLogHead + BATT_LOG_SIZE - battLogCount) % BATT_LOG_SIZE;
    for (int i = 0; i < battLogCount; i++) {
        JsonObject e = arr.add().to<JsonObject>();
        e["t"] = battLog[idx].uptime;
        e["v"] = battLog[idx].voltage;
        e["l"] = battLog[idx].level;
        idx = (idx + 1) % BATT_LOG_SIZE;
    }

    File f = LittleFS.open("/battlog.json", "w");
    if (f) {
        serializeJson(doc, f);
        f.close();
#ifdef DEBUG_BATTERY
        debugD("💾 Battery log saved: ");
        debugD_NUM(battLogCount, "%d");
        debuglnD(" entries");
#endif
    } else {
#ifdef DEBUG_BATTERY
        debuglnW("⚠ Failed to open /battlog.json for writing");
#endif
    }
}

void Battery::loadBatteryLog() {
    if (logLoaded) return;
    logLoaded = true;
    if (!LittleFS.exists("/battlog.json")) {
#ifdef DEBUG_BATTERY
        debuglnD("📂 No battery log file found — starting fresh");
#endif
        return;
    }

    File f = LittleFS.open("/battlog.json", "r");
    if (!f) return;

    // Estimate capacity: file size + 20% margin
    size_t fileSize = f.size();
    DynamicJsonDocument doc(fileSize + 256);
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
#ifdef DEBUG_BATTERY
        debugD("⚠ Battery log parse error: ");
        debuglnD(err.c_str());
#endif
        return;
    }

    JsonArray arr = doc["battLog"].as<JsonArray>();
    if (arr.isNull()) return;

    uint32_t now = millis() / 1000;
    int loaded = 0;
    for (JsonObject e : arr) {
        uint32_t t = e["t"] | 0;
        float v = e["v"] | 0.0f;
        uint8_t l = e["l"] | 0;
        if (v < 2.0f || v > 5.0f) continue;  // Sanity check

        battLog[battLogHead].uptime = t;
        battLog[battLogHead].voltage = v;
        battLog[battLogHead].level = l;
        battLogHead = (battLogHead + 1) % BATT_LOG_SIZE;
        if (battLogCount < BATT_LOG_SIZE) battLogCount++;
        loaded++;
    }

    // Reboot detection: if the newest entry's uptime is >300s ahead of current uptime,
    // the log is from a previous boot — discard to avoid incorrect wall-clock conversion.
    if (battLogCount > 0) {
        int newestIdx = (battLogHead + BATT_LOG_SIZE - 1) % BATT_LOG_SIZE;
        if (battLog[newestIdx].uptime > now + 300) {
#ifdef DEBUG_BATTERY
            debugD("🔄 Reboot detected (newest entry uptime=");
            debugD_NUM(battLog[newestIdx].uptime, "%u");
            debugD(", current uptime=");
            debugD_NUM(now, "%u");
            debuglnD(") — discarding old log");
#endif
            battLogCount = 0;
            battLogHead = 0;
            loaded = 0;
        } else {
            // Prime delta tracker to the newest entry so we don't re-log old data
            int newestIdx = (battLogHead + BATT_LOG_SIZE - 1) % BATT_LOG_SIZE;
            lastLoggedVoltage = battLog[newestIdx].voltage;
            lastLogUptime = battLog[newestIdx].uptime;
        }
    }

#ifdef DEBUG_BATTERY
    debugD("📂 Battery log loaded: ");
    debugD_NUM(loaded, "%d");
    debuglnD(" entries");
#endif
}

void Battery::getBatteryLog(BattLogEntry* outBuf, int* outCount) {
    int idx = (battLogHead + BATT_LOG_SIZE - battLogCount) % BATT_LOG_SIZE;
    int count = battLogCount;
    for (int i = 0; i < count; i++) {
        outBuf[i] = battLog[idx];
        idx = (idx + 1) % BATT_LOG_SIZE;
    }
    *outCount = count;
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
    // Clear buffer solo al CAMBIAR de efecto (evita superposición entre efectos
    // distintos). Si es el mismo efecto, NO se borra — los efectos que usan
    // fadeToBlackBy (Juggle, Sinelon, Comet, Twinkle) necesitan persistencia
    // entre frames para el trail.
    static int lastEffectId = -1;
    if (effectId != lastEffectId) {
        clear();
        lastEffectId = effectId;
    }
    runEffectById(effectId);
}
