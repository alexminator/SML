#pragma once
#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"
#include "../effects/PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// DJLightVU — Luz DJ audio-reactiva
// ──────────────────────────────────────────────────────────────────────────────
// Color brillante desde el centro que se expande con el beat.
// El color cambia según la intensidad del audio.
// Inspirado en WLED-SR DJ Light.
// Params:
//   speed     → velocidad de propagación
//   intensity → sensibilidad al color
// ──────────────────────────────────────────────────────────────────────────────

class DJLightVUEffect : public VUEffect {
private:
    uint8_t _hue = 0;

public:
    static const char _meta[];

    DJLightVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        uint16_t height = auxReading();
        uint8_t audioIntensity = constrain(map(height, 0, TOP, 0, 255), 0, 255);

        // Shift hue based on audio intensity
        if (audioIntensity > 30) {
            _hue += (audioIntensity >> 4) + 1;
        }

        // Fade everything
        fadeToBlackBy(leds, numLeds, 200 - (audioIntensity >> 2));

        // Center point
        uint8_t mid = numLeds / 2;
        uint8_t spread = map(audioIntensity, 0, 255, 0, numLeds / 3);

        uint8_t bright = (audioIntensity < 10) ? 0 : audioIntensity * 2 + 50;
        bright = constrain(bright, 0, 255);

        // Draw spread from center
        CRGBPalette16 pal = PaletteManager::getPalette(_paletteIndex);
        CRGB color = ColorFromPalette(pal, _hue, bright, LINEARBLEND);
        for (uint8_t i = 0; i <= spread; i++) {
            if (mid + i < numLeds) leds[mid + i] = color;
            if (mid >= i) leds[mid - i] = color;
        }

        // Center is brightest
        if (mid < numLeds) leds[mid] = ColorFromPalette(pal, _hue, bright, LINEARBLEND);

        dropPeak();
        averageReadings();
        FastLED.show();
    }
};

const char DJLightVUEffect::_meta[] =
    "DJ Light@Speed,Color Shift;;;;sx=128,ix=128";
