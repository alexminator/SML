// ──────────────────────────────────────────────────────────────────────────────
// WebSocket.cpp — WebSocket server implementation
// ──────────────────────────────────────────────────────────────────────────────
#include "WebSocket.h"
#include "state/AppState.h"
#include "effects/EffectRegistry.h"
#include "net/WebServer.h"

#include "config/debug_config.h"

#include <WiFi.h>

#include "power/PowerMgr.h"

// ============================================================================
// OBJETO GLOBAL
// ============================================================================

AsyncWebSocket ws("/ws");

// ============================================================================
// sendPeekData — Broadcast current LED colors as binary frame (WLED-compatible)
// ============================================================================
// Binary format:
//   [0] = 0x4C (magic 'L')
//   [1] = 0x01 (version)
//   [2] = N_PIXELS (width in LEDs)
//   [3] = 1 (height, single row)
//   [4..] = RGB data (R,G,B per pixel)
// Sends to ALL clients that opted in via {"lv": true}. Uses binaryAll()
// which copies data internally — safe for local buffer.
// Called from TaskLEDControl after stripLed.update().
// ============================================================================

static bool wsLiveActive = false; // any client wants live preview

void sendPeekData() {
    // ⚠ MUST be called with dataMutex held  (by TaskLEDControl).
    //   Lee leds[], wsLiveActive y llama ws.binaryAll() — todo bajo mutex.
    if (!wsLiveActive) return; // no client opted in

    // Rate limit to ~20fps (50ms)
    static uint32_t lastPeek = 0;
    uint32_t now = millis();
    if (now - lastPeek < 50) return;
    lastPeek = now;

    // Build binary frame and broadcast to all clients
    // WARNING: buf es stack-local (76 bytes para 24 LEDs) — seguro para TaskLEDControl
    // que tiene 2048 bytes de stack.
    uint8_t buf[4 + N_PIXELS * 3];
    buf[0] = 0x4C;  // magic 'L'
    buf[1] = 0x01;  // version 1 (single strip)
    buf[2] = N_PIXELS;
    buf[3] = 1;     // height = 1 row

    for (uint16_t i = 0; i < N_PIXELS; i++) {
        buf[4 + i * 3]     = leds[i].r;
        buf[4 + i * 3 + 1] = leds[i].g;
        buf[4 + i * 3 + 2] = leds[i].b;
    }

#ifdef DEBUG_WEBSOCKET
    static uint32_t lastPeekLog = 0;
    if (now - lastPeekLog > 5000) {  // log cada 5s para no saturar
        lastPeekLog = now;
        debugD("📤 sendPeekData: ");
        debugD_NUM(sizeof(buf), "%u");
        debugD(" bytes to ");
        debugD_NUM(ws.count(), "%u");
        debugD(" clients  |  sample pixel[0]: RGB(");
        debugD_NUM(leds[0].r, "%d");
        debugD(",");
        debugD_NUM(leds[0].g, "%d");
        debugD(",");
        debugD_NUM(leds[0].b, "%d");
        debugD(")\n");
    }
#endif

    ws.binaryAll(buf, sizeof(buf));
}

// ============================================================================
// Notify all WebSocket clients with current state
// ============================================================================

void notifyClients(bool includeParams)
{
    // Take mutex for reading shared data
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        JsonDocument json;

        // Usar valores numéricos directamente en JSON
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
        json["ip"] = WiFi.localIP();
        json["rssi"] = WiFi.RSSI();

        // System info (for Config tab)
        json["uptime"] = millis() / 1000;
        json["heap"] = ESP.getFreeHeap() / 1024;
        json["ver"] = SML_VERSION;

        // Añade el color actual (modern API)
        JsonObject color = json["color"].to<JsonObject>();
        color["r"] = stripLed.R;
        color["g"] = stripLed.G;
        color["b"] = stripLed.B;

        // Efectos y VU — usando EffectRegistry como fuente única
        for (uint8_t i = 0; i < EFFECT_COUNT; ++i)
            json[effectRegistry[i].jsonName] = (stripLed.effectId == i + 1 && stripLed.powerState) ? "on" : "off";

        // Random mode flag (0=off, 1=randomFX, 2=randomVU)
        json["randomMode"] = randomMode;

        // Params + meta solo en respuesta a acciones del usuario (evita que el
        // sync periódico sobreescriba sliders/checkboxes con valores viejos).
        if (includeParams) {
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
                p["palette"]   = currentFx->getPaletteIndex();
#ifdef DEBUG_WEBSOCKET
                debugD("notifyClients effectId=");
                debugD_NUM(stripLed.effectId, "%u");
                debugD(" check1=");
                debugD_NUM(currentFx->getCheck1(), "%d");
                debugD("\n");
#endif

                // Paleta del efecto activo (siempre incluida)
                json["palette"] = currentFx->getPaletteIndex();

                // Metadata del efecto activo
                const char* meta = currentFx->getMeta();
                if (meta) json["meta"] = meta;
            }
        }

        // Quick sanity check — reject unusually large payloads
        if (measureJson(json) + 256 > 2048) {
#ifdef DEBUG_WEBSOCKET
            debuglnE("JSON payload too large for WebSocket");
#endif
            xSemaphoreGive(dataMutex);
            return;
        }

        // Fixed-size buffer (static — NOT on task stack, TaskWebSocket solo tiene 4KB)
        static char buffer[2048];
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
// notifySensorData — Broadcast only sensor/battery/WiFi data (lightweight)
// ============================================================================

void notifySensorData()
{
    // Take mutex for reading shared data
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Dirty flag: skip if no state changes since last send
        static uint32_t lastGen = 0;
        if (stateGeneration == lastGen) {
            xSemaphoreGive(dataMutex);
            return;
        }
        lastGen = stateGeneration;

        // Small document — only real-time sensor/status fields (~180 bytes)
        JsonDocument json;

        json["level"] = batt.battLvl;
        json["battVoltage"] = batt.battVolts;
        json["charging"] = batt.chargeState;
        json["fullbatt"] = batt.fullBatt;
        json["temperature"] = temp;
        json["humidity"] = hum;
        json["ssid"] = WiFi.SSID();
        json["ip"] = WiFi.localIP();
        json["rssi"] = WiFi.RSSI();

        // System info (for Config tab)
        json["uptime"] = millis() / 1000;
        json["heap"] = ESP.getFreeHeap() / 1024;
        json["ver"] = SML_VERSION;

        // Static buffer (not on task stack — TaskWebSocket solo tiene 4KB)
        static char buffer[512];
        size_t len = serializeJson(json, buffer, sizeof(buffer));

        if (len > 0 && len < sizeof(buffer)) {
#ifdef DEBUG_WEBSOCKET
            debugD("Sensor payload size: ");
            debuglnD_NUM(len, "%u");
            debuglnD(" bytes");
#endif
            ws.textAll(buffer, len);
        }

        xSemaphoreGive(dataMutex);
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
        JsonDocument json;
        DeserializationError err = deserializeJson(json, data);
        if (err)
        {
#ifdef DEBUG_WEBSOCKET
            debuglnD("deserializeJson() failed with code ");
            debuglnD(err.c_str());
#endif
            return;
        }

        // ▸ WLED-style peek live view opt-in (multi-client) — procesar ANTES
        //   del early return de action==nullptr, porque {"lv":true} no lleva action.
        if (!json["lv"].isNull()) {
            // ⚠ wsLiveActive es leído por sendPeekData() en TaskLEDControl
            //   bajo dataMutex — escribir aquí también bajo mutex.
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                wsLiveActive = json["lv"].as<bool>();
                xSemaphoreGive(dataMutex);
            }
#ifdef DEBUG_WEBSOCKET
            debugD("🔴 Peek live view: ");
            debuglnD(wsLiveActive ? "ENABLED" : "DISABLED");
#endif
            // Si solo venía lv, responder con el estado completo + retornar
            // notifyClients toma su propio dataMutex — no retenerlo aquí.
            stateGeneration++;
            notifyClients();
            return;
        }

        const char *action = json["action"];
        const int effectId = json["effectId"] | -1;

        // ⚠ Solo asignar effectId si el campo existe. effectId 0 = Solid (color simple)
        // ⚠ Proteger stripLed y leds[] con dataMutex (TaskLEDControl los lee/escribe).
        if (effectId >= 0) {
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                stripLed.effectId = effectId;
                if (stripLed.powerState) stripLed.update();
                xSemaphoreGive(dataMutex);
            }
        }

        // ⚠ action puede ser NULL si el mensaje solo trae effectId
        if (action == nullptr) {
            stateGeneration++;
            notifyClients();  // notifyClients toma su propio dataMutex
            return;
        }

        // ── GPIO-ONLY ACTIONS (no shared state) ──────────────────────────────
        //   Se ejecutan sin dataMutex (NO tocan stripLed/leds/bt_powerState/lampState)
        if (strcmp(action, "volup") == 0) {
            digitalWrite(VOLUMENUP_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(long_delay));
            digitalWrite(VOLUMENUP_PIN, LOW);
        }
        else if (strcmp(action, "voldown") == 0) {
            digitalWrite(VOLUMENDOWN_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(long_delay));
            digitalWrite(VOLUMENDOWN_PIN, LOW);
        }
        else if (strcmp(action, "skipL") == 0) {
            digitalWrite(VOLUMENDOWN_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(short_delay));
            digitalWrite(VOLUMENDOWN_PIN, LOW);
        }
        else if (strcmp(action, "skipR") == 0) {
            digitalWrite(VOLUMENUP_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(short_delay));
            digitalWrite(VOLUMENUP_PIN, LOW);
        }
        else if (strcmp(action, "play-pause") == 0) {
            digitalWrite(PLAY_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(short_delay));
            digitalWrite(PLAY_PIN, LOW);
        }
        // ── SHARED-STATE ACTIONS (todo bajo dataMutex) ───────────────────────
        else
        {
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {

        if (strcmp(action, "toggle") == 0)
        {
            stripLed.powerState = !stripLed.powerState;
            stripLed.powerState ? stripLed.update() : stripLed.clear();
            if (!stripLed.powerState) randomMode = 0;
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
        else if (strcmp(action, "toggleBatt") == 0) {
            randomMode = 0;  // Exit random mode
            if (stripLed.effectId == EFFECT_BATTERY) {
                stripLed.effectId = EFFECT_SOLID;
            } else {
                stripLed.effectId = EFFECT_BATTERY;
            }
            if (stripLed.powerState) stripLed.update();
        }
        else if (strcmp(action, "toggleTemp") == 0) {
            randomMode = 0;  // Exit random mode
            if (stripLed.effectId == EFFECT_TEMP) {
                stripLed.effectId = EFFECT_SOLID;
            } else {
                stripLed.effectId = EFFECT_TEMP;
            }
            if (stripLed.powerState) stripLed.update();
        }
        else if (strcmp(action, "picker") == 0)
        {
            JsonObject color = json["color"];
            stripLed.R = color["r"].as<int>();
            stripLed.G = color["g"].as<int>();
            stripLed.B = color["b"].as<int>();
            if (!json["brightness"].isNull()) {
                stripLed.brightness = json["brightness"].as<int>();
            }
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
        else if (strcmp(action, "randomFX") == 0)
        {
            randomMode = json["state"].as<bool>() ? 1 : 0;
        }
        else if (strcmp(action, "randomVU") == 0)
        {
            randomMode = json["state"].as<bool>() ? 2 : 0;
        }
        // ── setParams: fx setters bajo mutex, saveEffectParams sin mutex ──
        else if (strcmp(action, "setParams") == 0)
        {
            uint8_t id = json["effectId"].as<uint8_t>();
            Effect* fx = getEffect(id);
            if (fx) {
                fx->setSpeed(json["speed"]     | fx->getSpeed());
                fx->setIntensity(json["intensity"] | fx->getIntensity());
                fx->setCustom1(json["custom1"] | fx->getCustom1());
                fx->setCustom2(json["custom2"] | fx->getCustom2());
                fx->setCustom3(json["custom3"] | fx->getCustom3());
                if (!json["check1"].isNull()) fx->setCheck1(json["check1"].as<int>() != 0);
                if (!json["check2"].isNull()) fx->setCheck2(json["check2"].as<int>() != 0);
                if (!json["check3"].isNull()) fx->setCheck3(json["check3"].as<int>() != 0);
                if (!json["palette"].isNull()) fx->setPaletteIndex(json["palette"].as<uint8_t>());
#ifdef DEBUG_WEBSOCKET
                debugD("setParams effectId=");
                debugD_NUM(id, "%u");
                debugD(" | check1 raw=");
                debugD_NUM(json["check1"].as<int>(), "%d");
                debugD(" current=");
                debugD_NUM(fx->getCheck1(), "%d");
                debugD("\n");
#endif
                // ⚠ Liberar mutex ANTES de saveEffectParams (lo toma internamente)
                xSemaphoreGive(dataMutex);
                saveEffectParams();
                // Volver a tomar (se libera al final del bloque else)
                if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
                    // Si falla, no podemos seguir — notificar y salir
                    stateGeneration++;
                    notifyClients();
                    return;
                }
            }
        }

                xSemaphoreGive(dataMutex);
            } // end if dataMutex acquired
        } // end else (shared-state actions)
        stateGeneration++;
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

        // Metadata disponible via HTTP GET /fxdata (cargado por la UI al iniciar)

        // Enviar estado completo al conectar
        stateGeneration++;
        notifyClients(true);
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
