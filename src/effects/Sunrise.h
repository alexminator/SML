#pragma once
#include "Effect.h"
#include "../state/AppState.h"

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
        uint8_t maxBright = 255;  // Brillo fijo (el global stripLed.brightness controla el brillo)

        _progress = qadd8(_progress, speed);

        // Sunrise color gradient: deep red → orange → yellow → white
        uint8_t hue, sat, val;
        if (_progress < 64) {
            // Deep red → orange
            hue = 0 + (_progress * 10 / 64);     // 0-10
            sat = 255 - (_progress * 2);          // 255-127
            val = _progress * maxBright / 64;     // 0-maxBright
        } else if (_progress < 128) {
            // Orange → warm yellow
            uint8_t stage = _progress - 64;
            hue = 10 + (stage * 20 / 64);         // 10-30
            sat = 127 - (stage * 127 / 64);        // 127-0
            val = maxBright;
        } else if (_progress < 192) {
            // Warm yellow → pale yellow
            uint8_t stage = _progress - 128;
            hue = 30 + (stage * 20 / 64);         // 30-50
            sat = 0;
            val = maxBright;
        } else {
            // Pale yellow → bright white
            hue = 50;
            sat = 0;
            val = maxBright;
        }

        CRGB color = CHSV(hue, sat, val);
        fill_solid(leds, numLeds, color);

        FastLED.show();
    }
};

const char SunriseEffect::_meta[] =
    "Sunrise@Speed;;;;sx=64,ix=128";
