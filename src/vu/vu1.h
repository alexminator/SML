#pragma once
#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"
#include "../effects/PaletteManager.h"

// VU: Rainbow gradient from bottom to top
class RainbowVUEffect : public VUEffect {
public:
    RainbowVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {}
    void render() override {
        uint8_t i = 0;
        uint16_t height = auxReading();
        // Paleta elegible (por defecto Rainbow): el degradado se arma con ella
        // de abajo a arriba, así que se ve entera cambie lo que cambie.
        CRGBPalette16 pal = PaletteManager::getPalette(_paletteIndex);

        fill_solid(leds, N_PIXELS, CRGB::Black);

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

        dropPeak();
        averageReadings();
        // Brillo: lo aplica StripLed::update() una vez por frame (única fuente de verdad).
        FastLED.show();
    }
};
