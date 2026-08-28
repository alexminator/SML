#pragma once

// WiFi credentials —来源优先级:
//   1. Build flags (-DDEFAULT_WIFI_SSID=...) desde .env / platformio.ini
//   2. config/secrets.h (credenciales por defecto)
// NO definir defaults aquí — si secrets.h no se incluyó antes,
// data.cpp se encarga de incluirlo.

// Solo declaraciones — definiciones en data.cpp
extern const char *WIFI_SSID;
extern const char *WIFI_PASS;
extern const char *WEB_NAME;

