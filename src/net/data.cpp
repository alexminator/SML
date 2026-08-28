// ──────────────────────────────────────────────────────────────────────────────
// data.cpp — Definiciones reales de credenciales WiFi
// ──────────────────────────────────────────────────────────────────────────────
// Los extern correspondientes están en data.h.
// Orden de includes:
//   1. secrets.h primero — define DEFAULT_WIFI_SSID/PASS si no vienen de build flags
//   2. data.h después  — declara los extern (sin definir defaults)
// ──────────────────────────────────────────────────────────────────────────────
#include "../config/secrets.h"  // FIRST — defines DEFAULT_WIFI_SSID/PASS
#include "data.h"

const char *WIFI_SSID = DEFAULT_WIFI_SSID;
const char *WIFI_PASS = DEFAULT_WIFI_PASS;
const char *WEB_NAME  = "sml";
