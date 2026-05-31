// ──────────────────────────────────────────────────────────────────────────────
// WebServer.cpp — Web server + WiFi + LittleFS
// ──────────────────────────────────────────────────────────────────────────────
#include "WebServer.h"
#include "Settings.h"
#include "data.h"

#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
#include "debug.h"

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

// ============================================================================
// Root request handler
// ============================================================================

void onRootRequest(AsyncWebServerRequest *request)
{
    request->send(LittleFS, "/index.html", "text/html", false, processor);
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
