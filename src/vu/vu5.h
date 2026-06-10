#pragma once
#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"
#include "../effects/PaletteManager.h"

// VU: Hue cycling, three bars (shatter, mono)
class ThreebarsVUEffect : public VUEffect {
public:
    ThreebarsVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {}
    void render() override {
        const uint8_t DRAW_MAX = 30;
        const uint8_t SEGMENTS = 3;

        static int origin = 0;
        static uint8_t scroll_color = 0;
        static int last_intensity = 0;
        static int intensity_max = 0;
        static int origin_at_flip = 0;
        static CRGB draw[DRAW_MAX];
        static bool growing = false;
        static bool fall_from_left = true;

        int intensity = auxReading();

        if (growing && intensity < last_intensity) {
            growing = false;
            intensity_max = last_intensity;
            fall_from_left = !fall_from_left;
            origin_at_flip = origin;
        } else if (intensity > last_intensity) {
            growing = true;
            origin_at_flip = origin;
        }
        last_intensity = intensity;

        if (!growing) {
            if (fall_from_left) {
                origin = origin_at_flip + ((intensity_max - intensity) / 2);
            } else {
                origin = origin_at_flip - ((intensity_max - intensity) / 2);
            }
            if (origin < 0) {
                origin = DRAW_MAX - abs(origin);
            } else if (origin > DRAW_MAX - 1) {
                origin = origin - DRAW_MAX - 1;
            }
        }

        int min_lit = origin - (intensity / 2);
        int max_lit = origin + (intensity / 2);
        if (min_lit < 0) {
            min_lit = min_lit + DRAW_MAX;
        }
        if (max_lit >= DRAW_MAX) {
            max_lit = max_lit - DRAW_MAX;
        }
        for (int i = 0; i < DRAW_MAX; i++) {
            if (
                (min_lit < max_lit && min_lit < i && i < max_lit)
                || (min_lit > max_lit && (i > min_lit || i < max_lit))
            ) {
                draw[i] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), scroll_color, 255, LINEARBLEND);
            } else {
                draw[i] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), scroll_color, 0, LINEARBLEND);
            }
        }

        int seg_len = N_PIXELS / SEGMENTS;

        for (int s = 0; s < SEGMENTS; s++) {
            for (int i = 0; i < seg_len; i++) {
                leds[i + (s * seg_len)] = draw[map(i, 0, seg_len, 0, DRAW_MAX)];
            }
        }

        FastLED.setBrightness(stripLed.brightness);
        FastLED.show();
        averageReadings();

        EVERY_N_MILLISECONDS(20) {
            scroll_color = ++scroll_color % 255;
        }
    }
};
