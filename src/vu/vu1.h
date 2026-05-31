#pragma once
#include "../effects/Effect.h"
#include "../Settings.h"
#include "VUEffect.h"

// VU: Rainbow from bottom or middle, green through purple
class RainbowVUEffect : public VUEffect {
public:
    RainbowVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {}
    void render() override {
        CRGB* vuleds;
        uint8_t i = 0;
        uint16_t height = auxReading();

        vuleds = leds;

        fill_solid(vuleds, N_PIXELS, CRGB::Black);
        if (is_centered) {
            fill_gradient(vuleds, N_PIXELS_HALF,  CHSV(96, 255, 255), N_PIXELS - 1, CHSV(224, 255, 255), SHORTEST_HUES);
            fill_gradient(vuleds, N_PIXELS_HALF-1, CHSV(96, 255, 255), 0, CHSV(224, 255, 255), LONGEST_HUES);

            for (i = 0; i < N_PIXELS; i++) {
                uint8_t numBlack = (N_PIXELS - constrain(height, 0, N_PIXELS-1)) / 2;
                if (i <= numBlack - 1 || i >= N_PIXELS - numBlack) leds[i] = CRGB::Black;
            }

            if (height / 2 > _peak)
                _peak = height / 2;

            if (_peak > 0 && _peak <= N_PIXELS_HALF - 1) {
                vuleds[N_PIXELS_HALF + _peak] = CHSV(rainbowHue2(_peak, N_PIXELS_HALF), 255, 255);
                vuleds[N_PIXELS_HALF - 1 - _peak] = CHSV(rainbowHue2(_peak, N_PIXELS_HALF), 255, 255);
            }
        } else {
            fill_gradient(leds, 0, CHSV(96, 255, 255), N_PIXELS - 1, CHSV(224, 255, 255), SHORTEST_HUES);

            for (i = 0; i < N_PIXELS; i++) {
                if (i >= height) leds[i] = CRGB::Black;
            }

            if (height > _peak)
                _peak = height;

            if (_peak > 0 && _peak <= N_PIXELS - 1)
                leds[_peak] = CHSV(rainbowHue2(_peak, N_PIXELS), 255, 255);
        }

        dropPeak();
        averageReadings();
        FastLED.show();
    }

private:
    uint8_t rainbowHue2(uint8_t pixel, uint8_t num_pixels) {
        uint8_t hue = 96 - pixel * (145 / num_pixels);
        return hue;
    }
};
