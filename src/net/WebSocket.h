// ──────────────────────────────────────────────────────────────────────────────
// WebSocket.h — WebSocket server
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include <ESPAsyncWebServer.h>

extern AsyncWebSocket ws;

void initWebSocket();
void notifyClients(bool includeParams = true);
void notifySensorData();
void notifyWSClientList();

// ── Peek live view (WLED binary frame) ────────────────────────────────────────
// El preview tiene un único cliente suscriptor. Se captura el frame con el mutex
// tomado y se envía unicast después de soltarlo; el envío se omite si ese cliente
// no puede aceptar otro mensaje WebSocket.

/// Snapshot del frame Peek. DEBE llamarse con dataMutex tomado.
/// Retorna true si hay frame listo y escribe su longitud y destinatario.
bool buildPeekFrame(uint8_t *buf, size_t bufSize, size_t *len, uint32_t *clientId);

/// Envía un frame al suscriptor concreto, SIN dataMutex.
void sendPeekFrame(uint32_t clientId, const uint8_t *buf, size_t len);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, uint32_t clientId, IPAddress clientIp);
