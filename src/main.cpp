// main.cpp
/**
 * ----------------------------------------------------------------------------
 * SMART MUSIC LAMP
 * ----------------------------------------------------------------------------
 * © 2023 Alexminator
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <Battery18650Stats.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Preferences.h>

// ── WIFI credentials — definiciones reales (data.h tiene solo extern) ──────────
const char *WIFI_SSID = DEFAULT_WIFI_SSID;
const char *WIFI_PASS = DEFAULT_WIFI_PASS;
const char *WEB_NAME = "sml";

#include "data.h"
#include "Settings.h"

// Declare the debugging level then include the header file
#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
// #define DEBUGLEVEL DEBUGLEVEL_NONE
#include "debug.h"

// ============================================================================
// DEBUG CATEGORIES - Uncomment to enable specific debug messages
// ============================================================================
// System-level messages (initialization, mutex, errors)
#define DEBUG_SYSTEM

// WiFi connection and authentication
#define DEBUG_WIFI

// Web server and LittleFS filesystem
// #define DEBUG_WEB

// WebSocket communication and JSON
// #define DEBUG_WEBSOCKET

// LED strip control and effects
// #define DEBUG_LED

// Battery monitoring and charging
// #define DEBUG_BATTERY

// Temperature and humidity sensor (DHT)
// #define DEBUG_TEMPERATURE

// Network services (mDNS)
#define DEBUG_NETWORK

// BLUETOOTH
// #define DEBUG_BLUETOOTH

// Power Management State Machine debugging
#define DEBUG_POWER_MANAGEMENT

// WEB
#define HTTP_PORT 80
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

// ----------------------------------------------------------------------------
// RTOS Mutex Protection
// ----------------------------------------------------------------------------
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
// DEFINICIÓN DE vImpact0 (SOLO UNA VEZ, AQUÍ)
// ============================================================================

float vImpact0 = sqrt(-2 * GRAVITY * h0);

// ============================================================================
// DEFINICIÓN DE MÉTODOS DEL STRUCT BATTERY
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
// DEFINICIÓN DE OBJETOS GLOBALES
// ============================================================================

StripLed stripLed;
Led onboard_led = {ONBOARD_LED_PIN, false};
Battery batt;

// ============================================================================
// DEFINICIÓN DEL STRUCT STRIPLED (MÉTODOS)
// ============================================================================

StripLed::StripLed() : R(255), G(255), B(255), brightness(130), effectId(0), powerState(false) {}  // Inicializado con el color blanco el brillo en la mitad y efecto 0

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

// ============================================================================
// EFFECTS LIBRARY INCLUDES
// ============================================================================
#include "common.h"
#include "MovingDot.h"
#include "RainbowBeat.h"
#include "RedWhiteBlue.h"
#include "Ripple.h"
#include "Fire.h"
#include "Twinkle.h"
#include "Balls.h"
#include "Juggle.h"
#include "Sinelon.h"
#include "Comet.h"
#include "Temp.h"
#include "Breath.h"
#include "ColorSweep.h"
#include "Battery.h"
// VU
#include "vu1.h"
#include "vu2.h"
#include "vu3.h"
#include "vu4.h"
#include "vu5.h"
#include "vu6.h"

void StripLed::runFire() {
    static Fire fire;
    fire.runPattern();
}

void StripLed::runMovingDot() {
    static MovingDot movingDot;
    movingDot.runPattern();
}

void StripLed::runRainbowBeat() {
    static RainbowBeat rainbowBeat;
    rainbowBeat.runPattern();
}

void StripLed::runRedWhiteBlue() {
    static RedWhiteBlue redWhiteBlue;
    redWhiteBlue.runPattern();
}

void StripLed::runRipple() {
    static Ripple ripple;
    ripple.runPattern();
}

void StripLed::runTwinkle() {
    static Twinkle twinkle;
    twinkle.runPattern();
}

void StripLed::runBalls() {
    static Balls balls;
    balls.runPattern();
}

void StripLed::runJuggle() {
    static Juggle juggle;
    juggle.runPattern();
}

void StripLed::runSinelon() {
    static Sinelon sinelon;
    sinelon.runPattern();
}

void StripLed::runComet() {
    static Comet comet;
    comet.runPattern();
}

void StripLed::runBreath() {
    static Breath breath;
    breath.runPattern();
}

void StripLed::runColorSweep() {
    static ColorSweep colorSweep;
    colorSweep.runPattern();
}

void StripLed::runRainbowVU() {
    static RainbowVU VU1;
    VU1.runPattern(is_centered, 0);
}

void StripLed::runOldVU() {
    static OldskoolVU VU2;
    VU2.runPattern(is_centered, 0);
}

void StripLed::runRainbowHueVU() {
    static RainbowHueVU VU3;
    VU3.runPattern(is_centered, 0);
}

void StripLed::runRippleVU() {
    static RippleVU VU4;
    VU4.runPattern(true);
}

void StripLed::runThreebarsVU() {
    static ThreebarsVU VU5;
    VU5.runPattern();
}

void StripLed::runOceanVU() {
    static OceanVU VU6;
    VU6.runPattern();
}

void StripLed::runTemperature() {
    static Temperature temp;
    temp.runPattern();
}

void StripLed::runBattery() {
    static Charge batt;
    batt.runPattern(lvlCharge);
}

void StripLed::update() {
    switch (effectId)
    {
    case 0:
        simpleColor(R, G, B, brightness);
        break;
    case 1:
        clear();
        runFire();
        break;
    case 2:
        clear();
        runMovingDot();
        break;
    case 3:
        clear();
        runRainbowBeat();
        break;
    case 4:
        clear();
        runRedWhiteBlue();
        break;
    case 5:
        clear();
        runRipple();
        break;
    case 6:
        clear();
        runTwinkle();
        break;
    case 7:
        clear();
        runBalls();
        break;
    case 8:
        clear();
        runJuggle();
        break;
    case 9:
        clear();
        runSinelon();
        break;
    case 10:
        clear();
        runComet();
        break;
    case 11:  // NEW - Breath effect
        clear();
        runBreath();
        break;
    case 12:  // NEW - ColorSweep effect
        clear();
        runColorSweep();
        break;
    case 13:  // Was case 12 - RainbowVU
        clear();
        runRainbowVU();
        break;
    case 14:  // Was case 13 - OldVU
        clear();
        runOldVU();
        break;
    case 15:  // Was case 14 - RainbowHueVU
        clear();
        runRainbowHueVU();
        break;
    case 16:  // Was case 15 - RippleVU
        clear();
        runRippleVU();
        break;
    case 17:  // Was case 16 - ThreebarsVU
        clear();
        runThreebarsVU();
        break;
    case 18:  // Was case 17 - OceanVU
        clear();
        runOceanVU();
        break;
    case 19:  // Was case 18 - Temperature
        clear();
        runTemperature();
        break;
    case 20:  // Was case 19 - Battery
        clear();
        runBattery();
        break;
    default:
        break;
    }
}

// ----------------------------------------------------------------------------
// DHT initialization
// ----------------------------------------------------------------------------

void readSensor()
{
    sensors_event_t event;
    int retryCount = 0;
    const int maxRetries = 3; // Maximum number of retries
    const unsigned long retryDelay = 2000; // Delay between retries (2 seconds)
    while (retryCount < maxRetries)
    {
        dht.temperature().getEvent(&event);
        if (!isnan(event.temperature))
        {
            temp = event.temperature;
#ifdef DEBUG_TEMPERATURE
            debugD("Temperature: ");
            debugD_FLOAT1(temp);
            debugD("°C\n");
#endif
            break; // Exit loop on success
        }
        else
        {
#ifdef DEBUG_TEMPERATURE
            debuglnD("Error reading temperature! Retrying...");
#endif
            retryCount++;
            vTaskDelay(pdMS_TO_TICKS(retryDelay)); // Wait before retrying
        }
    }
    retryCount = 0; // Reset retry count for humidity reading
    while (retryCount < maxRetries)
    {
        dht.humidity().getEvent(&event);
        if (!isnan(event.relative_humidity))
        {
            hum = event.relative_humidity;
#ifdef DEBUG_TEMPERATURE
            debugD("Humidity: ");
            debugD_FLOAT1(hum);
            debugD("%\n");
#endif
            break; // Exit loop on success
        }
        else
        {
#ifdef DEBUG_TEMPERATURE
            debuglnD("Error reading humidity! Retrying...");
#endif
            retryCount++;
            vTaskDelay(pdMS_TO_TICKS(retryDelay)); // Wait before retrying
        }
    }
}

// ----------------------------------------------------------------------------
// SPIFFS initialization
// ----------------------------------------------------------------------------

void initLittleFS()
{
    if (!LittleFS.begin())
    {
#ifdef DEBUG_WEB
        debuglnD("Cannot mount LittleFS volume...");
#endif

        // Timeout instead of infinite loop
        unsigned long errorStartTime = millis();
        const unsigned long MAX_ERROR_TIME = LITTLEFS_TIMEOUT; // 30 seconds

        // Overflow-safe elapsed time calculation
        unsigned long elapsed;
        do
        {
            elapsed = millis() - errorStartTime;
            // Overflow-safe LED flash timing
            uint32_t flashOnTime = millis() % LED_ERROR_FLASH_CYCLE;
            onboard_led.on = flashOnTime < LED_ERROR_FLASH_ON;
            onboard_led.update();
            vTaskDelay(pdMS_TO_TICKS(100));
        } while (elapsed < MAX_ERROR_TIME);

        // Continue with setup
#ifdef DEBUG_WEB
        debuglnE("LittleFS mount failed - web interface unavailable");
        debuglnW("Device will continue with limited functionality");
#endif
    }
}

// ----------------------------------------------------------------------------
// Connecting to the WiFi network
// ----------------------------------------------------------------------------

void initWiFi()
{
    WiFi.mode(WIFI_STA);

    // Try to get saved credentials from Preferences
    Preferences preferences;
    preferences.begin("wifi", false);

    // Read WiFi credentials (putBytes format - no String objects)
    size_t ssidLen = preferences.getBytes("ssid", savedSSID, sizeof(savedSSID) - 1);
    savedSSID[ssidLen] = '\0';  // Ensure null termination

    size_t passLen = preferences.getBytes("password", savedPass, sizeof(savedPass) - 1);
    savedPass[passLen] = '\0';  // Ensure null termination

    preferences.end();

    // Try to connect with saved credentials first, then defaults
    bool connected = false;
    int attempts = 0;

    // Try with saved credentials first
    if (strlen(savedSSID) > 0 && strlen(savedPass) > 0) {
#ifdef DEBUG_WIFI
        debuglnD("Trying SAVED credentials...");
        debugD("SSID: ");
        debugD(savedSSID);
        debugD("\n");
#endif
        WiFi.begin(savedSSID, savedPass);

        while (WiFi.status() != WL_CONNECTED && attempts < WIFI_MAX_ATTEMPTS)
        {
#ifdef DEBUG_WIFI
            debugD(".");
#endif
            vTaskDelay(pdMS_TO_TICKS(WIFI_RETRY_DELAY));
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
#ifdef DEBUG_WIFI
            debuglnD("Using saved credentials from Preferences");
#endif
        } else {
#ifdef DEBUG_WIFI
            debugD("\n");
            debuglnD("Failed with saved credentials!");
#endif
        }
    }

    // If saved credentials failed, try with defaults
    if (!connected) {
#ifdef DEBUG_WIFI
        debugD("\n");
        debuglnD("Trying DEFAULT credentials...");
        debugD("SSID: ");
        debugD(WIFI_SSID);
        debugD("\n");
#endif
        WiFi.begin(WIFI_SSID, WIFI_PASS);

        attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < WIFI_MAX_ATTEMPTS)
        {
#ifdef DEBUG_WIFI
            debugD(".");
#endif
            vTaskDelay(pdMS_TO_TICKS(WIFI_RETRY_DELAY));
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
#ifdef DEBUG_WIFI
            debugD("\n");
            debuglnD("Connected with DEFAULT credentials");
            debuglnD("Using default credentials from build configuration");
#endif
        } else {
#ifdef DEBUG_WIFI
            debugD("\n");
            debuglnD("Failed with DEFAULT credentials too!");
#endif
        }
    }

    // If all connection attempts failed
    if (!connected)
    {
#ifdef DEBUG_WIFI
        debuglnD("\n\n*** WiFi CONNECTION FAILED ***");
        debuglnD("Please check:");
        debuglnD("1. WiFi router is powered on");
        debuglnD("2. SSID and password are correct");
        debuglnD("3. ESP32 is within WiFi range");
        debuglnD("Restarting in 5 seconds...\n");
#endif

        // Blink LED to indicate error (non-blocking)
        pinMode(STRIP_PIN, OUTPUT);
        unsigned long blinkStart = millis();
        int blinkCount = 0;
        while (blinkCount < 10 && millis() - blinkStart < 4000)
        {
            if (millis() - blinkStart >= (blinkCount * 400))
            {
                digitalWrite(STRIP_PIN, (blinkCount % 2 == 0) ? HIGH : LOW);
                blinkCount++;
            }
            vTaskDelay(pdMS_TO_TICKS(10));  // Small delay to yield CPU
        }
        digitalWrite(STRIP_PIN, LOW);  // Ensure LED is off

        // Non-blocking delay before restart
        unsigned long restartStart = millis();
        while (millis() - restartStart < 5000)
        {
            vTaskDelay(pdMS_TO_TICKS(100));  // Yield to FreeRTOS
        }

        ESP.restart();
    }

    // Connected successfully
#ifdef DEBUG_WIFI
    debugD("\nConnected! IP: ");
    debugD(WiFi.localIP().toString().c_str());
    debugD("\n");
    debugD("Abre http://");
    debugD(WEB_NAME);
    debugD(".local o http://");
    debugD(WiFi.localIP().toString().c_str());
    debugD("\n");
#endif

    if (!MDNS.begin(WEB_NAME))
    {
#ifdef DEBUG_NETWORK
        debuglnE("Error configurando mDNS — el nombre sml.local no estará disponible");
        debuglnE("Usa la IP directamente para acceder al dispositivo");
#endif
        // Continuar sin mDNS — no bloquear el dispositivo
    }
#ifdef DEBUG_NETWORK
    else {
        debuglnD("mDNS configurado");
    }
#endif
}

// ----------------------------------------------------------------------------
// Web server initialization
// ----------------------------------------------------------------------------

enum Status
{
    COLOR,
    FIRE_STATE,
    MOVINGDOT_STATE,
    RAINBOWBEAT_STATE,
    RWB_STATE,
    RIPPLE_STATE,
    TWINKLE_STATE,
    BALLS_STATE,
    JUGGLE_STATE,
    SINELON_STATE,
    COMET_STATE,
    BREATH_STATE,
    COLORSWEEP_STATE,
    BRIGHTNESS,
    STRIPLED,
    BLUETOOTH,
    VU1,
    VU2,
    VU3,
    VU4,
    VU5,
    VU6,
    LAMP,
    TEMPNEO,
    BATTNEO
} status;

const char* processor(const String &var)
{
    static char buffer[64];
    switch (status)
    {
    case COLOR:
    {
        JsonDocument doc;  // Modern API - auto sizing
        doc["color"]["r"] = stripLed.R;
        doc["color"]["g"] = stripLed.G;
        doc["color"]["b"] = stripLed.B;
        serializeJson(doc, buffer, sizeof(buffer));
        return buffer;
    }
    case BRIGHTNESS:
        itoa(stripLed.brightness, buffer, 10);
        return buffer;
    case STRIPLED:
        return stripLed.powerState ? "on" : "off";
    case BLUETOOTH:
        return bt_powerState ? "on" : "off";
    case FIRE_STATE:
    case MOVINGDOT_STATE:
    case RAINBOWBEAT_STATE:
    case RWB_STATE:
    case RIPPLE_STATE:
    case TWINKLE_STATE:
    case BALLS_STATE:
    case JUGGLE_STATE:
    case SINELON_STATE:
    case COMET_STATE:
    case BREATH_STATE:
    case COLORSWEEP_STATE:
    case VU1:
    case VU2:
    case VU3:
    case VU4:
    case VU5:
    case VU6:
    case TEMPNEO:
    case BATTNEO:
    case LAMP:
        return "off";
    default:
        buffer[0] = '\0';
        return buffer;
    }
}

void onRootRequest(AsyncWebServerRequest *request)
{
    request->send(LittleFS, "/index.html", "text/html", false, processor);
}

// Initialize webserver URLs
void initWebServer()
{
    server.on("/", onRootRequest);
    server.on("/wifi-info", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      JsonDocument json;  // Modern API
      json["status"] = "ok";
      json["ssid"] = WiFi.SSID();
      json["ip"] = WiFi.localIP().toString();
      json["rssi"] = WiFi.RSSI();
      serializeJson(json, *response);
      request->send(response);
    });

    // Endpoint to save WiFi configuration and restart
    server.on("/save-wifi", HTTP_POST, [](AsyncWebServerRequest *request)
    {
      // Direct char array copy - no String objects to prevent heap fragmentation
      char newSSID[33] = {0};  // SSID max 32 chars + null
      char newPassword[65] = {0};  // Password max 64 chars + null

      // Get parameters directly without String objects
      const AsyncWebParameter* pSSID = request->getParam("ssid", true);
      const AsyncWebParameter* pPass = request->getParam("password", true);

      // Validate parameters exist
      if (!pSSID || !pPass) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing parameters\"}");
        return;
      }

      // Get string lengths
      size_t ssidLen = pSSID->value().length();
      size_t passLen = pPass->value().length();

      // Validate lengths
      if (ssidLen > 32 || passLen > 64) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"SSID or password too long\"}");
        return;
      }

      // Copy directly to char arrays (no String objects!)
      strncpy(newSSID, pSSID->value().c_str(), sizeof(newSSID) - 1);
      newSSID[sizeof(newSSID) - 1] = '\0';  // Ensure null termination

      strncpy(newPassword, pPass->value().c_str(), sizeof(newPassword) - 1);
      newPassword[sizeof(newPassword) - 1] = '\0';  // Ensure null termination

      // If SSID is empty, use current SSID
      if (strlen(newSSID) == 0) {
        strncpy(newSSID, WiFi.SSID().c_str(), sizeof(newSSID) - 1);
        newSSID[sizeof(newSSID) - 1] = '\0';  // Ensure null termination
      }

#ifdef DEBUG_WIFI
      debuglnD("Saving WiFi credentials...");
#endif

      // Protect WiFi credential operations with mutex
      if (xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
          // Save to Preferences
          Preferences preferences;
          preferences.begin("wifi", false);
          preferences.putBytes("ssid", newSSID, strlen(newSSID));
          preferences.putBytes("password", newPassword, strlen(newPassword));
          preferences.end();

#ifdef DEBUG_WIFI
          debuglnD("Saved. Restarting.");
          size_t heapAfter = ESP.getFreeHeap();
#endif
          xSemaphoreGive(wifiMutex);

          // Send quick response
          request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Saved\"}");

          // Restart immediately
          ESP.restart();
      } else {
#ifdef DEBUG_WIFI
          debuglnE("Failed to acquire WiFi mutex for saving credentials");
#endif
          request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Mutex busy\"}");
      }
    });

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html").setTryGzipFirst(false);
    server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(400, "text/plain", "Not found"); });
    ElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
#ifdef DEBUG_WEB
    debuglnD("HTTP server started");
#endif
    MDNS.addService("http", "tcp", 80);
}

// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------

const char* bars()
{
    int signal = WiFi.RSSI();
    if (signal >= -63 && signal <= -1)
        return "waveStrength-4";
    else if (signal >= -73 && signal <= -64)
        return "waveStrength-3";
    else if (signal >= -83 && signal <= -74)
        return "waveStrength-2";
    else if (signal >= -93 && signal <= -84)
        return "waveStrength-1";
    else
        return "no-signal";
}

void notifyClients()
{
    // Take mutex for reading shared data
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        JsonDocument json;  // Modern API - auto sizing

        // Usar valores numéricos directamente en JSON
        json["bars"] = bars();
        json["battVoltage"] = batt.battVolts;
        json["level"] = batt.battLvl;
        json["charging"] = batt.chargeState;
        json["fullbatt"] = batt.fullBatt;
        json["temperature"] = temp;
        json["humidity"] = hum;
        json["lampstatus"] = lampState ? "on" : "off";
        json["neostatus"] = stripLed.powerState ? "on" : "off";
        json["btstatus"] = bt_powerState ? "on" : "off";
        json["neobrightness"] = stripLed.brightness;

        // WiFi information
        json["ssid"] = WiFi.SSID();
        json["ip"] = WiFi.localIP().toString();
        json["rssi"] = WiFi.RSSI();

        // Añade el color actual (modern API)
        JsonObject color = json["color"].to<JsonObject>();
        color["r"] = stripLed.R;
        color["g"] = stripLed.G;
        color["b"] = stripLed.B;

        // Efectos y VU
        const char *effectNames[] = {
            "fireStatus", "movingdotStatus", "rainbowbeatStatus", "rwbStatus", "rippleStatus",
            "twinkleStatus", "ballsStatus", "juggleStatus", "sinelonStatus", "cometStatus", "breathStatus",
            "colorSweepStatus", "rainbowVUStatus", "oldVUStatus", "rainbowHueVUStatus", "rippleVUStatus",
            "threebarsVUStatus", "oceanVUStatus", "tempNEOStatus", "battNEOStatus"};
        for (uint8_t i = 0; i < 20; ++i)
            json[effectNames[i]] = (stripLed.effectId == i + 1 && stripLed.powerState) ? "on" : "off";

        // Quick sanity check — reject unusually large payloads
        if (measureJson(json) + 128 > 1024) {
#ifdef DEBUG_WEBSOCKET
            debuglnE("JSON payload too large for WebSocket");
#endif
            xSemaphoreGive(dataMutex);
            return;
        }

        // Fixed-size buffer (no VLA — safer on ESP32 stack)
        char buffer[1024];
        size_t len = serializeJson(json, buffer, sizeof(buffer));

        if (len >= sizeof(buffer)) {
#ifdef DEBUG_WEBSOCKET
            debuglnE("JSON serialization failed - buffer too small");
#endif
            xSemaphoreGive(dataMutex);
            return;
        }

#ifdef DEBUG_WEBSOCKET
        debugD("WebSocket payload size: ");
        debuglnD_NUM(len, "%u");
        debuglnD(" bytes");
#endif

        ws.textAll(buffer, len);

        xSemaphoreGive(dataMutex);
    } else {
#ifdef DEBUG_WEBSOCKET
        debuglnW("Failed to acquire mutex for WebSocket update");
#endif
    }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
        JsonDocument json;  // Modern API - auto sizing
        DeserializationError err = deserializeJson(json, data);
        if (err)
        {
#ifdef DEBUG_WEBSOCKET
            debuglnD("deserializeJson() failed with code ");
            debuglnD(err.c_str());
#endif
            return;
        }

        const char *action = json["action"];
        const int effectId = json["effectId"];
        stripLed.effectId = effectId;

        if (strcmp(action, "toggle") == 0)
        {
            stripLed.powerState = !stripLed.powerState;
            stripLed.powerState ? stripLed.update() : stripLed.clear();
        }
        else if (strcmp(action, "lamp") == 0)
        {
            lampState = !lampState;
            digitalWrite(LAMP_PIN, lampState ? LOW : HIGH);
#ifdef DEBUG_LED
            debuglnD(lampState ? "Lampara ON" : "Lampara OFF");
#endif
        }
        else if (strcmp(action, "animation") == 0 || strcmp(action, "vu") == 0 || strcmp(action, "indicator") == 0)
        {
            if (stripLed.powerState)
            {
                stripLed.update();
            }
        }
        else if (strcmp(action, "slider") == 0)
        {
            const int brightness = json["brightness"].as<int>();
#ifdef DEBUG_LED
            debugD("Brillo ");
            debuglnD_NUM(brightness, "%d");
#endif
            stripLed.brightness = brightness;
        }
        else if (strcmp(action, "picker") == 0)
        {
            JsonObject color = json["color"];
            stripLed.R = color["r"].as<int>();
            stripLed.G = color["g"].as<int>();
            stripLed.B = color["b"].as<int>();
#ifdef DEBUG_LED
            debugD("RGB: ");
            debugD_NUM(stripLed.R, "%d");
            debugD(", ");
            debugD_NUM(stripLed.G, "%d");
            debugD(", ");
            debuglnD_NUM(stripLed.B, "%d");
#endif
        }
        else if (strcmp(action, "music") == 0)
        {
            bt_powerState = !bt_powerState;
            digitalWrite(SWITCH_PIN, bt_powerState ? LOW : HIGH);
#ifdef DEBUG_BLUETOOTH
            if (bt_powerState) {
                debuglnD("Encendido del modulo BT");
            } else {
                debuglnD("Apagado del modulo BT");
            }
#endif
        }
        else if (strcmp(action, "volup") == 0)
        {
            // Simulate a button press
            digitalWrite(VOLUMENUP_PIN, HIGH); // Activate Mosfet, push a button
            vTaskDelay(pdMS_TO_TICKS(long_delay));
            digitalWrite(VOLUMENUP_PIN, LOW); // Deactivate Mosfet, release button
        }
        else if (strcmp(action, "voldown") == 0)
        {
            digitalWrite(VOLUMENDOWN_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(long_delay));
            digitalWrite(VOLUMENDOWN_PIN, LOW);
        }
        else if (strcmp(action, "skipL") == 0)
        {
            digitalWrite(VOLUMENDOWN_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(short_delay));
            digitalWrite(VOLUMENDOWN_PIN, LOW);
        }
        else if (strcmp(action, "skipR") == 0)
        {
            digitalWrite(VOLUMENUP_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(short_delay));
            digitalWrite(VOLUMENUP_PIN, LOW);
        }
        else if (strcmp(action, "play-pause") == 0)
        {
            digitalWrite(PLAY_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(short_delay));
            digitalWrite(PLAY_PIN, LOW);
        }
        notifyClients();
    }
}

//-----------------------------------------------------------------------------
// Power Management Function Declarations
//-----------------------------------------------------------------------------

void transitionToState(PowerState newState);
void applyStateConfiguration(PowerState state);
void onPowerSourceChanged(bool nowOnBattery);
void onCriticalBatteryLevel();
void handleBatteryConnectingState();
void handleBatterySleepState();
void updatePowerStateMachine();
void checkWebSocketClients();

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
#ifdef DEBUG_WEBSOCKET
        debugD("WebSocket client #");
        debugD_NUM(client->id(), "%u");
        debugD(" connected from ");
        debugD(client->remoteIP().toString().c_str());
        debugD("\n");
#endif
        webSocketClientConnected = true;
#ifdef DEBUG_POWER_MANAGEMENT
        debuglnD("✅ WebSocket client connected");
#endif

        // If in battery sleep or connecting, wake up immediately
        if (currentPowerState == POWER_BATTERY_SLEEP ||
            currentPowerState == POWER_BATTERY_CONNECTING) {
#ifdef DEBUG_POWER_MANAGEMENT
            debuglnD("🔔 Waking for WebSocket client");
#endif
            transitionToState(POWER_BATTERY_ACTIVE);
        }
        break;
    case WS_EVT_DISCONNECT:
#ifdef DEBUG_WEBSOCKET
        debugD("WebSocket client #");
        debugD_NUM(client->id(), "%u");
        debugD(" disconnected\n");
#endif
        webSocketClientConnected = false;
#ifdef DEBUG_POWER_MANAGEMENT
        debuglnD("❌ WebSocket client disconnected");
#endif

        // If in battery active mode, go back to connecting (wait 30s)
        if (currentPowerState == POWER_BATTERY_ACTIVE) {
#ifdef DEBUG_POWER_MANAGEMENT
            debuglnD("💤 Client disconnected - waiting 30s for reconnect");
#endif
            transitionToState(POWER_BATTERY_CONNECTING);
        }
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
#ifdef DEBUG_WEBSOCKET
        debugD("ws[");
        debugD(server->url());
        debugD("][");
        debugD_NUM(client->id(), "%u");
        debugD("] pong[");
        debugD_NUM(len, "%u");
        debugD("]: ");
        debugD((len) ? (char *)data : "");
        debugD("\n");
#endif
        break;
    case WS_EVT_ERROR:
#ifdef DEBUG_WEBSOCKET
        debugD("ws[");
        debugD(server->url());
        debugD("][");
        debugD_NUM(client->id(), "%u");
        debugD("] error(");
        debugD_NUM(*((uint16_t *)arg), "%u");
        debugD("): ");
        debugD((char *)data);
        debugD("\n");
#endif
        break;
    }
}

void initWebSocket()
{
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
#ifdef DEBUG_WEBSOCKET
    debuglnD("WebSocket server started");
#endif
}

// ----------------------------------------------------------------------------
// RTOS Tasks
// ----------------------------------------------------------------------------

TaskHandle_t TaskWebSocketHandle;
TaskHandle_t TaskBatteryMonitorHandle;
TaskHandle_t TaskLEDControlHandle;
TaskHandle_t TaskWiFiMonitorHandle;
TaskHandle_t TaskSensorHandle;
TaskHandle_t TaskOnboardLEDHandle;

void TaskWebSocket(void *pvParameters)
{
    UBaseType_t stackHighWaterMark;

    while (true)
    {
        ws.cleanupClients();
        notifyClients();

        // Monitor stack every 10 cycles
        static uint8_t cycleCount = 0;
        if (++cycleCount >= WEBSOCKET_STACK_CHECK_CYCLES) {
            stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
            if (stackHighWaterMark < STACK_WARNING_THRESHOLD) {
#ifdef DEBUG_WEBSOCKET
                debuglnW("WebSocket task stack running low!");
                debugD("Stack free: ");
                debugD_NUM(stackHighWaterMark, "%u");
                debuglnD(" bytes");
#endif
            }
            cycleCount = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(WEBSOCKET_UPDATE_INTERVAL));
    }
}

void TaskBatteryMonitor(void *pvParameters)
{
    while (true)
    {
        batt.battMonitor();

        // Protect shared variable access
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            lvlCharge = batt.battLvl;
            xSemaphoreGive(dataMutex);
        } else {
#ifdef DEBUG_BATTERY
            debuglnW("Failed to acquire data mutex in BatteryMonitor");
#endif
        }

        // Power source detection (Task 4)
        bool currentlyOnBattery = (!batt.fullBatt && !batt.chargeState);

        if (currentlyOnBattery != onBatteryPower) {
            onPowerSourceChanged(currentlyOnBattery);
        }

        // Check for critical battery level
        if (onBatteryPower && batt.battLvl < BATTERY_CRITICAL_LEVEL) {
            onCriticalBatteryLevel();
        }

        // WebSocket client tracking (Task 11)
        checkWebSocketClients();

        // Power management state machine (Task 12)
        updatePowerStateMachine();

        vTaskDelay(pdMS_TO_TICKS(BATTERY_CHECK_INTERVAL));
    }
}

void TaskLEDControl(void *pvParameters)
{
    while (true)
    {
        if (stripLed.powerState)
        {
            stripLed.update();
        }
        else
        {
            stripLed.clear();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void TaskWiFiMonitor(void *pvParameters)
{
    static int disconnectCount = 0;
    const int MAX_DISCONNECTS = 5;

    while (true)
    {
        // Protect WiFi status check
        if (xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Only count disconnects if power management is NOT controlling WiFi
            if (!powerManagementControllingWiFi && WiFi.status() != WL_CONNECTED) {
                disconnectCount++;
#ifdef DEBUG_WIFI
                debugD("WiFi disconnect count: ");
                debuglnD_NUM(disconnectCount, "%d");
#endif

                if (disconnectCount >= MAX_DISCONNECTS) {
#ifdef DEBUG_WIFI
                    debuglnE("Max WiFi disconnects reached, restarting...");
#endif
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    ESP.restart();
                }
            } else {
#ifdef DEBUG_WIFI
                if (disconnectCount > 0) {
                    debugD("WiFi reconnected after ");
                    debugD_NUM(disconnectCount, "%d");
                    debuglnD(" disconnects");
                }
#endif
                disconnectCount = 0;
            }

            // Debug: Show when power management is controlling WiFi
#ifdef DEBUG_POWER_MANAGEMENT
            if (powerManagementControllingWiFi) {
                static unsigned long lastDebugMsg = 0;
                if (millis() - lastDebugMsg > 10000) {  // Every 10 seconds
                    debuglnD("🔋 Power Management controlling WiFi - TaskWiFiMonitor bypassed");
                    lastDebugMsg = millis();
                }
            }
#endif
            xSemaphoreGive(wifiMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(WIFI_MONITOR_INTERVAL));
    }
}

void TaskSensor(void *pvParameters)
{
    while (true)
    {
        readSensor();
        vTaskDelay(pdMS_TO_TICKS(SENSOR_CHECK_INTERVAL));
    }
}

void TaskOnboardLED(void *pvParameters)
{
    while (true)
    {
        // LED behavior based on connection status (not power management)
        // Pattern meanings:
        // - WiFi + WebSocket: Mixed short+long blink (200ms+800ms)
        // - WiFi only: Regular 1s blink
        // - Not connected: OFF
        bool wifiConnected = (WiFi.status() == WL_CONNECTED);

        if (wifiConnected && webSocketClientConnected) {
            // WiFi + WebSocket connected: Mixed pattern (short 200ms + long 800ms)
            // Cycle: ON(200ms) -> OFF(300ms) -> ON(500ms) -> OFF(1000ms)
            uint32_t cycleTime = millis() % 2000;
            if (cycleTime < 200) {
                onboard_led.on = true;   // Short blink
            } else if (cycleTime < 500) {
                onboard_led.on = false;  // Short pause
            } else if (cycleTime < 1000) {
                onboard_led.on = true;   // Long blink
            } else {
                onboard_led.on = false;  // Long pause (remainder of 2s cycle)
            }
        } else if (wifiConnected) {
            // WiFi only (no WebSocket): Regular 1s blink
            onboard_led.on = millis() % 1000 < 500;
        } else {
            // Not connected: OFF
            onboard_led.on = false;
        }

        onboard_led.update();
        vTaskDelay(pdMS_TO_TICKS(100)); // Update every 100 ms
    }
}

//-----------------------------------------------------------------------------
// Power Management State Machine Functions
//-----------------------------------------------------------------------------

// Forward declarations
void applyStateConfiguration(PowerState state);

// Transition to new power state with logging
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

// Apply hardware configuration for each state
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

//-----------------------------------------------------------------------------
// Power Management Event Handlers
//-----------------------------------------------------------------------------

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

// Main state machine update function - call from loop()
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

// Periodic verification of WebSocket client connections
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

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

void setup()
{
    delay(3000); // sanity delay
    pinMode(onboard_led.pin, OUTPUT);
    pinMode(STRIP_PIN, OUTPUT);
    pinMode(LAMP_PIN, OUTPUT);
    pinMode(SWITCH_PIN, OUTPUT);
    pinMode(CHARGE_PIN, INPUT);
    pinMode(ADC_PIN, INPUT);
    pinMode(FULL_CHARGE_PIN, INPUT);
    pinMode(VOLUMENUP_PIN, OUTPUT);
    pinMode(VOLUMENDOWN_PIN, OUTPUT);
    pinMode(PLAY_PIN, OUTPUT);

    // Init Rele on OFF
    digitalWrite(LAMP_PIN, HIGH);
    digitalWrite(SWITCH_PIN, HIGH);

    // Init Mosfet on OFF. Emulate Button not pressed
    digitalWrite(VOLUMENUP_PIN, LOW);
    digitalWrite(VOLUMENDOWN_PIN, LOW);
    digitalWrite(PLAY_PIN, LOW);

    Serial.begin(115200);
    initMutexes();

    // Initialize DHT22 device.
    dht.begin();
    // Print temperature sensor details.
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
#ifdef DEBUG_TEMPERATURE
    debuglnD("Temperature Sensor");
    debugD("Sensor Type: ");
    debuglnD(sensor.name);
    debugD("Driver Ver: ");
    debugD_NUM(sensor.version, "%d");
    debugD("\n");
    debugD("Unique ID: ");
    debugD_NUM(sensor.sensor_id, "%u");
    debugD("\n");
    debugD("Max Value: ");
    debugD_FLOAT1(sensor.max_value);
    debugD("°C\n");
    debugD("Min Value: ");
    debugD_FLOAT1(sensor.min_value);
    debugD("°C\n");
    debugD("Resolution: ");
    debugD_FLOAT1(sensor.resolution);
    debugD("°C\n");
    debuglnD("----------------------------------------------------");
#endif
    // Print humidity sensor details.
    dht.humidity().getSensor(&sensor);
#ifdef DEBUG_TEMPERATURE
    debuglnD("Humidity Sensor");
    debugD("Sensor Type: ");
    debuglnD(sensor.name);
    debugD("Driver Ver: ");
    debugD_NUM(sensor.version, "%d");
    debugD("\n");
    debugD("Unique ID: ");
    debugD_NUM(sensor.sensor_id, "%u");
    debugD("\n");
    debugD("Max Value: ");
    debugD_FLOAT1(sensor.max_value);
    debugD("%\n");
    debugD("Min Value: ");
    debugD_FLOAT1(sensor.min_value);
    debugD("%\n");
    debugD("Resolution: ");
    debugD_FLOAT1(sensor.resolution);
    debugD("%\n");
    debuglnD("----------------------------------------------------");
#endif
    
    // For FASTLED library
    FastLED.addLeds<LED_TYPE, STRIP_PIN, COLOR_ORDER>(leds, N_PIXELS).setCorrection(TypicalLEDStrip);
    FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_MILLIAMPS);
    FastLED.setBrightness(stripLed.brightness);
    // Clear all neo's
    FastLED.clear();
    FastLED.show();

    // Initialize variables for balls effect
    for (int i = 0; i < NUM_BALLS; i++)
    {
        tLast[i] = millis();
        h[i] = h0;
        pos[i] = 0;            // Balls start on the ground
        vImpact[i] = vImpact0; // And "pop" up at vImpact0
        tCycle[i] = 0;
        COR[i] = 0.90 - float(i) / pow(NUM_BALLS, 2);
    }

    initLittleFS();
    initWiFi();
    initWebSocket();
    initWebServer();

    xTaskCreatePinnedToCore(TaskWebSocket, "WebSocketTask", 4096, NULL, 1, &TaskWebSocketHandle, 0);
    xTaskCreatePinnedToCore(TaskBatteryMonitor, "BatteryMonitorTask", 2048, NULL, 1, &TaskBatteryMonitorHandle, 1);
    xTaskCreatePinnedToCore(TaskLEDControl, "LEDControlTask", 2048, NULL, 1, &TaskLEDControlHandle, 0);
    xTaskCreatePinnedToCore(TaskWiFiMonitor, "WiFiMonitorTask", 2048, NULL, 1, &TaskWiFiMonitorHandle, 1);
    xTaskCreatePinnedToCore(TaskSensor, "SensorTask", 2048, NULL, 1, &TaskSensorHandle, 0);
    xTaskCreatePinnedToCore(TaskOnboardLED, "LEDOnboardTask", 2048, NULL, 1, &TaskOnboardLEDHandle, 1);
}

// ----------------------------------------------------------------------------
// Main control loop
// ----------------------------------------------------------------------------

void loop()
{

}