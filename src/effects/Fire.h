#pragma once
#include "Effect.h"
#include "../state/AppState.h"

class FireEffect : public Effect {
private:
    // Temperature readings at each simulation cell — member instead of static local
    byte _heat[N_PIXELS];

public:
    FireEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        params.custom1 = 55;   // fireCooling default
        params.custom2 = 50;   // fireSparking default
        params.check1  = false; // fireReverse default
        memset(_heat, 0, sizeof(_heat));
    }

    void render() override {
        uint8_t cooling  = params.custom1;
        uint8_t sparking = params.custom2;
        bool reverse     = params.check1;

        // Step 1.  Cool down every cell a little
        for (int i = 0; i < N_PIXELS; i++) {
            _heat[i] = qsub8(_heat[i], random8(0, ((cooling * 10) / N_PIXELS) + 2));
        }

        // Step 2.  Heat from each cell drifts 'up' and diffuses a little
        for (int k = N_PIXELS - 1; k >= 2; k--) {
            _heat[k] = (_heat[k - 1] + _heat[k - 2] + _heat[k - 2]) / 3;
        }

        // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
        if (random8() < sparking) {
            int y = random8(7);
            _heat[y] = qadd8(_heat[y], random8(160, 255));
        }

        // Step 4.  Map from heat cells to LED colors
        for (int j = 0; j < N_PIXELS; j++) {
            byte colorindex = scale8(_heat[j], 240);
            CRGB color = ColorFromPalette(CRGBPalette16(CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White), colorindex);

            int pixelnumber = reverse ? (N_PIXELS - 1 - j) : j;
            leds[pixelnumber] = color;
        }

        FastLED.show();
    }
};
