#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// FireEffect — WLED Fire 2012 algorithm adapted for SML
// ──────────────────────────────────────────────────────────────────────────────
// Parameters (from metadata):
//   speed     → Cooling rate   (sx=64)  — higher = faster cooling
//   intensity → Sparking chance (ix=160) — higher = more sparks
//   custom3   → Boost           (c3=16)  — spark intensity multiplier
//   check1    → Reverse                   — flip direction
//
// All parameters are auto-scaled to N_PIXELS (ignition area = 10% of strip).
// ──────────────────────────────────────────────────────────────────────────────

class FireEffect : public Effect {
private:
    byte _heat[N_PIXELS];
    uint32_t _lastIt;     // frame counter for step 2/3 throttling

public:
    static const char _meta[];

    FireEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        memset(_heat, 0, sizeof(_heat));
        _lastIt = 0;
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        const uint8_t cooling  = params.speed;
        const uint8_t sparking = params.intensity;
        const uint8_t boost    = params.custom3;
        const bool reverse     = params.check1;

        // Ignition zone: bottom 10% of strip (min 3 pixels)
        const uint8_t ignition = (numLeds < 30) ? 3 : (numLeds / 10);

        // Frame counter — only re-diffuse & re-spark every ~32ms
        uint32_t it = millis() >> 5;

        // Step 1. Cool down every cell a little
        for (unsigned i = 0; i < numLeds; i++) {
            // Scale cooling to strip length so it behaves the same on any N_PIXELS
            uint8_t cool = (it != _lastIt)
                ? random8((((20 + cooling / 3) * 16) / numLeds) + 2)
                : random8(4);
            // Ignition zone must never go fully black
            uint8_t minTemp = (i < ignition) ? (ignition - i) / 4 + 16 : 0;
            uint8_t temp = qsub8(_heat[i], cool);
            _heat[i] = (temp < minTemp) ? minTemp : temp;
        }

        // Steps 2 & 3: only on frame change (saves CPU)
        if (it != _lastIt) {
            // Step 2. Heat drifts 'up' and diffuses
            for (int k = numLeds - 1; k >= 2; k--) {
                _heat[k] = (_heat[k - 1] + (_heat[k - 2] << 1)) / 3;
            }

            // Step 3. Randomly ignite new sparks near the bottom
            if (random8() <= sparking) {
                uint8_t y = random8(ignition);
                // Boost makes sparks brighter (especially near the bottom)
                uint8_t sparkBoost = (17 + boost) * (ignition - y / 2) / ignition;
                _heat[y] = qadd8(_heat[y], random8(96 + sparkBoost * 2, 207 + sparkBoost));
            }

            _lastIt = it;
        }

        // Step 4. Map heat to LED colors using a black→red→yellow→white palette
        for (unsigned j = 0; j < numLeds; j++) {
            byte colorindex = scale8(_heat[j], 240);
            CRGB color = ColorFromPalette(
                CRGBPalette16(CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White),
                colorindex
            );
            unsigned p = reverse ? (numLeds - 1 - j) : j;
            leds[p] = color;
        }

        FastLED.show();
    }
};

// Metadata: "Name@labels;...;...;...;defaults"
// Labels: speed, intensity, custom1, custom2, custom3, (reserved), check1, check2, check3
const char FireEffect::_meta[] =
    "Fire@Cooling,Sparking,,,Boost,,Reverse,,;;;;sx=64,ix=160,c3=16,m1=0";
