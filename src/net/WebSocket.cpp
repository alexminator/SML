// ──────────────────────────────────────────────────────────────────────────────
// WebSocket.cpp — WebSocket server implementation
// ──────────────────────────────────────────────────────────────────────────────
#include "WebSocket.h"
#include "state/AppState.h"
#include "effects/EffectRegistry.h"
#include "net/WebServer.h"

// Debug level (needed before debug.h for macros to compile).
// Los flags DEBUG_* se heredan conceptualmente de main.cpp;
// al compilar este .cpp por separado, los #ifdef DEBUG_* son false,
// por lo que los mensajes de depuración no se muestran desde aquí.
#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
#include "config/debug.h"

#include <WiFi.h>

#include "power/PowerMgr.h"

// ============================================================================
// OBJETO GLOBAL
// ============================================================================

AsyncWebSocket ws("/ws");

// ============================================================================
// WiFi signal strength indicator (used by notifyClients)
// ============================================================================

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

// ============================================================================
// Notify all WebSocket clients with current state
// ============================================================================

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

        // Efectos y VU — usando EffectRegistry como fuente única
        for (uint8_t i = 0; i < EFFECT_COUNT; ++i)
            json[effectRegistry[i].jsonName] = (stripLed.effectId == i + 1 && stripLed.powerState) ? "on" : "off";

        // Serializar parámetros del efecto activo
        Effect* currentFx = getEffect(stripLed.effectId);
        if (currentFx) {
            JsonObject p = json["params"].to<JsonObject>();
            p["speed"]     = currentFx->getSpeed();
            p["intensity"] = currentFx->getIntensity();
            p["custom1"]   = currentFx->getCustom1();
            p["custom2"]   = currentFx->getCustom2();
            p["custom3"]   = currentFx->getCustom3();
            p["check1"]    = currentFx->getCheck1();
            p["check2"]    = currentFx->getCheck2();
            p["check3"]    = currentFx->getCheck3();
        }

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

// ============================================================================
// Handle incoming WebSocket messages
// ============================================================================

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
            digitalWrite(VOLUMENUP_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(long_delay));
            digitalWrite(VOLUMENUP_PIN, LOW);
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
        else if (strcmp(action, "setParams") == 0)
        {
            uint8_t id = json["effectId"].as<uint8_t>();
            Effect* fx = getEffect(id);
            if (fx) {
                // Usar operador | para mantener valor actual si no se envió el campo
                fx->setSpeed(json["speed"]     | fx->getSpeed());
                fx->setIntensity(json["intensity"] | fx->getIntensity());
                fx->setCustom1(json["custom1"] | fx->getCustom1());
                fx->setCustom2(json["custom2"] | fx->getCustom2());
                fx->setCustom3(json["custom3"] | fx->getCustom3());
                fx->setCheck1(json["check1"]   | fx->getCheck1());
                fx->setCheck2(json["check2"]   | fx->getCheck2());
                fx->setCheck3(json["check3"]   | fx->getCheck3());
#ifdef DEBUG_WEBSOCKET
                debugD("setParams effectId=");
                debugD_NUM(id, "%u");
                debugD("\n");
#endif
                saveEffectParams();
            }
        }
        notifyClients();
    }
}

// ============================================================================
// WebSocket event handler
// ============================================================================

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

// ============================================================================
// Initialize WebSocket
// ============================================================================

void initWebSocket()
{
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
#ifdef DEBUG_WEBSOCKET
    debuglnD("WebSocket server started");
#endif
}
