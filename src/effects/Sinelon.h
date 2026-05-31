#pragma once
#include "Effect.h"
#include "../state/AppState.h"

class SinelonEffect : public Effect {
public:
    SinelonEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        params.speed    = 23;  // sinelonBeat default (bpm)
        params.custom1  = 2;   // sinelonFade default
    }

    void render() override {
        uint8_t bpm  = params.speed;
        uint8_t fade = params.custom1;

        fadeToBlackBy(leds, N_PIXELS, fade);
        int pos1 = beatsin16(bpm, 0, N_PIXELS - 1);
        int pos2 = beatsin16(bpm + 5, 0, N_PIXELS - 1);
        leds[(pos1 + pos2) / 2] += CHSV(myhue, 255, stripLed.brightness);

        EVERY_N_MILLISECONDS(10) {
            myhue++;
        }

        FastLED.show();
    }
};
