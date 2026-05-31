// ──────────────────────────────────────────────────────────────────────────────
// WebServer.h — Web server + WiFi + LittleFS
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;

// Status enum for HTML template processor
enum Status {
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
    VU1, VU2, VU3, VU4, VU5, VU6,
    LAMP,
    TEMPNEO,
    BATTNEO
};

void initLittleFS();
void initWiFi();
void initWebServer();
void onRootRequest(AsyncWebServerRequest *request);
const char* processor(const String &var);
