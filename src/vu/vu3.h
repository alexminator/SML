#pragma once
#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"
#include "../effects/PaletteManager.h"

// VU: Rainbow from bottom or middle with hue cycling
class RainbowHueVUEffect : public VUEffect {
public:
    RainbowHueVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {}
    void render() override {
        const uint8_t SPEED = 10;
        static uint8_t hueOffset = 30;

        CRGB* vuleds;
        uint8_t i = 0;
        uint16_t height = auxReading();

        vuleds = leds;

        if (height > _peak)
            _peak = height;

        EVERY_N_MILLISECONDS(SPEED) { hueOffset++; }

        if (is_centered) {
            for (uint8_t i = 0; i < N_PIXELS_HALF; i++) {
                if (i >= height) {
                    vuleds[N_PIXELS_HALF - i - 1] = CRGB::Black;
                    vuleds[N_PIXELS_HALF + i] = CRGB::Black;
                } else {
                    uint8_t hueIdx = hueOffset + (10 * i);
                    vuleds[N_PIXELS_HALF - i - 1] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hueIdx, 255, LINEARBLEND);
                    vuleds[N_PIXELS_HALF + i] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hueIdx, 255, LINEARBLEND);
                }
            }

            if (_peak > 0 && _peak <= N_PIXELS_HALF - 1) {
                vuleds[N_PIXELS_HALF - _peak - 1] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hueOffset, 255, LINEARBLEND);
                vuleds[N_PIXELS_HALF + _peak] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hueOffset, 255, LINEARBLEND);
            }
        } else {
            for (uint8_t i = 0; i < N_PIXELS; i++) {
                if (i >= height) {
                    vuleds[i] = CRGB::Black;
                } else {
                    vuleds[i] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hueOffset + (10 * i), 255, LINEARBLEND);
                }
            }
            if (_peak > 0 && _peak <= N_PIXELS - 1)
                vuleds[_peak] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hueOffset, 255, LINEARBLEND);
        }

        dropPeak();
        averageReadings();
        FastLED.show();
    }
};
