#pragma once
#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"

// VU: Old-skool green and red (fixed colors, no palette)
class OldskoolVUEffect : public VUEffect {
public:
    OldskoolVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {}
    void render() override {
        CRGB* vuleds;
        uint16_t height = auxReading();

        vuleds = leds;

        if (height > _peak)
            _peak = constrain(height, 0, numLeds - 1);

        for (uint8_t i = 0; i < N_PIXELS; i++) {
            if (i >= height) leds[i] = CRGB::Black;
            else if (i > N_PIXELS - (N_PIXELS / 3)) vuleds[i] = CRGB::Red;
            else vuleds[i] = CRGB::Green;
        }

        if (_peak > 0 && _peak <= N_PIXELS - 1)
            if (_peak > N_PIXELS - (N_PIXELS / 3)) vuleds[_peak] = CRGB::Red;
            else vuleds[_peak] = CRGB::Green;

        dropPeak();
        averageReadings();
        // Brillo: lo aplica StripLed::update() una vez por frame (única fuente de verdad).
        FastLED.show();
    }
};
