#pragma once
#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"
#include "../effects/PaletteManager.h"

// VU: Palette gradient with the hue cycling (default: Rainbow)
class RainbowHueVUEffect : public VUEffect {
public:
    RainbowHueVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {}
    void render() override {
        const uint8_t SPEED = 10;
        static uint8_t hueOffset = 30;
        // Paleta elegible (por defecto Rainbow), leída una vez por frame en vez
        // de en cada píxel: el hue que avanza recorre toda la rampa.
        const CRGBPalette16& pal = PaletteManager::getPalette(_paletteIndex);

        CRGB* vuleds;
        uint16_t height = auxReading();

        vuleds = leds;

        if (height > _peak)
            _peak = constrain(height, 0, numLeds - 1);

        EVERY_N_MILLISECONDS(SPEED) { hueOffset++; }

        for (uint8_t i = 0; i < N_PIXELS; i++) {
            if (i >= height) {
                vuleds[i] = CRGB::Black;
            } else {
                vuleds[i] = ColorFromPalette(pal, hueOffset + (10 * i), 255, LINEARBLEND);
            }
        }
        if (_peak > 0 && _peak <= N_PIXELS - 1)
            vuleds[_peak] = ColorFromPalette(pal, hueOffset, 255, LINEARBLEND);

        dropPeak();
        averageReadings();
        // Brillo: lo aplica StripLed::update() una vez por frame (única fuente de verdad).
        FastLED.show();
    }
};
