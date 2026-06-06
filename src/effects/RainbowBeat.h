#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// RainbowBeatEffect — WLED mode_rainbow_cycle adaptado para SML
// ──────────────────────────────────────────────────────────────────────────────
// Arcoíris continuo que cicla a través de toda la tira.
// Algoritmo WLED: cada LED toma un color del color_wheel (HSV rotado) según su
// posición, con un offset temporal que hace que el arcoíris se mueva.
//
// Parámetros:
//   speed     → Rainbow rotation speed (sx=128) — higher = faster cycle
//   intensity → Color spread (ix=128) — 0 = solid color, 255 = full rainbow
// ──────────────────────────────────────────────────────────────────────────────

class RainbowBeatEffect : public Effect {
private:
    static const char _meta[];

public:
    RainbowBeatEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        // ── WLED mode_rainbow_cycle ─────────────────────────────────────────
        // counter: moving phase from 0-255, speed-controlled
        unsigned counter = (millis() * ((params.speed >> 2) + 2)) & 0xFFFF;
        counter = counter >> 8;  // 0-255

        // intensity: spread of colors
        // 0 = solid (all same hue), 255 = full rainbow spectrum
        uint8_t spread = 16 << (params.intensity / 29);  // 16-256+

        for (unsigned i = 0; i < numLeds; i++) {
            // WLED: index = (i * spread / SEGLEN) + counter
            uint8_t index = (i * spread / numLeds) + counter;
            leds[i] = CHSV(index, 255, stripLed.brightness);
        }

        FastLED.show();
    }
};

// Metadata: speed (rotation), intensity (spread)
const char RainbowBeatEffect::_meta[] =
    "Rainbow@Speed,Spread,,,,,,,,;;;;sx=128,ix=128";
