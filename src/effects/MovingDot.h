#pragma once
#include "Effect.h"
#include "../Settings.h"

class MovingDotEffect : public Effect {
public:
    MovingDotEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        params.speed    = 30;  // bpm for main beatsin
        params.custom1  = 10;  // fade amount
    }

    void render() override {
        uint8_t bpm  = params.speed;
        uint8_t fade = params.custom1;

        uint16_t posBeat  = beatsin16(bpm, 0, N_PIXELS - 1, 0, 0);
        uint16_t posBeat2 = beatsin16(bpm * 2, 0, N_PIXELS - 1, 0, 0);

        uint16_t posBeat3 = beatsin16(bpm, 0, N_PIXELS - 1, 0, 32767);
        uint16_t posBeat4 = beatsin16(bpm * 2, 0, N_PIXELS - 1, 0, 32767);

        // Wave for LED color
        uint8_t colBeat  = beatsin8(45, 0, 255, 0, 0);

        leds[(posBeat + posBeat2) / 2]  = CHSV(colBeat, 255, stripLed.brightness);
        leds[(posBeat3 + posBeat4) / 2]  = CHSV(colBeat, 255, stripLed.brightness);

        fadeToBlackBy(leds, N_PIXELS, fade);

        FastLED.show();
    }
};
