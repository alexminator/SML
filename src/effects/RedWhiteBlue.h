#pragma once
#include "Effect.h"
#include "../Settings.h"

class RedWhiteBlueEffect : public Effect {
public:
    RedWhiteBlueEffect(CRGB* l, uint16_t n) : Effect(l, n) {}
    void render() override {
        uint16_t sinBeat   = beatsin16(30, 0, N_PIXELS - 1, 0, 0);
        uint16_t sinBeat2  = beatsin16(30, 0, N_PIXELS - 1, 0, 21845);
        uint16_t sinBeat3  = beatsin16(30, 0, N_PIXELS - 1, 0, 43690);

        leds[sinBeat]   = CHSV(160,255,stripLed.brightness); //blue
        leds[sinBeat2]   = CHSV(0,255,stripLed.brightness); //red
        leds[sinBeat3]   = CHSV(0,0,stripLed.brightness); //white

        fadeToBlackBy(leds, N_PIXELS, 10);

        FastLED.show();
    }
};
