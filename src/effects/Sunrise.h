#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// Sunrise — Simulación de amanecer
// ──────────────────────────────────────────────────────────────────────────────
// Gradual de rojo cálido → naranja → amarillo → blanco brillante.
// Ideal como despertador o luz ambiental.
// Params:
//   speed     → velocidad del amanecer
//   intensity → brillo máximo
// ──────────────────────────────────────────────────────────────────────────────

class SunriseEffect : public Effect {
public:
    static const char _meta[];

    SunriseEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        // ── Continuous sunrise/sunset cycle via triangle wave ─────────────────
        //   counter = millis() * ((speed >> 2) + 2)
        //   triwave8(counter >> 8) → 0-254-0 continuous oscillation
        //   At speed=64:  period ≈ 3.6s
        //   At speed=255: period ≈ 1.0s
        unsigned counter = millis() * ((params.speed >> 2) + 2);
        uint8_t progress = triwave8(counter >> 8);  // 0-254 triangle wave

        // Palette index & brightness both driven by progress:
        //   progress=0   → first palette color, dim    (warm sunrise start)
        //   progress=254 → last palette color,  bright (bright daylight)
        // Then fades back down for sunset.
        CRGB color = ColorFromPalette(
            PaletteManager::getPalette(_paletteIndex),
            progress,
            progress,
            LINEARBLEND
        );
        fill_solid(leds, numLeds, color);

        FastLED.show();
    }
};

const char SunriseEffect::_meta[] =
    "Sunrise@Speed;;;;sx=64,ix=128,pa=24";
