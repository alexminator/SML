#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// MeteorEffect — WLED mode_meteor (port completo)
// ──────────────────────────────────────────────────────────────────────────────
// Meteoros que caen con estela.
// Algoritmo WLED:
//   - fadeToBlackBy o degradado aleatorio de estela (según smooth mode)
//   - meteorSize = 1 + SEGLEN/20 (5%)
//   - Posición avanza con speed
//   - check3 = smooth mode (estela con gradiente suave)
//
// Parámetros:
//   speed     → Meteor speed (sx=128) — higher = faster
//   intensity → Trail fade (ix=128) — higher = estela más persistente
//   check3    → Smooth (m3=0) — estela con gradiente suave
// ──────────────────────────────────────────────────────────────────────────────

class MeteorEffect : public Effect {
private:
    static const char _meta[];
    uint16_t _meteorPos = 0;
    uint8_t _trail[24]; // max 24 LEDs

public:
    MeteorEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        memset(_trail, 0, sizeof(_trail));
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        // ── WLED mode_meteor ─────────────────────────────────────────────────
        const unsigned meteorSize = 1 + numLeds / 20;
        if (meteorSize < 1) return;

        // Posición del meteoro
        unsigned counter = millis() * ((params.speed >> 2) + 8);
        uint16_t meteorstart = (counter * numLeds) >> 16;

        const int maxBright = 240;

        // ── Fade all LEDs ───────────────────────────────────────────────────
        for (unsigned i = 0; i < numLeds; i++) {
            if (random8() <= 255 - params.intensity) {
                // Smooth trail: decay gradual por LED
                if (params.check3) {
                    if (_trail[i] > 0) {
                        int change = _trail[i] + 4 - random8(24);
                        _trail[i] = constrain(change, 0, maxBright);
                    }
                    uint8_t hue = uint8_t((i * 240 / numLeds) + (millis() >> 10));
                    leds[i] = CHSV(hue, 255, scale8(stripLed.brightness, _trail[i]));
                } else {
                    // Classic trail: scale random
                    _trail[i] = scale8(_trail[i], 128 + random8(127));
                    leds[i] = CHSV(64, 100, scale8(stripLed.brightness, _trail[i]));
                }
            }
        }

        // ── Dibujar cabeza del meteoro ──────────────────────────────────────
        for (unsigned j = 0; j < meteorSize && j < numLeds; j++) {
            int pos = (meteorstart + j) % numLeds;
            if (params.check3) {
                uint8_t hue = uint8_t((pos * 240 / numLeds) + (millis() >> 10));
                leds[pos] = CHSV(hue, 255, stripLed.brightness);
            } else {
                leds[pos] = CHSV(64, 200, stripLed.brightness);
            }
            _trail[pos] = maxBright; // Reset trail at meteor head
        }

        FastLED.show();
    }
};

const char MeteorEffect::_meta[] =
    "Meteor@Speed,Trail,,,,,,Smooth;;;;sx=128,ix=128,m3=0";
