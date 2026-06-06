#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// RunningLightsEffect — WLED mode_running_lights (port completo)
// ──────────────────────────────────────────────────────────────────────────────
// Ondas sinusoidales tipo "luces corriendo".
// Algoritmo WLED running_base(false):
//   x_scale = intensity >> 2
//   counter = (strip.now * speed) >> 9
//   a = i*x_scale - counter
//   s = sin8_t(a)
//   color = blend(secundario, palette_color, s)
//
// Parámetros:
//   speed     → Wave speed (sx=128) — higher = faster
//   intensity → Wave width (ix=128) — higher = más estrecho
// ──────────────────────────────────────────────────────────────────────────────

class RunningLightsEffect : public Effect {
private:
    static const char _meta[];

public:
    RunningLightsEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        // ── WLED running_base(false) ─────────────────────────────────────────
        unsigned x_scale = params.intensity >> 2;
        uint32_t counter = (millis() * params.speed) >> 9;

        for (unsigned i = 0; i < numLeds; i++) {
            unsigned a = i * x_scale - counter;
            uint8_t s = sin8(a);  // FastLED sin8 (0-255)

            // Color base = negro (fondo)
            CRGB colorBase = CRGB::Black;
            // Color palette = hue que evoluciona
            uint8_t hue = uint8_t((millis() >> 9) + i * 8);
            CRGB colorPal = CHSV(hue, 200, stripLed.brightness);

            // Blend según seno
            leds[i] = blend(colorBase, colorPal, s);
        }

        FastLED.show();
    }
};

const char RunningLightsEffect::_meta[] =
    "Running@Speed,Wave width;;;;;;;sx=128,ix=128";
