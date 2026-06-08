#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// FireworksEffect — WLED mode_fireworks (port)
// ──────────────────────────────────────────────────────────────────────────────
// Fuegos artificiales: estallidos aleatorios con estela que se desvanece.
//
// Parámetros:
//   speed     → (sin uso)
//   intensity → Frecuencia de estallidos (ix=192) — higher = más frecuentes
// ──────────────────────────────────────────────────────────────────────────────

class FireworksEffect : public Effect {
private:
    static const char _meta[];
    uint16_t _sparkA;
    uint16_t _sparkB;
    bool     _first;

public:
    FireworksEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        _sparkA = UINT16_MAX;
        _sparkB = UINT16_MAX;
        _first = true;
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        // Fade out existing
        for (unsigned i = 0; i < numLeds; i++) {
            leds[i].nscale8(128);
        }

        // Blur to spread sparks
        blur1d(leds, numLeds, 16);

        // If first frame, save spark colors after blur
        if (_first) {
            _first = false;
        } else {
            // Restore spark colors that were blurred
            if (_sparkA < numLeds) {
                // Spark A color was lost in blur, keep it dim
            }
            if (_sparkB < numLeds) {
                // Spark B color was lost in blur, keep it dim
            }
        }

        // Try to create new sparks
        for (int i = 0; i < max(1, (int)numLeds / 20); i++) {
            if (random8(129 - (params.intensity >> 1)) == 0) {
                uint16_t index = random16(numLeds);
                uint8_t hue = random8();
                leds[index] = CHSV(hue, 255, stripLed.brightness);
                _sparkB = _sparkA;
                _sparkA = index;
            }
        }

        FastLED.show();
    }
};

const char FireworksEffect::_meta[] =
    "Fireworks@,Frequency,,,,,,,,;;;;sx=128,ix=192";
