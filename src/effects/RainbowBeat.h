#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// RainbowBeatEffect — Arcoíris pulsante con beatsin16 + fill_rainbow
// ──────────────────────────────────────────────────────────────────────────────
// Usa dos ondas seno a diferentes frecuencias (beatsin16) para generar un
// arcoíris que pulsa y se desplaza rítmicamente.
//
// Algoritmo:
//   beatA = beatsin16(bpmA, 0, 255)
//   beatB = beatsin16(bpmB, 0, 255)
//   startHue = (beatA + beatB) / 2
//   fill_rainbow(leds, N, startHue, delta)
//
// Parámetros:
//   speed     → BPM base (sx=100) — higher = beats más rápidos
//   intensity → Delta hue (ix=128) — higher = más dispersión
// ──────────────────────────────────────────────────────────────────────────────

class RainbowBeatEffect : public Effect {
private:
    static const char _meta[];

public:
    RainbowBeatEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        // speed → BPM (10-70), dos frecuencias con ratio 3:2
        uint8_t bpmA = 10 + (params.speed * 60 / 255);
        uint8_t bpmB = bpmA * 2 / 3;
        if (bpmB < 5) bpmB = 5;

        uint16_t beatA = beatsin16(bpmA, 0, 255);
        uint16_t beatB = beatsin16(bpmB, 0, 255);

        // intensity → delta hue (2-32)
        uint8_t delta = 2 + (params.intensity * 30 / 255);

        uint8_t startHue = (beatA + beatB) / 2;

        // Reemplazar fill_rainbow con palette seleccionable
        const CRGBPalette16& pal = PaletteManager::getPalette(_paletteIndex);
        for (unsigned i = 0; i < numLeds; i++) {
            leds[i] = ColorFromPalette(pal, startHue + i * delta, stripLed.brightness, LINEARBLEND);
        }

        FastLED.show();
    }
};

// Metadata: speed (BPM), intensity (delta hue)
const char RainbowBeatEffect::_meta[] =
    "RainbowBeat@BPM,Spread,,,,,,,,;;;;sx=100,ix=128";
