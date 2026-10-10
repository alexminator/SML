// ──────────────────────────────────────────────────────────────────────────────
// WebServer.h — Web server + WiFi + LittleFS
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;

void initLittleFS();
void initWiFi();
void initWebServer();
void initMDNS();
void onRootRequest(AsyncWebServerRequest *request);
