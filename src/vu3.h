#pragma once
#include "Effect.h"
#include "Settings.h"
#include "common.h"

// VU: Rainbow from bottom or middle with hue cycling
class RainbowHueVUEffect : public Effect {
public:
    RainbowHueVUEffect(CRGB* l, uint16_t n) : Effect(l, n) {}
    void render() override {
        const uint8_t SPEED = 10;
        static uint8_t hueOffset = 30;

        CRGB* vuleds;
        uint8_t i = 0;
        uint8_t *peak;
        uint16_t height = auxReading(0);

        vuleds = leds;
        peak = &peakLeft;

        if (height > *peak)
            *peak = height;

        EVERY_N_MILLISECONDS(SPEED) { hueOffset++; }

        if (is_centered) {
            for (uint8_t i = 0; i < N_PIXELS_HALF; i++) {
                if (i >= height) {
                    vuleds[N_PIXELS_HALF - i - 1] = CRGB::Black;
                    vuleds[N_PIXELS_HALF + i] = CRGB::Black;
                } else {
                    vuleds[N_PIXELS_HALF - i - 1] = CHSV(hueOffset + (10 * i), 255, 255);
                    vuleds[N_PIXELS_HALF + i] = CHSV(hueOffset + (10 * i), 255, 255);
                }
            }

            if (*peak > 0 && *peak <= N_PIXELS_HALF - 1) {
                vuleds[N_PIXELS_HALF - *peak - 1] = CHSV(hueOffset, 255, 255);
                vuleds[N_PIXELS_HALF + *peak] = CHSV(hueOffset, 255, 255);
            }
        } else {
            for (uint8_t i = 0; i < N_PIXELS; i++) {
                if (i >= height) {
                    vuleds[i] = CRGB::Black;
                } else {
                    vuleds[i] = CHSV(hueOffset + (10 * i), 255, 255);
                }
            }
            if (*peak > 0 && *peak <= N_PIXELS - 1)
                vuleds[*peak] = CHSV(hueOffset, 255, 255);
        }

        dropPeak(0);
        averageReadings(0);
        FastLED.show();
    }
};
