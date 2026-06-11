#pragma once
#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"

// VU: Old-skool green and red from bottom or middle
class OldskoolVUEffect : public VUEffect {
public:
    OldskoolVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {}
    void render() override {
        CRGB* vuleds;
        uint8_t i = 0;
        uint16_t height = auxReading();

        vuleds = leds;

        if (height > _peak)
            _peak = constrain(height, 0, numLeds - 1);

        if (is_centered) {
            for (uint8_t i = 0; i < N_PIXELS_HALF; i++) {
                if (i >= height) {
                    vuleds[N_PIXELS_HALF - i - 1] = CRGB::Black;
                    vuleds[N_PIXELS_HALF + i] = CRGB::Black;
                } else {
                    if (i > N_PIXELS_HALF - (N_PIXELS_HALF / 3)) {
                        vuleds[N_PIXELS_HALF - i - 1] = CRGB::Red;
                        vuleds[N_PIXELS_HALF + i] = CRGB::Red;
                    } else {
                        vuleds[N_PIXELS_HALF - i - 1] = CRGB::Green;
                        vuleds[N_PIXELS_HALF + i] = CRGB::Green;
                    }
                }
            }

            if (_peak > 0 && _peak <= N_PIXELS_HALF - 1) {
                if (_peak > N_PIXELS_HALF - (N_PIXELS_HALF / 3)) {
                    vuleds[N_PIXELS_HALF - _peak - 1] = CRGB::Red;
                    vuleds[N_PIXELS_HALF + _peak] = CRGB::Red;
                } else {
                    vuleds[N_PIXELS_HALF - _peak - 1] = CRGB::Green;
                    vuleds[N_PIXELS_HALF + _peak] = CRGB::Green;
                }
            }
        } else {
            for (uint8_t i = 0; i < N_PIXELS; i++) {
                if (i >= height) leds[i] = CRGB::Black;
                else if (i > N_PIXELS - (N_PIXELS / 3)) vuleds[i] = CRGB::Red;
                else vuleds[i] = CRGB::Green;
            }

            if (_peak > 0 && _peak <= N_PIXELS - 1)
                if (_peak > N_PIXELS - (N_PIXELS / 3)) vuleds[_peak] = CRGB::Red;
                else vuleds[_peak] = CRGB::Green;
        }

        dropPeak();
        averageReadings();
        FastLED.setBrightness(stripLed.brightness);
        FastLED.show();
    }
};
