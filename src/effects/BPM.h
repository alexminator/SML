#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// BPMEffect — WLED mode_bpm (port)
// ──────────────────────────────────────────────────────────────────────────────
// Arcoíris pulsante al ritmo de beatsin. El color se desplaza y el brillo
// pulsa siguiendo una onda seno a la frecuencia configurada.
//
// Parámetros:
//   speed     → BPM (sx=64)
//   intensity → (sin uso)
// ──────────────────────────────────────────────────────────────────────────────

class BPMEffect : public Effect {
private:
    static const char _meta[];

public:
    BPMEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        uint8_t stp = (millis() / 20) & 0xFF;
        uint8_t beat = beatsin8(params.speed, 64, 255);

        for (unsigned i = 0; i < numLeds; i++) {
            uint8_t hue = stp + (i * 2);
            int16_t bri = (int16_t)beat - (int16_t)stp + (int16_t)(i * 10);
            bri = constrain(bri, 0, 255);
            leds[i] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hue, scale8((uint8_t)bri, stripLed.brightness), LINEARBLEND);
        }

        FastLED.show();
    }
};

const char BPMEffect::_meta[] =
    "BPM@BPM,,,,,,,,;;;;sx=64";
