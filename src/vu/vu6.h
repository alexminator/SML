#pragma once
#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"
#include "../effects/PaletteManager.h"

// VU: Ocean waves — sound-reactive waves emanating from center
class OceanVUEffect : public VUEffect {
public:
    OceanVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {}

    void render() override {
        CRGBPalette16 pal = PaletteManager::getPalette(_paletteIndex);

        EVERY_N_MILLISECONDS_I(thistimer, 20) {
            uint8_t timeval = beatsin8(10, 20, 50);
            thistimer.setPeriod(timeval);
            fadeToBlackBy(leds, N_PIXELS, 16);
            sndwave(pal);
            // Adjust dynamic min/max range using the circular buffer
            averageReadings();
        }

        FastLED.setBrightness(stripLed.brightness);
        FastLED.show();
    }

private:
    void sndwave(const CRGBPalette16& pal) {
        uint16_t height = auxReading();  // 0..TOP with noise gate + dampening

        // Scale 0..TOP → 0..255 for palette index and brightness
        uint8_t intensity = (height * 255U) / TOP;

        leds[N_PIXELS_HALF] = ColorFromPalette(
            pal, intensity, intensity, LINEARBLEND
        );

        // Shift right half right, left half left (wave ripples outward)
        for (int i = N_PIXELS - 1; i > N_PIXELS_HALF; i--) {
            leds[i] = leds[i - 1];
        }
        for (int i = 0; i < N_PIXELS_HALF; i++) {
            leds[i] = leds[i + 1];
        }
    }
};
