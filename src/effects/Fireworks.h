#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

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

        // Save previous spark colors BEFORE fade+blur (so we can restore them after)
        CRGB savedSparkA = (_sparkA < numLeds) ? leds[_sparkA] : CRGB(0, 0, 0);
        CRGB savedSparkB = (_sparkB < numLeds) ? leds[_sparkB] : CRGB(0, 0, 0);

        // Fade out existing
        for (unsigned i = 0; i < numLeds; i++) {
            leds[i].nscale8(128);
        }

        // Blur to spread sparks (this dims the spark centers)
        blur1d(leds, numLeds, 16);

        // Restore spark brightness at their original positions,
        // so the blur creates a glow around them instead of dimming them.
        // Skip on the very first frame (no valid sparks yet).
        if (!_first) {
            if (_sparkA < numLeds) leds[_sparkA] = savedSparkA;
            if (_sparkB < numLeds) leds[_sparkB] = savedSparkB;
        }
        _first = false;

        // Try to create new sparks
        for (int i = 0; i < max(1, (int)numLeds / 20); i++) {
            if (random8(129 - (params.intensity >> 1)) == 0) {
                uint16_t index = random16(numLeds);
                uint8_t hue = random8();
                leds[index] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hue, stripLed.brightness, LINEARBLEND);
                _sparkB = _sparkA;
                _sparkA = index;
            }
        }

        FastLED.show();
    }
};

const char FireworksEffect::_meta[] =
    "Fireworks@,Frequency,,,,,,,,;;;;sx=128,ix=192";
