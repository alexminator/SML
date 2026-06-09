#pragma once
#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"

// ──────────────────────────────────────────────────────────────────────────────
// NoisemeterVU — Medidor de ruido estilizado
// ──────────────────────────────────────────────────────────────────────────────
// VU meter clásico con colores dinámicos: verde (bajo), amarillo (medio),
// rojo (alto). Con pico (peak) flotante.
// Params:
//   speed     → tasa de decaimiento del pico
//   intensity → sensibilidad
// ──────────────────────────────────────────────────────────────────────────────

class NoisemeterVUEffect : public VUEffect {
public:
    static const char _meta[];

    NoisemeterVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        uint16_t height = auxReading();
        height = constrain(height, 0, numLeds - 1);

        fill_solid(leds, numLeds, CRGB::Black);

        // Draw bars with color gradient: green → yellow → red
        uint8_t greenZone  = numLeds * 2 / 3;
        uint8_t yellowZone = numLeds * 5 / 6;

        for (uint16_t i = 0; i <= height; i++) {
            if (i < greenZone) {
                leds[i] = CHSV(96, 255, 255);    // Green
            } else if (i < yellowZone) {
                leds[i] = CHSV(32, 255, 255);    // Yellow
            } else {
                leds[i] = CHSV(0, 255, 255);     // Red
            }
        }

        // Peak dot with decay
        if (height > _peak) _peak = height;
        if (_peak > 0 && _peak < numLeds) {
            leds[_peak] = CRGB::White;
        }

        dropPeak();
        averageReadings();
        FastLED.show();
    }
};

const char NoisemeterVUEffect::_meta[] =
    "Noisemeter@Decay,Sensitivity;;;;sx=128,ix=128";
