// ──────────────────────────────────────────────────────────────────────────────
// GPIO Pin Assignments — Smart Music Lamp (ESP32)
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

// LED Strip (Neopixel)
#define STRIP_PIN       4

// Onboard LED
#define ONBOARD_LED_PIN LED_BUILTIN

// DHT22 Sensor
#define DHTPIN          23

// Bluetooth Module Relay
#define SWITCH_PIN      18

// Bluetooth Button Emulation (MOSFETs)
#define VOLUMENUP_PIN   5
#define VOLUMENDOWN_PIN 19
#define PLAY_PIN        21

// Lamp Relay
#define LAMP_PIN        32

// Battery Monitoring — TP4056
#define CHARGE_PIN      34
#define FULL_CHARGE_PIN 35
#define ADC_PIN         33

// Audio Input (VU Meter)
#define AUDIO_IN_PIN    36
