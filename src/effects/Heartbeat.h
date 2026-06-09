#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// Heartbeat — Latido cardíaco pulsante
// ──────────────────────────────────────────────────────────────────────────────
// Simula el pulso cardiaco: dos latidos rápidos (lub-dub) seguidos de pausa.
// Params:
//   speed     → frecuencia cardíaca (BPM)
//   intensity → brillo máximo del latido
// ──────────────────────────────────────────────────────────────────────────────

class HeartbeatEffect : public Effect {
private:
    uint8_t _phase = 0;

public:
    static const char _meta[];

    HeartbeatEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        uint8_t bpm = map(params.speed, 0, 255, 40, 140);  // 40-140 BPM
        uint8_t peak = 255;  // Brillo fijo (el global stripLed.brightness controla el brillo)

        // Two-part heartbeat: lub (strong) + dub (weaker)
        uint16_t period = (60000 / bpm);           // ms per beat
        uint16_t lubMs   = period * 0 / 8;         // lub at 0%
        uint16_t dubMs   = period * 2 / 8;         // dub at 25%
        uint16_t cycleMs = period;                  // cycle length

        uint16_t now = millis() % cycleMs;

        uint8_t brightness = 0;
        if (now < period / 8) {
            // Lub - quick strong pulse
            float t = (float)now / (float)(period / 8);
            brightness = t < 0.5f ? (t * 2.0f) : (2.0f - t * 2.0f);
            brightness = (uint8_t)(brightness * peak * 1.2f);
        } else if (now >= period * 2 / 8 && now < period * 3 / 8) {
            // Dub - quick weaker pulse
            float t = (float)(now - period * 2 / 8) / (float)(period / 8);
            brightness = t < 0.5f ? (t * 2.0f) : (2.0f - t * 2.0f);
            brightness = (uint8_t)(brightness * peak * 0.8f);
        }

        brightness = constrain(brightness, 0, 255);
        CRGB color = CHSV(0, 0, brightness);  // White-to-red pulse
        fill_solid(leds, numLeds, color);

        // Red tint on pulse
        if (brightness > 50) {
            CRGB heartColor = CHSV(0, 255, brightness);
            fill_solid(leds, numLeds, heartColor);
        }

        FastLED.show();
    }
};

const char HeartbeatEffect::_meta[] =
    "Heartbeat@BPM;;;;sx=128,ix=128";
