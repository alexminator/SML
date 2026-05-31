#pragma once
#include "Effect.h"
#include "../state/AppState.h"

class RainbowBeatEffect : public Effect {
public:
    RainbowBeatEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        params.speed    = 30;  // rainbowSpeed default
        params.custom1  = 8;   // rainbowDelta default
    }

    void render() override {
        uint8_t bpm   = params.speed;
        uint8_t delta = params.custom1;

        uint16_t beatA = beatsin16(bpm, 0, 255);
        uint16_t beatB = beatsin16(bpm - 10, 0, 255);
        fill_rainbow(leds, N_PIXELS, (beatA + beatB) / 2, delta);

        FastLED.show();
    }
};
