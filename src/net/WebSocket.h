// ──────────────────────────────────────────────────────────────────────────────
// WebSocket.h — WebSocket server
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include <ESPAsyncWebServer.h>

extern AsyncWebSocket ws;

void initWebSocket();
void notifyClients();
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
const char* bars();
