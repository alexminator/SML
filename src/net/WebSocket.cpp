// ──────────────────────────────────────────────────────────────────────────────
// WebSocket.cpp — WebSocket server implementation
// ──────────────────────────────────────────────────────────────────────────────
#include "WebSocket.h"
#include "state/AppState.h"
#include "effects/EffectRegistry.h"
#include "net/WebServer.h"
#include "net/data.h"

#include "config/debug_config.h"

#include <WiFi.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <algorithm>

#include "power/PowerMgr.h"

// ============================================================================
// OBJETO GLOBAL
// ============================================================================

AsyncWebSocket ws("/ws");

// ── WebSocket client tracking ─────────────────────────────────────────────────
//   Mantiene un array de clientes conectados para mostrarlos en la UI.
//   El primer cliente en conectarse es el "master".
// ──────────────────────────────────────────────────────────────────────────────
#define WS_MAX_CLIENTS 8

static uint32_t   _wsClientIds[WS_MAX_CLIENTS];
static IPAddress  _wsClientIps[WS_MAX_CLIENTS];
static uint8_t    _wsClientCount = 0;
static uint32_t   _wsMasterId = 0;
static uint8_t    _wsChangesGeneration = 0;  // ++ on any change (log, connect, disconnect)
static uint8_t    _wsClChangesGeneration = 0; // ++ only on client connect/disconnect

static void _wsAddClient(uint32_t id, IPAddress ip) {
    if (_wsClientCount >= WS_MAX_CLIENTS) return;
    if (_wsClientCount == 0) _wsMasterId = id;  // first = master
    _wsClientIds[_wsClientCount] = id;
    _wsClientIps[_wsClientCount] = ip;
    _wsClientCount++;
    _wsChangesGeneration++;
    _wsClChangesGeneration++;
}

static void _wsRemoveClient(uint32_t id) {
    for (uint8_t i = 0; i < _wsClientCount; i++) {
        if (_wsClientIds[i] == id) {
            for (uint8_t j = i; j < _wsClientCount - 1; j++) {
                _wsClientIds[j] = _wsClientIds[j + 1];
                _wsClientIps[j] = _wsClientIps[j + 1];
            }
            _wsClientCount--;
            if (id == _wsMasterId && _wsClientCount > 0) {
                _wsMasterId = _wsClientIds[0];  // next client becomes master
            }
            _wsChangesGeneration++;
            _wsClChangesGeneration++;
            break;
        }
    }
}

// ¿El cliente es el master actual? El master es el primer cliente conectado y
// pasa al siguiente al desconectarse (_wsRemoveClient).
// Lo usan las acciones terminales (reboot / factory reset), que el README
// documenta como "reserved for the master".
static bool _wsIsMaster(uint32_t clientId) {
    return _wsClientCount > 0 && clientId != 0 && clientId == _wsMasterId;
}

// ── WebSocket action log (circular buffer) ──────────────────────────────────────
//   Últimas N acciones de clientes, enviadas con la lista de clientes.
//   Tipos (ty): 0=effect 1=color 2=brightness 3=params 4=power 5=lamp 6=bt 7=random
//               8=battery 9=temp 10=volup 11=voldown 12=skipL 13=skipR 14=play-pause
// ────────────────────────────────────────────────────────────────────────────────
#define WS_LOG_SIZE 64

struct WsLogEntry {
    uint32_t timestamp;      // uptime seconds   (millis() / 1000)
    uint32_t clientId;
    uint8_t  type;
    int16_t  val1;
    int16_t  val2;
    int16_t  val3;
};

static WsLogEntry _wsActionLog[WS_LOG_SIZE];
static uint8_t _wsLogHead = 0;
static uint8_t _wsLogCount = 0;

// Dirty-flags separados para client-list y action-log.
// Solo se limpian DESPUÉS de que ws.textAll() haya enviado con éxito.
static uint8_t _lastSentClGen = 0xFE;   // forzará envío en la primera llamada
static uint8_t _lastSentLogGen = 0xFE;

static void _wsLogAction(uint32_t clientId, uint8_t type, int16_t v1, int16_t v2, int16_t v3) {
    WsLogEntry* e = &_wsActionLog[_wsLogHead];
    e->timestamp = millis() / 1000;
    e->clientId = clientId;
    e->type = type;
    e->val1 = v1;
    e->val2 = v2;
    e->val3 = v3;
    _wsLogHead = (_wsLogHead + 1) % WS_LOG_SIZE;
    if (_wsLogCount < WS_LOG_SIZE) _wsLogCount++;
    _wsChangesGeneration++;          // signal que el log cambió
    _lastSentLogGen = 0xFE;         // marca dirty para que se re‑envíe
}

// ============================================================================
// notifyWSClientList — Broadcast full client list (on connect/disconnect)
// ============================================================================

void notifyWSClientList() {
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // ── MENSAJE 1: Client list ──────────────────────────────────────────────
        //   Se envía solo si _wsClChangesGeneration cambió (nuevo cliente / desconexión)
        if (_wsClChangesGeneration != _lastSentClGen) {
            DynamicJsonDocument clJson(512);
            clJson["wsSlaves"] = (_wsClientCount > 0) ? _wsClientCount - 1 : 0;
            clJson["wsMax"] = WS_MAX_CLIENTS;
            JsonArray arr = clJson["wsClientList"].to<JsonArray>();
            for (uint8_t i = 0; i < _wsClientCount; i++) {
                JsonObject c = arr.add<JsonObject>();
                c["id"] = _wsClientIds[i];
                c["ip"] = _wsClientIps[i];
                c["master"] = (_wsClientIds[i] == _wsMasterId);
            }

            static char clBuf[512];
            size_t clLen = serializeJson(clJson, clBuf, sizeof(clBuf));
            if (clLen > 0 && clLen < sizeof(clBuf)) {
                ws.textAll(clBuf, clLen);
                _lastSentClGen = _wsClChangesGeneration;  // solo se limpia si se envió OK
            }
        }

        // ── MENSAJE 2: Action log (solo si hay cambios) ─────────────────────────
        if (_lastSentLogGen == 0xFE) {   // dirty flag → hay cambios sin enviar
            // Capacidad calculada con las macros oficiales de ArduinoJson.
            //   JSON_ARRAY_SIZE(N)  → slots del array
            //   JSON_OBJECT_SIZE(6) → slots para cada entrada (6 campos)
            //   +64                 → margen para claves y wrapper
            size_t capacity = JSON_ARRAY_SIZE(_wsLogCount)
                           + _wsLogCount * JSON_OBJECT_SIZE(6)
                           + 64;
            DynamicJsonDocument logJson(capacity);
            JsonArray logArr = logJson["wsActionLog"].to<JsonArray>();
            uint8_t idx = (_wsLogHead + WS_LOG_SIZE - _wsLogCount) % WS_LOG_SIZE;
            for (uint8_t i = 0; i < _wsLogCount; i++) {
                WsLogEntry* e = &_wsActionLog[idx];
                JsonObject le = logArr.add<JsonObject>();
                le["t"]  = e->timestamp;
                le["c"]  = e->clientId;
                le["ty"] = e->type;
                le["v1"] = e->val1;
                le["v2"] = e->val2;
                le["v3"] = e->val3;
                idx = (idx + 1) % WS_LOG_SIZE;
            }

            // Buffer exacto para serializar: medimos primero, asignamos después.
            size_t jsonLen = measureJson(logJson) + 1;  // +1 para null byte
            char* logBuf = (char*)malloc(jsonLen);
            if (logBuf) {
                size_t written = serializeJson(logJson, logBuf, jsonLen);
#ifdef DEBUG_WEBSOCKET
                debugD("📋 Log JSON: ");
                debugD_NUM(_wsLogCount, "%u");
                debugD(" entries, capacity=");
                debugD_NUM(capacity, "%u");
                debugD(", jsonLen=");
                debugD_NUM(jsonLen, "%u");
                debugD(", written=");
                debugD_NUM(written, "%u");
                debugD(", measure=");
                debugD_NUM(measureJson(logJson), "%u");
                if (written < jsonLen - 1) debugD(" ⚠ NEAR_TRUNCATED!");
                if (written == 0)           debugD(" ⚠ WRITE_FAILED!");
                debugD("\n");
#endif
                ws.textAll(logBuf, written);
                free(logBuf);
                _lastSentLogGen = 0;    // se limpió — se envió OK
            }
        }

        xSemaphoreGive(dataMutex);
    }
}

// ============================================================================
// Peek live view — current LED colors as binary frame (WLED-compatible)
// ============================================================================
// Binary format:
//   [0] = 0x4C (magic 'L')
//   [1] = 0x01 (version)
//   [2] = N_PIXELS (width in LEDs)
//   [3] = 1 (height, single row)
//   [4..] = RGB data (R,G,B per pixel)
//
// El snapshot se arma bajo dataMutex; la cola de red se toca después y sólo
// para el cliente que pidió Peek. El intervalo de 40 ms limita nominalmente a 25 fps,
// alineado con el render del canvas. Si el cliente va atrasado no se encolan frames.
// ============================================================================

static uint32_t wsPeekClientId = 0;
static constexpr uint32_t PEEK_FRAME_INTERVAL_MS = 40; // máximo nominal de 25 fps

bool buildPeekFrame(uint8_t *buf, size_t bufSize, size_t *len, uint32_t *clientId) {
    // ⚠ Llamar con dataMutex tomado (lo hace TaskLEDControl).
    *len = 0;
    *clientId = wsPeekClientId;
    if (wsPeekClientId == 0) return false;  // nadie pidió el stream
    const size_t frameSize = 4 + (size_t)N_PIXELS * 3;
    if (bufSize < frameSize) return false;

    // Hasta 25 fps, sin acelerar el loop LED existente (20 ms).
    // Se omite el envío si no toca, no se acumulan frames de preview.
    static uint32_t lastPeek = 0;
    const uint32_t now = millis();
    if (now - lastPeek < PEEK_FRAME_INTERVAL_MS) return false;
    lastPeek = now;

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
        debugD("📤 peek frame: ");
        debugD_NUM((unsigned)frameSize, "%u");
        debugD(" bytes para client #");
        debugD_NUM(wsPeekClientId, "%u");
        debugD(" | pixel[0]: RGB(");
        debugD_NUM(leds[0].r, "%d");
        debugD(",");
        debugD_NUM(leds[0].g, "%d");
        debugD(",");
        debugD_NUM(leds[0].b, "%d");
        debugD(")\n");
    }
#endif

    *len = frameSize;
    return true;
}

void sendPeekFrame(uint32_t clientId, const uint8_t *buf, size_t len) {
    // ⚠ Llamar SIN dataMutex tomado. Limitar a una trama pendiente evita
    // latencia en cascada; desactivar close-on-full hace el stream best-effort.
    if (!len || clientId == 0) return;
    AsyncWebSocketClient *client = ws.client(clientId);
    if (!client || client->queueLen() != 0 || !ws.availableForWrite(clientId)) return;
    ws.binary(clientId, buf, len);
}

// ============================================================================
// Notify all WebSocket clients with current state
// ============================================================================

void notifyClients(bool includeParams)
{
    // Take mutex for reading shared data
    // Capacidad única para el documento y el buffer de estado. Antes había tres
    // números distintos (doc 2048, guard 2048−256, buffer 2048): el guard
    // rechazaba payloads que sí cabían en el buffer y descartaba el mensaje
    // ENTERO, dejando al cliente sin ninguna actualización de estado. El estado
    // real ronda los ~1,9 KB (50 campos *Status + params + meta), así que el
    // margen de 256 era el que decidía el envío.
    static const size_t WS_STATE_CAPACITY = 3072;
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        DynamicJsonDocument json(WS_STATE_CAPACITY);

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
        json["mac"] = WiFi.macAddress();
        json["rssi"] = WiFi.RSSI();
        // Diagnóstico visible en la consola del navegador: [SML ← ESP32]
        json["wifiOk"] = (WiFi.status() == WL_CONNECTED);
        json["stripSuspend"] = wifiStripSuspended;
        json["wifiLostEvents"] = wifiLostEvents;
        json["wifiTimeout"] = wifiLostStripTimeoutMs / 1000UL;

        // System info (for Config tab)
        json["uptime"] = millis() / 1000;
        json["heap"] = ESP.getFreeHeap() / 1024;
        json["ver"] = SML_VERSION;

        // WebSocket client count (slaves only — master doesn't count)
        json["wsSlaves"] = (_wsClientCount > 0) ? _wsClientCount - 1 : 0;
        json["wsMax"] = WS_MAX_CLIENTS;

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

        // Random FX config (broadcast to all clients)
        json["randomFXMode"] = randomFXMode;
        json["randomFXDuration"] = randomFXDuration;
        // ⚠ SIEMPRE incluimos randomFXPool, incluso vacío: cuando el master
        //   limpia la playlist, el broadcast con una array vacía es lo único
        //   que permite a los otros clientes sincronizar su estado local.
        {
            JsonArray pool = json["randomFXPool"].to<JsonArray>();
            for (int id : randomFXPool) pool.add(id);
        }
        if (!randomFXCategories.empty()) {
            JsonArray cats = json["randomFXCategories"].to<JsonArray>();
            for (int c : randomFXCategories) cats.add(c);
        }

        // Random VU config is server-authoritative and sent to all clients.
        json["randomVUDuration"] = randomVUDuration;
        {
            JsonArray pool = json["randomVUPool"].to<JsonArray>();
            for (int id : randomVUPool) pool.add(id);
        }

        // Direct effectId (for random FX cycling and general sync)
        json["effectId"] = stripLed.effectId;

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

        // Sanity check contra la capacidad real del buffer (+1 por el NUL que
        // escribe serializeJson). No se usa un margen arbitrario: si cabe en el
        // buffer, se envía.
        if (measureJson(json) + 1 >= WS_STATE_CAPACITY) {
#ifdef DEBUG_WEBSOCKET
            debuglnE("JSON payload too large for WebSocket");
#endif
            xSemaphoreGive(dataMutex);
            return;
        }

        // Fixed-size buffer (static — NOT on task stack, TaskWebSocket solo tiene 4KB)
        static char buffer[WS_STATE_CAPACITY];
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
        DynamicJsonDocument json(512);

        json["level"] = batt.battLvl;
        json["battVoltage"] = batt.battVolts;
        json["charging"] = batt.chargeState;
        json["fullbatt"] = batt.fullBatt;
        json["temperature"] = temp;
        json["humidity"] = hum;
        json["ssid"] = WiFi.SSID();
        json["ip"] = WiFi.localIP();
        json["mac"] = WiFi.macAddress();
        json["rssi"] = WiFi.RSSI();
        // Diagnóstico visible en la consola del navegador: [SML ← ESP32]
        json["wifiOk"] = (WiFi.status() == WL_CONNECTED);
        json["stripSuspend"] = wifiStripSuspended;
        json["wifiLostEvents"] = wifiLostEvents;
        json["wifiTimeout"] = wifiLostStripTimeoutMs / 1000UL;

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
// sendBatteryHistory — Broadcast current battery log to all clients
// ============================================================================

static void sendBatteryHistory() {
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return;
    }

    // Ensure log is loaded
    batt.loadBatteryLog();

    if (batt.battLogCount == 0) {
        xSemaphoreGive(dataMutex);
        return;
    }

    // Build { "battHistory": [{"t":...,"v":...,"l":...}, ...] }
    // Use the same buffer+mutex pattern as notifyClients (proven to work)
    int count = min(batt.battLogCount, BATT_LOG_SIZE);
    size_t cap = JSON_ARRAY_SIZE(count) + count * JSON_OBJECT_SIZE(3) + 64;
    DynamicJsonDocument doc(cap);
    JsonArray arr = doc["battHistory"].to<JsonArray>();
    int idx = (batt.battLogHead + BATT_LOG_SIZE - count) % BATT_LOG_SIZE;
    for (int i = 0; i < count; i++) {
        JsonObject e = arr.add<JsonObject>();
        e["t"] = batt.battLog[idx].uptime;
        e["v"] = batt.battLog[idx].voltage;
        e["l"] = batt.battLog[idx].level;
        idx = (idx + 1) % BATT_LOG_SIZE;
    }

    xSemaphoreGive(dataMutex);

    // Serialize to static buffer and send via textAll (same as notifyClients)
    static char outBuf[2048];
    size_t jsonLen = serializeJson(doc, outBuf, sizeof(outBuf));
    if (jsonLen >= sizeof(outBuf)) return;

#ifdef DEBUG_BATTERY
    debugD("[BATT] sendBatteryHistory — sending ");
    debugD_NUM(count, "%d");
    debugD(" entries, jsonLen=");
    debuglnD_NUM(jsonLen, "%u");
#endif
    ws.textAll(outBuf, jsonLen);
}

// ── Overload: send battery history to a SINGLE client (no ws.textAll) ────
static void sendBatteryHistory(AsyncWebSocketClient* client) {
    if (!client) return;
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return;
    }

    batt.loadBatteryLog();

    if (batt.battLogCount == 0) {
        xSemaphoreGive(dataMutex);
        return;
    }

    int count = min(batt.battLogCount, BATT_LOG_SIZE);
    size_t cap = JSON_ARRAY_SIZE(count) + count * JSON_OBJECT_SIZE(3) + 64;
    DynamicJsonDocument doc(cap);
    JsonArray arr = doc["battHistory"].to<JsonArray>();
    int idx = (batt.battLogHead + BATT_LOG_SIZE - count) % BATT_LOG_SIZE;
    for (int i = 0; i < count; i++) {
        JsonObject e = arr.add<JsonObject>();
        e["t"] = batt.battLog[idx].uptime;
        e["v"] = batt.battLog[idx].voltage;
        e["l"] = batt.battLog[idx].level;
        idx = (idx + 1) % BATT_LOG_SIZE;
    }

    xSemaphoreGive(dataMutex);

    static char outBuf[2048];
    size_t jsonLen = serializeJson(doc, outBuf, sizeof(outBuf));
    if (jsonLen >= sizeof(outBuf)) return;

#ifdef DEBUG_BATTERY
    debugD("[BATT] sendBatteryHistory → client #");
    debugD_NUM(client->id(), "%u");
    debugD(": ");
    debugD_NUM(count, "%d");
    debugD(" entries, jsonLen=");
    debuglnD_NUM(jsonLen, "%u");
#endif
    client->text(outBuf, jsonLen);
}

// ============================================================================
// Handle incoming WebSocket messages
// ============================================================================

// Opt-in/opt-out del preview, registrado por clientId para que sólo el dueño
// del Peek reciba el stream y otro cliente no pueda desuscribirlo accidentalmente.
static bool setPeekLiveState(uint32_t clientId, bool enabled) {
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    if (enabled) {
        if (wsPeekClientId != 0 && wsPeekClientId != clientId) {
            xSemaphoreGive(dataMutex);
            return false;
        }
        wsPeekClientId = clientId;
    } else if (wsPeekClientId == clientId) {
        wsPeekClientId = 0;
    }
    xSemaphoreGive(dataMutex);
    return true;
}

static void clearPeekLiveState(uint32_t clientId) {
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (wsPeekClientId == clientId) wsPeekClientId = 0;
        xSemaphoreGive(dataMutex);
    }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, uint32_t clientId, IPAddress clientIp)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
        DynamicJsonDocument json(1024);
        DeserializationError err = deserializeJson(json, data);
        if (err)
        {
#ifdef DEBUG_WEBSOCKET
            debugD("deserializeJson() failed with code ");
            debugD(err.c_str());
            debugD("\n");
#endif
            return;
        }

        // Peek opt-in procesado antes del early return de action==nullptr.
        // Sólo el cliente que inicia Peek es suscriptor del stream.
        if (!json["lv"].isNull()) {
            const bool lvOn = json["lv"].as<bool>();
            AsyncWebSocketClient *peekClient = ws.client(clientId);
            if (lvOn && peekClient) peekClient->setCloseClientOnQueueFull(false);
            const bool peekStateUpdated = setPeekLiveState(clientId, lvOn);
            if (!peekStateUpdated && lvOn && peekClient) {
                peekClient->setCloseClientOnQueueFull(true);
            } else if (peekStateUpdated && !lvOn && peekClient) {
                peekClient->setCloseClientOnQueueFull(true);
            }
#ifdef DEBUG_WEBSOCKET
            // Se usa la copia local: leer wsPeekClientId aquí fuera del mutex sería
            // una carrera (aunque sólo alimente un mensaje de debug).
            debugD("🔴 Peek live view: ");
            debuglnD(lvOn ? "ENABLED" : "DISABLED");
#endif
            // El stream es binario unicast; no hace falta broadcast del estado
            // completo para habilitar/deshabilitarlo.
            if (!peekStateUpdated) {
                static const char PEEK_BUSY_RESPONSE[] = "{\"peekBusy\":true}";
                ws.text(clientId, PEEK_BUSY_RESPONSE, sizeof(PEEK_BUSY_RESPONSE) - 1);
#ifdef DEBUG_WEBSOCKET
                debuglnW("Peek request rejected — another client owns the stream");
#endif
            }
            return;
        }

        const char *action = json["action"];
        const int effectId = json["effectId"] | -1;
        bool shouldSaveGlobalState = false;

        // ⚠ Solo asignar effectId si el campo existe. effectId 0 = Solid (color simple)
        // ⚠ Proteger stripLed y leds[] con dataMutex (TaskLEDControl los lee/escribe).
        if (effectId >= 0) {
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                stripLed.effectId = effectId;
                if (stripLed.powerState) stripLed.update();
                shouldSaveGlobalState = true;
                xSemaphoreGive(dataMutex);
            }
            _wsLogAction(clientId, 0, effectId, 0, 0);
        }

        // ⚠ action puede ser NULL si el mensaje solo trae effectId
        if (action == nullptr) {
            if (shouldSaveGlobalState) saveGlobalState();
            stateGeneration++;
            notifyClients(false);  // notifyClients toma su propio dataMutex
            notifyWSClientList();  // Broadcast updated action log
            return;
        }

        // ── REFRESH CLIENT LIST (force broadcast of current list + action log) ──
        if (strcmp(action, "refreshClientList") == 0) {
            _lastSentClGen = 0xFE;   // Force re-send client list
            _lastSentLogGen = 0xFE;   // Force re-send action log
            notifyWSClientList();
            return;
        }

        // ── CONFIG: tiempo de apagado de la tira al caer el WiFi (modo AC) ──
        if (strcmp(action, "setWifiTimeout") == 0) {
            int secs = json["seconds"] | -1;
            if (secs >= 5 && secs <= 3600 &&
                xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                wifiLostStripTimeoutMs = (uint32_t)secs * 1000UL;
                xSemaphoreGive(dataMutex);
                saveGlobalState();  // snapshot protegido; escritura fuera del mutex
            }
            stateGeneration++;
            notifyClients(true);
            return;
        }

        // ── REQUEST FULL STATE (cliente vuelve de minimizar / reconecta) ──
        //   Envía notifyClients(true) a TODOS los clientes. Sin side effects.
        if (strcmp(action, "requestFullState") == 0) {
            stateGeneration++;
            notifyClients(true);
            return;
        }

        // ── REQUEST BATTERY HISTORY (for chart) ─────────────────────────
        if (strcmp(action, "requestBattHistory") == 0) {
#ifdef DEBUG_BATTERY
            debuglnD("[BATT] requestBattHistory received from client");
#endif
            sendBatteryHistory();
            return;
        }

        // ── REBOOT / FACTORY RESET (terminal actions — no mutex needed) ──────
        //   Master-only: el README lo documenta como "reserved for the master",
        //   pero hasta ahora sólo lo aplicaba la UI (client-side). Aquí se aplica
        //   de verdad: un slave que envíe el comando crudo por WebSocket no puede
        //   reiniciar ni borrar la configuración del dispositivo.
        if (strcmp(action, "reboot") == 0) {
            if (!_wsIsMaster(clientId)) {
#ifdef DEBUG_WEBSOCKET
                debuglnW("⛔ reboot ignorado — el cliente no es el master");
#endif
                return;
            }
#ifdef DEBUG_WEBSOCKET
            debuglnD("🔄 Rebooting ESP32...");
#endif
            // Small delay to allow the response to be sent before restart
            vTaskDelay(pdMS_TO_TICKS(100));
            ESP.restart();
            return; // never reached
        }
        if (strcmp(action, "factoryReset") == 0) {
            if (!_wsIsMaster(clientId)) {
#ifdef DEBUG_WEBSOCKET
                debuglnW("⛔ factoryReset ignorado — el cliente no es el master");
#endif
                return;
            }
#ifdef DEBUG_WEBSOCKET
            debuglnD("🗑 Factory reset — clearing all settings...");
#endif
            // Only clear WiFi credentials if compiled defaults exist as fallback.
            // Otherwise the device would lose WiFi access with no way to recover.
            if (strlen(WIFI_SSID) > 0) {
                Preferences prefs;
                prefs.begin("wifi", false);
                prefs.clear();
                prefs.end();
#ifdef DEBUG_WEBSOCKET
                debuglnD("   ✓ WiFi credentials cleared (fallback to compiled defaults)");
#endif
            } else {
#ifdef DEBUG_WEBSOCKET
                debuglnW("   ⚠ No compiled WiFi defaults — keeping saved credentials");
#endif
            }
            // Delete effect params file (safe — will be recreated on next change)
            if (LittleFS.exists("/params.json")) {
                LittleFS.remove("/params.json");
#ifdef DEBUG_WEBSOCKET
                debuglnD("   ✓ Effect params cleared");
#endif
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            ESP.restart();
            return; // never reached
        }

        // ── GPIO-ONLY ACTIONS (no shared state) ──────────────────────────────
        //   Se ejecutan sin dataMutex (NO tocan stripLed/leds/bt_powerState/lampState)
        if (strcmp(action, "volup") == 0) {
            digitalWrite(VOLUMENUP_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(long_delay));
            digitalWrite(VOLUMENUP_PIN, LOW);
            _wsLogAction(clientId, 10, 0, 0, 0);
        }
        else if (strcmp(action, "voldown") == 0) {
            digitalWrite(VOLUMENDOWN_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(long_delay));
            digitalWrite(VOLUMENDOWN_PIN, LOW);
            _wsLogAction(clientId, 11, 0, 0, 0);
        }
        else if (strcmp(action, "skipL") == 0) {
            digitalWrite(VOLUMENDOWN_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(short_delay));
            digitalWrite(VOLUMENDOWN_PIN, LOW);
            _wsLogAction(clientId, 12, 0, 0, 0);
        }
        else if (strcmp(action, "skipR") == 0) {
            digitalWrite(VOLUMENUP_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(short_delay));
            digitalWrite(VOLUMENUP_PIN, LOW);
            _wsLogAction(clientId, 13, 0, 0, 0);
        }
        else if (strcmp(action, "play-pause") == 0) {
            digitalWrite(PLAY_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(short_delay));
            digitalWrite(PLAY_PIN, LOW);
            _wsLogAction(clientId, 14, 0, 0, 0);
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
            shouldSaveGlobalState = true;
            _wsLogAction(clientId, 4, stripLed.powerState ? 1 : 0, 0, 0);
        }
        else if (strcmp(action, "lamp") == 0)
        {
            lampState = !lampState;
            digitalWrite(LAMP_PIN, lampState ? LOW : HIGH);
#ifdef DEBUG_LED
            debuglnD(lampState ? "Lampara ON" : "Lampara OFF");
#endif
            _wsLogAction(clientId, 5, lampState ? 1 : 0, 0, 0);
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
            shouldSaveGlobalState = true;
            _wsLogAction(clientId, 2, brightness, 0, 0);
        }
        else if (strcmp(action, "toggleBatt") == 0) {
            randomMode = 0;  // Exit random mode
            if (stripLed.effectId == EFFECT_BATTERY) {
                stripLed.effectId = EFFECT_SOLID;
            } else {
                stripLed.effectId = EFFECT_BATTERY;
            }
            if (stripLed.powerState) stripLed.update();
            // Log battery toggle as type 8
            shouldSaveGlobalState = true;
            _wsLogAction(clientId, 8, stripLed.effectId == EFFECT_BATTERY ? 1 : 0, 0, 0);
        }
        else if (strcmp(action, "toggleTemp") == 0) {
            randomMode = 0;  // Exit random mode
            if (stripLed.effectId == EFFECT_TEMP) {
                stripLed.effectId = EFFECT_SOLID;
            } else {
                stripLed.effectId = EFFECT_TEMP;
            }
            if (stripLed.powerState) stripLed.update();
            // Log temperature toggle as type 9
            shouldSaveGlobalState = true;
            _wsLogAction(clientId, 9, stripLed.effectId == EFFECT_TEMP ? 1 : 0, 0, 0);
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
            shouldSaveGlobalState = true;
#ifdef DEBUG_LED
            debugD("RGB: ");
            debugD_NUM(stripLed.R, "%d");
            debugD(", ");
            debugD_NUM(stripLed.G, "%d");
            debugD(", ");
            debuglnD_NUM(stripLed.B, "%d");
#endif
            _wsLogAction(clientId, 1, stripLed.R, stripLed.G, stripLed.B);
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
            _wsLogAction(clientId, 6, bt_powerState ? 1 : 0, 0, 0);
        }
        else if (strcmp(action, "randomFX") == 0)
        {
            if (json["state"].as<bool>()) {
                randomMode = 1;
                lastRandomSwitch = millis();
                randomPlaylistIndex = 0;
            } else if (randomMode == 1) {
                randomMode = 0;
            }
            _wsLogAction(clientId, 7, 1, 0, 0);
        }
        else if (strcmp(action, "randomVU") == 0 || strcmp(action, "randomVUConfig") == 0)
        {
            const bool configOnly = strcmp(action, "randomVUConfig") == 0;
            if (configOnly && !json["duration"].isNull()) {
                randomVUDuration = constrain(json["duration"].as<int>(), 3, 30);
            }
            if (!configOnly && !json["effectPool"].isNull() && json["effectPool"].is<JsonArray>()) {
                randomVUPool.clear();
                for (JsonVariant value : json["effectPool"].as<JsonArray>()) {
                    const int id = value.as<int>();
                    // Only real, active VU effects are eligible; removed registry
                    // placeholders (IDs 49-51) are deliberately excluded.
                    if (id >= 12 && id <= 17) randomVUPool.push_back(id);
                    else if (id == 47 || id == 48) randomVUPool.push_back(id);
                }
                std::sort(randomVUPool.begin(), randomVUPool.end());
                randomVUPool.erase(std::unique(randomVUPool.begin(), randomVUPool.end()), randomVUPool.end());
                if (randomVUPool.empty()) randomVUPool = {12, 13, 14, 15, 16, 17, 47, 48};
            }
            if (!configOnly) {
                const bool shouldStart = json["state"].as<bool>();
                if (shouldStart && !json["duration"].isNull()) {
                    randomVUDuration = constrain(json["duration"].as<int>(), 3, 30);
                }
                if (shouldStart && !json["effectPool"].isNull() && !json["effectPool"].is<JsonArray>()) {
                    randomVUPool.clear();
                }
                if (shouldStart && randomVUPool.empty()) {
                    randomVUPool = {12, 13, 14, 15, 16, 17, 47, 48};
                }
                if (shouldStart && !randomVUPool.empty()) {
                    randomMode = 2;
                    const int nextId = chooseNextRandomVUEffect(stripLed.effectId);
                    if (nextId >= 0) {
                        stripLed.effectId = nextId;
                        if (stripLed.powerState) stripLed.update();
                    }
                    lastRandomSwitch = millis();
                } else if (shouldStart) {
                    randomMode = 0;
                } else if (!shouldStart) {
                    if (randomMode == 2) randomMode = 0;
                    if (json["effectId"].is<int>()) {
                        stripLed.effectId = json["effectId"].as<int>();
                        if (stripLed.powerState) stripLed.update();
                    }
                }
            } else if (randomMode == 2 && !json["duration"].isNull()) {
                // Restart the interval from the moment the user changes it.
                lastRandomSwitch = millis();
            }
            if (randomMode == 2 && !configOnly && !json["duration"].isNull()) {
                lastRandomSwitch = millis();
            }
            if (!configOnly) _wsLogAction(clientId, 7, 2, 0, 0);
        }
        else if (strcmp(action, "randomConfig") == 0)
        {
            if (!json["mode"].isNull())
                randomFXMode = String((const char*)json["mode"]);
            if (!json["duration"].isNull())
                randomFXDuration = json["duration"].as<int>();
            if (!json["effectPool"].isNull()) {
                randomFXPool.clear();
                for (JsonVariant v : json["effectPool"].as<JsonArray>())
                    randomFXPool.push_back(v.as<int>());
            }
            if (!json["categories"].isNull()) {
                randomFXCategories.clear();
                for (JsonVariant v : json["categories"].as<JsonArray>())
                    randomFXCategories.push_back(v.as<int>());
            }
            if (json["start"].as<bool>()) {
                randomMode = 1;
                randomPlaylistIndex = 0;
                lastRandomSwitch = millis();
                // Pick the FIRST random effect immediately — no esperar
                // randomFXDuration segundos para que el usuario vea algo.
                if (!randomFXPool.empty()) {
                    int firstId;
                    if (randomFXMode == "playlist") {
                        firstId = randomFXPool[randomPlaylistIndex];
                        randomPlaylistIndex = (randomPlaylistIndex + 1) % randomFXPool.size();
                    } else {
                        firstId = randomFXPool[random(0, randomFXPool.size())];
                    }
                    stripLed.effectId = firstId;
                    if (stripLed.powerState) stripLed.update();
                }
            }
            _wsLogAction(clientId, 7, 3, 0, 0);
        }
        // ── setParams: setters + marca dirty bajo mutex (write al final) ──
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
                _wsLogAction(clientId, 3, id, json["speed"] | fx->getSpeed(), json["intensity"] | fx->getIntensity());
#ifdef DEBUG_WEBSOCKET
                debugD("setParams effectId=");
                debugD_NUM(id, "%u");
                debugD(" | check1 raw=");
                debugD_NUM(json["check1"].as<int>(), "%d");
                debugD(" current=");
                debugD_NUM(fx->getCheck1(), "%d");
                debugD("\n");
#endif
                // ⚠ Sólo marca el efecto como pendiente de persistir (no toca el
                // filesystem), así que es seguro con dataMutex tomado. El write
                // real ocurre en saveEffectParamsFlush() al final del handler.
                saveEffectParams(id);
            }
        }

                xSemaphoreGive(dataMutex);
            } // end if dataMutex acquired
        } // end else (shared-state actions)
        if (shouldSaveGlobalState) saveGlobalState();
        stateGeneration++;
        notifyClients(false);
        notifyWSClientList();  // Broadcast updated action log
    }

    // Persistencia diferida de params de efectos. dataMutex ya está libre en este
    // punto y saveEffectParamsFlush() lo toma internamente, con debounce.
    saveEffectParamsFlush();
}

// ============================================================================
// WebSocket event handler
// ============================================================================

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
    {
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

        // Track client
        _wsAddClient(client->id(), client->remoteIP());

        // If in battery sleep or connecting, wake up immediately
        if (currentPowerState == POWER_BATTERY_SLEEP ||
            currentPowerState == POWER_BATTERY_CONNECTING) {
#ifdef DEBUG_POWER_MANAGEMENT
            debuglnD("🔔 Waking for WebSocket client");
#endif
            transitionToState(POWER_BATTERY_ACTIVE);
        }

        // Forzar envío completo (client list + log) incluso si dirty-flags
        // estaban limpios — el nuevo cliente necesita todo el historial.
        _lastSentClGen = 0xFE;
        _lastSentLogGen = 0xFE;
        stateGeneration++;

        // ⚠ Enviar mensajes con pequeño delay entre ellos para evitar
        //   desbordar el buffer TCP del cliente recién conectado.
        //   lwIP tiene un buffer de ~2-4KB; 3 ws.textAll() seguidos pueden
        //   saturarlo y causar desconexión intermitente del 2º cliente.

        // 1. yourClientId PRIMERO — mensaje pequeño, crítico para identidad
        {
            DynamicJsonDocument privMsg(64);
            privMsg["yourClientId"] = client->id();
            char privBuf[96];
            size_t privLen = serializeJson(privMsg, privBuf, sizeof(privBuf));
            if (privLen > 0 && privLen < sizeof(privBuf)) {
                client->text(privBuf, privLen);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));

        // 2. Full state broadcast to all clients
        notifyClients(true);
        vTaskDelay(pdMS_TO_TICKS(5));

        // 3. Client list + action log
        notifyWSClientList();
        vTaskDelay(pdMS_TO_TICKS(5));

        // 4. Battery history: solo al cliente nuevo, no a todos
        //    (los clientes existentes ya lo tienen, y si no pueden pedirlo
        //     con action: 'requestBattHistory')
        sendBatteryHistory(client);

        // Nota: notifyClients(true) ya se envió arriba como broadcast a todos.
        // Al ser el primer cliente nuevo, recibe el estado completo del master
        // y lo refleja al entrar. Si el master también está conectado, recibe
        // este mismo broadcast (estado de todos al entrar el slave).
        break;
    }
    case WS_EVT_DISCONNECT:
    {
#ifdef DEBUG_WEBSOCKET
        debugD("WebSocket client #");
        debugD_NUM(client->id(), "%u");
        debugD(" disconnected\n");
#endif
        // Remove from tracking BEFORE setting webSocketClientConnected
        _wsRemoveClient(client->id());
        clearPeekLiveState(client->id());

        webSocketClientConnected = (_wsClientCount > 0);
#ifdef DEBUG_POWER_MANAGEMENT
        debuglnD("❌ WebSocket client disconnected");
#endif

        // Notify remaining clients about the updated list
        notifyWSClientList();

        // Refresh completo (estado + efectos + params) para los clientes que
        // quedan conectados, incluido el master.
        // Esto es importante cuando el slave que se fue fue quien realizó la
        // última acción: el master debe reflejar ese efecto/color “ahora” y no
        // quedarse con un estado desactualizado hasta el próximo cambio. El log
        // histórico no se altera — solo se actualiza lo que consume la web.

        // If in battery active mode, go back to connecting (wait 30s)
        if (currentPowerState == POWER_BATTERY_ACTIVE &&
            _wsClientCount == 0) {
#ifdef DEBUG_POWER_MANAGEMENT
            debuglnD("💤 No clients left - waiting 30s for reconnect");
#endif
            transitionToState(POWER_BATTERY_CONNECTING);
        }
        break;
    }
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len, client->id(), client->remoteIP());
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
