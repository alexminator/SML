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
void sendPeekData();
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, uint32_t clientId, IPAddress clientIp);
