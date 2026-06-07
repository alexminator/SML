#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// Fire2012Effect — WLED mode_fire_2012 (port completo)
// ──────────────────────────────────────────────────────────────────────────────
// Fuego realista con algoritmo de simulación celular (Mark Kriegsman).
// Algoritmo WLED:
//   1. Cool down: cada celda se enfría aleatoriamente
//   2. Heat drifts up: el calor sube y se difunde
//   3. Spark ignition: ignición aleatoria cerca de la base
//   4. Map to palette: calor → ColorFromPalette(HeatColors_p)
//
// Parámetros:
//   speed     → Cooling (sx=100) — higher = más enfriamiento (menos fuego)
//   intensity → Sparking (ix=100) — higher = más chispas
//   custom1   → Spark boost (c1=0) — intensidad extra de chispa
//   check1    → Reverse (m1=0) — fuego de arriba a abajo
// ──────────────────────────────────────────────────────────────────────────────

class Fire2012Effect : public Effect {
private:
    static const char _meta[];
    uint8_t _heat[N_PIXELS];
    CRGBPalette16 _palette;
    uint32_t _lastIt;     // frame counter for step 2/3 throttling

public:
    Fire2012Effect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        memset(_heat, 0, sizeof(_heat));
        _palette = HeatColors_p;
        _lastIt = 0;
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        const unsigned ignition = (numLeds < 10) ? 3 : numLeds / 10;

        // Frame counter — only re-diffuse & re-spark every ~32ms
        uint32_t it = millis() >> 5;

        // ── Step 1. Cool down (always runs, but less random on off-frames) ──
        for (unsigned i = 0; i < numLeds; i++) {
            uint8_t cool = (it != _lastIt)
                ? random8((((20 + params.speed / 3) * 16) / numLeds) + 2)
                : random8(4);
            uint8_t minTemp = (i < ignition) ? (ignition - i) / 4 + 16 : 0;
            uint8_t temp = qsub8(_heat[i], cool);
            _heat[i] = (temp < minTemp) ? minTemp : temp;
        }

        // Steps 2 & 3: only on frame change (saves CPU)
        if (it != _lastIt) {
            // ── Step 2. Heat drifts up ──────────────────────────────────────
            for (int k = numLeds - 1; k > 1; k--) {
                _heat[k] = (_heat[k - 1] + (_heat[k - 2] << 1)) / 3;
            }

            // ── Step 3. Spark ignition ──────────────────────────────────────
            if (random8() <= params.intensity) {
                uint8_t y = random8(ignition);
                uint8_t boost = (17 + params.custom1) * (ignition - y / 2) / ignition;
                _heat[y] = qadd8(_heat[y], random8(96 + 2 * boost, 207 + boost));
            }

            _lastIt = it;
        }

        // ── Step 4. Map heat to colors ──────────────────────────────────────
        for (unsigned j = 0; j < numLeds; j++) {
            uint8_t colorIndex = scale8(_heat[j], 240);
            CRGB color = ColorFromPalette(_palette, colorIndex, stripLed.brightness, NOBLEND);

            if (params.check1) {
                leds[numLeds - 1 - j] = color;
            } else {
                leds[j] = color;
            }
        }

        FastLED.show();
    }
};

const char Fire2012Effect::_meta[] =
    "Fire2012@Cooling,Sparking,Boost:0:30,,,,Reverse;;;;sx=100,ix=100,c1=0,m1=0";
