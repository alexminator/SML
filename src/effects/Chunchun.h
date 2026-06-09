#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// Chunchun — Pájaros de fuego (WLED Chunchun)
// ──────────────────────────────────────────────────────────────────────────────
// Múltiples "pájaros" que vuelan sinusoidalmente a lo largo de la tira,
// como fuegos artificiales o bengalas.
// Params:
//   speed     → velocidad de vuelo
//   intensity → separación entre pájaros
// ──────────────────────────────────────────────────────────────────────────────

class ChunchunEffect : public Effect {
public:
    static const char _meta[];

    ChunchunEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        fadeToBlackBy(leds, numLeds, 254);  // suave estela

        uint8_t numBirds = 2 + (numLeds >> 3);      // 2 + 1/8 de la tira
        uint16_t span = (params.intensity << 8) / numBirds;

        for (uint8_t i = 0; i < numBirds; i++) {
            uint16_t counter = (millis() * (6 + (params.speed >> 4)) + i * span);
            int16_t sinVal = sin16(counter);
            uint16_t megumin = (uint16_t)sinVal + 0x8000U; // 0-65535 range
            uint16_t bird = (uint32_t(megumin) * numLeds) >> 16;
            bird = constrain(bird, 0U, numLeds - 1);

            leds[bird] += ColorFromPalette(
                RainbowColors_p,
                (i * 255) / numBirds,
                255,
                LINEARBLEND
            );
        }

        FastLED.show();
    }
};

const char ChunchunEffect::_meta[] =
    "Chunchun@Speed,Gap;;;;sx=128,ix=128";
