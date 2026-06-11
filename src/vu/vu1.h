#pragma once
#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"
#include "../effects/PaletteManager.h"

// VU: Palette gradient from bottom or middle
class RainbowVUEffect : public VUEffect {
public:
    RainbowVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {}
    void render() override {
        uint8_t i = 0;
        uint16_t height = auxReading();
        CRGBPalette16 pal = PaletteManager::getPalette(_paletteIndex);

        fill_solid(leds, N_PIXELS, CRGB::Black);

        if (is_centered) {
            // Palette gradient from center outward — covers ALL LEDs (0..N_PIXELS-1)
            for (i = 0; i < N_PIXELS; i++) {
                uint8_t dist = (i <= N_PIXELS_HALF) ? (N_PIXELS_HALF - i) : (i - N_PIXELS_HALF);
                uint8_t paletteIdx = map(dist, 0, N_PIXELS_HALF, 0, 255);
                leds[i] = ColorFromPalette(pal, paletteIdx, 255, LINEARBLEND);
            }

            for (i = 0; i < N_PIXELS; i++) {
                uint8_t numBlack = (N_PIXELS - constrain(height, 0, N_PIXELS-1)) / 2;
                if (i <= numBlack - 1 || i >= N_PIXELS - numBlack) leds[i] = CRGB::Black;
            }

            if (height / 2 > _peak)
                _peak = height / 2;

            if (_peak > 0 && _peak <= N_PIXELS_HALF - 1) {
                uint8_t peakHue = map(_peak, 0, N_PIXELS_HALF, 0, 255);
                leds[N_PIXELS_HALF + _peak] = ColorFromPalette(pal, peakHue, 255, LINEARBLEND);
                leds[N_PIXELS_HALF - 1 - _peak] = ColorFromPalette(pal, peakHue, 255, LINEARBLEND);
            }
        } else {
            // Palette gradient from bottom to top
            for (i = 0; i < N_PIXELS; i++) {
                uint8_t paletteIdx = map(i, 0, N_PIXELS - 1, 0, 255);
                leds[i] = ColorFromPalette(pal, paletteIdx, 255, LINEARBLEND);
            }

            for (i = 0; i < N_PIXELS; i++) {
                if (i >= height) leds[i] = CRGB::Black;
            }

            if (height > _peak)
                _peak = height;

            if (_peak > 0 && _peak <= N_PIXELS - 1)
                leds[_peak] = ColorFromPalette(pal, map(_peak, 0, N_PIXELS, 0, 255), 255, LINEARBLEND);
        }

        dropPeak();
        averageReadings();
        FastLED.setBrightness(stripLed.brightness);
        FastLED.show();
    }
};
