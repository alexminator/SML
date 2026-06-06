#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// JuggleEffect — WLED Juggle adapted for SML
// ──────────────────────────────────────────────────────────────────────────────
// Parameters (from metadata):
//   speed     → Bounce speed (sx=20) — 0-40 BPM
//   intensity → Trail fade rate (ix=128) — higher = shorter trail
//   custom1   → Number of dots (c1=3) — max 5 para 24 LEDs
// ──────────────────────────────────────────────────────────────────────────────

class JuggleEffect : public Effect {
private:
    static const char _meta[];

public:
    JuggleEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        // Intensity → trail fade rate (calibrated for 20ms / 50fps)
        // 3=trail ~3s (muy visible), 100=trail ~100ms (snappy)
        uint8_t fader = map(params.intensity, 0, 255, 3, 100);
        fadeToBlackBy(leds, numLeds, fader);

        // Max dots: 5 para 24 LEDs (más satura el efecto visual)
        uint8_t maxDots = min((uint8_t)5, max((uint8_t)2, (uint8_t)(numLeds / 4)));
        uint8_t numDots = constrain(params.custom1, 1, maxDots);

        // Speed: BPM 1-40 (rango útil visualmente para 24 LEDs)
        uint8_t bpm = max((uint8_t)1, params.speed);

        // Phase offset por dot: cada uno arranca en distinta posición del ciclo
        uint16_t phaseStep = 65536 / maxDots;

        byte dothue = 0;
        for (int i = 0; i < numDots; i++) {
            unsigned pos = beatsin16(bpm, 0, numLeds - 1, 0, i * phaseStep);
            leds[pos] = CHSV(dothue, 220, stripLed.brightness);
            dothue += 32;
        }

        FastLED.show();
    }
};

// Metadata: "Name@labels;defaults"
// Labels: speed, intensity, custom1, custom2, custom3, (reserved), check1, check2, check3
const char JuggleEffect::_meta[] =
    "Juggle@Speed:0:40,Trail,Dots:1:5,,,,,,,,;;;;sx=20,ix=128,c1=3";
