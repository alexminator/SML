#pragma once
#include "Effect.h"
#include "Settings.h"
#include "effects/utils.h"

class TemperatureEffect : public Effect {
public:
    TemperatureEffect(CRGB* l, uint16_t n) : Effect(l, n) {}
    void render() override {
        FastLED.clear();
        int t = map(temp, 17, 40, 0, N_PIXELS);

        for (uint16_t L = 0; L < t; L++) {
            leds[L] = wheel(((205 + (L * 6)) & 255));
        }
        FastLED.show();
    }
};
