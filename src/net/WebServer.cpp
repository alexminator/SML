// ──────────────────────────────────────────────────────────────────────────────
// WebServer.cpp — Web server + WiFi + LittleFS
// ──────────────────────────────────────────────────────────────────────────────
#include "WebServer.h"
#include "state/AppState.h"
#include "data.h"
#include "effects/EffectRegistry.h"
#include "effects/PaletteManager.h"

#include "config/debug_config.h"

#include <WiFi.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <ElegantOTA.h>

// ============================================================================
// CONSTANTES
// ============================================================================

#define HTTP_PORT 80

// ============================================================================
// OBJETOS GLOBALES
// ============================================================================

AsyncWebServer server(HTTP_PORT);
Status status = COLOR;

// ============================================================================
// LittleFS initialization
// ============================================================================

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

// ============================================================================
// Connecting to the WiFi network
// ============================================================================

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

    // If all connection attempts failed — let the power management
    // state machine handle retries (no more ESP.restart()).
    if (!connected)
    {
#ifdef DEBUG_WIFI
        debuglnD("\n\n*** WiFi CONNECTION FAILED ***");
        debuglnD("Power management will retry in the background");
#endif
        return;  // Nothing else to do — mDNS etc. require WiFi
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

    initMDNS();
}

// ============================================================================
// Template processor (for server-side HTML variable replacement)
// ============================================================================

const char* processor(const String &var)
{
    static char buffer[64];
    switch (status)
    {
    case COLOR:
    {
        StaticJsonDocument<128> doc;
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
    case LAMP:
        return lampState ? "on" : "off";
    case TEMPNEO:
    case BATTNEO:
    default:
        buffer[0] = '\0';
        return buffer;
    }
}

// ============================================================================
// Root request handler
// ============================================================================

void onRootRequest(AsyncWebServerRequest *request)
{
    request->send(LittleFS, "/index.html", "text/html", false, processor);
}

// ============================================================================
// Initialize mDNS — safe to call again when WiFi reconnects
// ============================================================================

void initMDNS() {
    MDNS.end();  // Stop any previous instance
    if (MDNS.begin(WEB_NAME)) {
#ifdef DEBUG_NETWORK
        debuglnD("mDNS iniciado — sml.local disponible");
#endif
        MDNS.addService("http", "tcp", 80);
#ifdef DEBUG_NETWORK
        debuglnD("Servicio HTTP registrado en mDNS");
#endif
    } else {
#ifdef DEBUG_NETWORK
        debuglnW("mDNS no disponible — usa la IP directamente");
#endif
    }
}

// ============================================================================
// Initialize web server routes
// ============================================================================

void initWebServer()
{
    server.on("/", onRootRequest);
    server.on("/wifi-info", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      StaticJsonDocument<256> json;
      json["status"] = "ok";
      json["ssid"] = WiFi.SSID();
      json["ip"] = WiFi.localIP();
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

    // Endpoint: metadata de todos los efectos (estilo WLED /json/fxda)
    server.on("/fxdata", HTTP_GET, [](AsyncWebServerRequest *request) {
      StaticJsonDocument<8192> fxJson;
      JsonObject metas = fxJson.to<JsonObject>();
      for (uint8_t i = 0; i < EFFECT_COUNT; i++) {
        Effect* fx = effectRegistry[i].instance;
        if (!fx) continue;
        const char* m = fx->getMeta();
        if (m) metas[String(i + 1)] = m;
      }
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      serializeJson(fxJson, *response);
      request->send(response);
    });

    // Endpoint: lista de paletas con nombres y colores representativos
    server.on("/palettes", HTTP_GET, [](AsyncWebServerRequest *request) {
      StaticJsonDocument<4096> pDoc;
      uint8_t cnt = PaletteManager::count();
      // Nombres
      JsonArray names = pDoc["names"].to<JsonArray>();
      for (uint8_t i = 0; i < cnt; i++) {
        names.add(PaletteManager::getName(i));
      }
      // Swatches (6 colores representativos por paleta)
      CRGB swatch[6];
      JsonArray allSwatches = pDoc["swatches"].to<JsonArray>();
      for (uint8_t i = 0; i < cnt; i++) {
        PaletteManager::getSwatch(i, swatch, 6);
        JsonArray sw = allSwatches.add().to<JsonArray>();
        for (uint8_t j = 0; j < 6; j++) {
          sw.add(swatch[j].r);
          sw.add(swatch[j].g);
          sw.add(swatch[j].b);
        }
      }
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      serializeJson(pDoc, *response);
      request->send(response);
    });

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html").setTryGzipFirst(false).setCacheControl("no-cache");
    server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(400, "text/plain", "Not found"); });
    ElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
#ifdef DEBUG_WEB
    debuglnD("HTTP server started");
#endif
}
