#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// FadeEffect — WLED mode_fade (port completo)
// ──────────────────────────────────────────────────────────────────────────────
// Fundido suave entre colores usando triwave16.
// Algoritmo WLED:
//   counter = strip.now * ((speed >> 3) + 10)
//   lum = triwave16(counter) >> 8
//   color = blend(color1, color_from_palette(), lum)
//
// Parámetros:
//   speed → Fade speed (sx=128) — higher = faster fade
// ──────────────────────────────────────────────────────────────────────────────

class FadeEffect : public Effect {
private:
    static const char _meta[];

public:
    FadeEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        // ── WLED triwave16 fade ──────────────────────────────────────────────
        unsigned counter = millis() * ((params.speed >> 3) + 10);
        uint8_t lum = triwave8(counter >> 8);

        // Color destino = hue que evoluciona lentamente
        uint8_t hueTarget = uint8_t(millis() >> 10);

        for (unsigned i = 0; i < numLeds; i++) {
            // Color base = negro (fondo)
            CRGB color1 = CRGB::Black;
            // Color palette = basado en hue + posición
            uint8_t hue = hueTarget + (i * 256 / numLeds);
            CRGB color2 = CHSV(hue, 255, stripLed.brightness);
            // Blend según lum
            leds[i] = blend(color1, color2, lum);
        }

        FastLED.show();
    }
};

const char FadeEffect::_meta[] =
    "Fade@Speed;;;;;;;;sx=128";
