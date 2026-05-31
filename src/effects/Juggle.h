#pragma once
#include "Effect.h"
#include "../state/AppState.h"

class JuggleEffect : public Effect {
public:
    JuggleEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        params.custom1   = 4;   // juggleDots default
        params.speed     = 64;  // juggleSpeed default
        params.intensity = 128; // juggleIntensity default
    }

    void render() override {
        uint8_t dots      = params.custom1;
        uint8_t speedVal  = params.speed;
        uint8_t intensity = params.intensity;

        uint8_t fadeAmount = 192 - (3 * intensity / 4);
        fadeToBlackBy(leds, N_PIXELS, fadeAmount);

        byte dothue = 0;
        for (int i = 0; i < dots; i++) {
            int index = beatsin16((16 + speedVal) * (i + 7), 0, N_PIXELS - 1);
            leds[index] |= CHSV(dothue, 220, stripLed.brightness);
            dothue += 32;
        }

        FastLED.show();
    }
};
