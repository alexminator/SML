#pragma once
#include "Effect.h"
#include "Settings.h"
#include "common.h"

// VU: Old-skool green and red from bottom or middle
class OldskoolVUEffect : public Effect {
public:
    OldskoolVUEffect(CRGB* l, uint16_t n) : Effect(l, n) {}
    void render() override {
        CRGB* vuleds;
        uint8_t i = 0;
        uint8_t *peak;
        uint16_t height = auxReading(0);

        vuleds = leds;
        peak = &peakLeft;

        if (height > *peak)
            *peak = height;

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

            if (*peak > 0 && *peak <= N_PIXELS_HALF - 1) {
                if (*peak > N_PIXELS_HALF - (N_PIXELS_HALF / 3)) {
                    vuleds[N_PIXELS_HALF - *peak - 1] = CRGB::Red;
                    vuleds[N_PIXELS_HALF + *peak] = CRGB::Red;
                } else {
                    vuleds[N_PIXELS_HALF - *peak - 1] = CRGB::Green;
                    vuleds[N_PIXELS_HALF + *peak] = CRGB::Green;
                }
            }
        } else {
            for (uint8_t i = 0; i < N_PIXELS; i++) {
                if (i >= height) leds[i] = CRGB::Black;
                else if (i > N_PIXELS - (N_PIXELS / 3)) vuleds[i] = CRGB::Red;
                else vuleds[i] = CRGB::Green;
            }

            if (*peak > 0 && *peak <= N_PIXELS - 1)
                if (*peak > N_PIXELS - (N_PIXELS / 3)) vuleds[*peak] = CRGB::Red;
                else vuleds[*peak] = CRGB::Green;
        }

        dropPeak(0);
        averageReadings(0);
        FastLED.show();
    }
};
