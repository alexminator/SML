// ──────────────────────────────────────────────────────────────────────────────
// data.cpp — Definiciones reales de credenciales WiFi
// ──────────────────────────────────────────────────────────────────────────────
// Los extern correspondientes están en data.h.
// Las credenciales vienen de config/secrets.h (vía build flags o .env).
// ──────────────────────────────────────────────────────────────────────────────
#include "data.h"
#include "../config/secrets.h"

const char *WIFI_SSID = DEFAULT_WIFI_SSID;
const char *WIFI_PASS = DEFAULT_WIFI_PASS;
const char *WEB_NAME  = "sml";
