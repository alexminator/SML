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
private:
    uint8_t _progress = 0;  // 0-255 = sunrise progress

public:
    static const char _meta[];

    SunriseEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        uint8_t speed = map(params.speed, 0, 255, 1, 20);
        _progress = qadd8(_progress, speed);

        // Use palette for color, _progress for palette position and brightness fade-in
        CRGB color = ColorFromPalette(
            PaletteManager::getPalette(_paletteIndex),
            _progress,
            _progress,
            LINEARBLEND
        );
        fill_solid(leds, numLeds, color);

        FastLED.show();
    }
};

const char SunriseEffect::_meta[] =
    "Sunrise@Speed;;;;sx=64,ix=128,pa=24";
