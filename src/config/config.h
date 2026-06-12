// ──────────────────────────────────────────────────────────────────────────────
// System Configuration — Smart Music Lamp (ESP32)
// ──────────────────────────────────────────────────────────────────────────────
// Extraído de Settings.h durante la refactorización Fase 2
// Valores de comportamiento, timings y constantes del sistema.
// Los #define se usan donde se necesita una constante de compile-time
// (tamaño de arrays, etc.). Los constexpr se usan para el resto.
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

// ============================================================================
// LED STRIP (NEOPIXEL)
// ============================================================================
#define N_PIXELS        24
#define VOLTS           5
#define MAX_MILLIAMPS   500
#define COLOR_ORDER     GRB
#define LED_TYPE        WS2812B

// ============================================================================
// DHT SENSOR
// ============================================================================
#define DHTTYPE         DHT22

// ============================================================================
// BATTERY MONITORING (TP4056)
// ============================================================================
#define CONV_FACTOR     1.702
#define READS           30
#define MAXV            4.00
#define MINV            3.20
#define BATT_THRESHOLD  30
#define MAX_READS       10
#define FULL_READS      10

// ============================================================================
// VU METER (AUDIO)
// ============================================================================
#define DC_OFFSET       0
#define NOISE           30
#define SAMPLES         60
#define TOP             (N_PIXELS + 2)
#define PEAK_FALL       20
#define N_PIXELS_HALF   (N_PIXELS / 2)
#define BIAS            1850

// ============================================================================
// EFECTO BALLS — constantes físicas
// ============================================================================
#define GRAVITY         (-1)
#define h0              1
#define NUM_BALLS       3

// ============================================================================
// STACK MONITORING
// ============================================================================
constexpr uint32_t STACK_WARNING_THRESHOLD  = 256;
constexpr uint32_t STACK_CRITICAL_THRESHOLD = 128;

// ============================================================================
// WIFI CONNECTION
// ============================================================================
constexpr int           WIFI_MAX_ATTEMPTS    = 40;
constexpr unsigned long WIFI_RETRY_DELAY     = 500;
constexpr unsigned long WIFI_MONITOR_INTERVAL = 2000;

// ============================================================================
// TIMING
// ============================================================================
constexpr unsigned long LED_ERROR_FLASH_CYCLE = 200;
constexpr unsigned long LED_ERROR_FLASH_ON    = 50;
constexpr unsigned long BATTERY_CHECK_INTERVAL = 3000;
constexpr unsigned long LITTLEFS_TIMEOUT       = 30000;
constexpr unsigned long WEBSOCKET_UPDATE_INTERVAL = 3000;
constexpr unsigned long WEBSOCKET_SENSOR_INTERVAL = 3000;
constexpr uint8_t       WEBSOCKET_STACK_CHECK_CYCLES = 10;
constexpr unsigned long SENSOR_CHECK_INTERVAL  = 5000;

// ============================================================================
// BLUETOOTH BUTTON EMULATION — duraciones de pulsación
// ============================================================================
constexpr unsigned long long_delay  = 1000;  // Volumen +/-
constexpr unsigned long short_delay = 200;   // FF, RW, Play/Pause

// ============================================================================
// POWER MANAGEMENT STATE MACHINE
// ============================================================================
constexpr unsigned long SLEEP_DURATION        = 60000;
constexpr unsigned long AWAKE_DURATION        = 10000;
constexpr unsigned long POWER_CHANGE_DEBOUNCE = 3000;
constexpr unsigned long WS_WAIT_DURATION      = 30000;
constexpr int           BATTERY_CRITICAL_LEVEL = 15;

// ============================================================================
// BATTERY LOGGING (LittleFS persistence)
// ============================================================================
constexpr int BATT_LOG_SIZE          = 200;    // Max entries in circular buffer
constexpr int BATT_LOG_SAVE_INTERVAL = 5;      // Save to LittleFS every N writes
constexpr float BATT_LOG_DELTA       = 0.10f;  // Min voltage change to log (Volts) — 100mV filtra ruido ADC
constexpr int BATT_LOG_TIMEOUT       = 60;     // Force entry every 60s even if V stable

// ============================================================================
// VERSION STRING (mostrado en Config tab)
// ============================================================================
#define SML_VERSION "2.0.0"
