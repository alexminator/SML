// ──────────────────────────────────────────────────────────────────────────────
// debug_config.h — UNIFIED DEBUG CONFIGURATION (single source of truth)
// ──────────────────────────────────────────────────────────────────────────────
// Edit THIS file to change debug verbosity or toggle categories.
// Every .cpp that needs debug output just does:
//
//     #include "config/debug_config.h"
//
// The underlying logging macros live in config/debug.h (included at the end
// of this file) — you shouldn't need to touch that file.
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

// ── Debug level ───────────────────────────────────────────────────────────────
// Corresponding macro families:
//   DEBUGLEVEL_NONE       (0) — nothing
//   DEBUGLEVEL_ERRORS     (1) — debugE / debuglnE
//   DEBUGLEVEL_WARNINGS   (2) — + debugW / debuglnW
//   DEBUGLEVEL_DEBUGGING  (3) — + debugD / debuglnD
//   DEBUGLEVEL_VERBOSE    (4) — + debugV / debuglnV
#define DEBUGLEVEL DEBUGLEVEL_DEBUGGING
// #define DEBUGLEVEL DEBUGLEVEL_NONE   // ← uncomment for release builds only

// ── Debug categories ──────────────────────────────────────────────────────────
// Uncomment a line to enable that category's messages.
#define DEBUG_SYSTEM              // System-level (init, mutex, errors)
#define DEBUG_WIFI                // WiFi connection and authentication
//#define DEBUG_WEB               // Web server and LittleFS filesystem
#define DEBUG_WEBSOCKET         // WebSocket communication and JSON
//#define DEBUG_LED               // LED strip control and effects
//#define DEBUG_BATTERY           // Battery monitoring and charging
//#define DEBUG_TEMPERATURE       // Temperature and humidity sensor (DHT)
#define DEBUG_NETWORK             // Network services (mDNS)
//#define DEBUG_BLUETOOTH         // Bluetooth module
#define DEBUG_POWER_MANAGEMENT    // Power Management State Machine

// ══════════════════════════════════════════════════════════════════════════════
// Include the logging engine — this gives you the actual debugE/debugD/… macros
// ══════════════════════════════════════════════════════════════════════════════
#include "debug.h"
