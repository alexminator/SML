#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// TwinkleEffect — TwinkleFOX by Mark Kriegsman (WLED-compatible)
// ──────────────────────────────────────────────────────────────────────────────
// Parameters (from metadata):
//   speed     → Animation speed (sx=8) — higher = faster twinkling
//   intensity → Twinkle density (ix=160) — higher = more simultaneous twinkles
//   check1    → RedCool (m1=1) — cools red tones for a bluer palette
// ──────────────────────────────────────────────────────────────────────────────

class TwinkleEffect : public Effect {
    static const char _meta[];

public:
    TwinkleEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        uint16_t speedVal      = params.speed;
        uint8_t intensityVal   = params.intensity;
        bool redCool           = params.check1;

        uint16_t PRNG16 = 11337;
        static unsigned long lastUpdate = 0;
        uint32_t ms = millis();

        uint16_t speedDelay = map(speedVal, 0, 255, 100, 10);

        if (ms - lastUpdate < speedDelay) {
            FastLED.show();
            return;
        }
        lastUpdate = ms;

        uint32_t ticks = ms / speedDelay;
        uint8_t twinkleDensity = (intensityVal >> 5) + 1;

        fadeToBlackBy(leds, numLeds, 200);

        PRNG16 = 11337;
        for (unsigned i = 0; i < numLeds; i++) {
            uint8_t salt = i * 3;

            uint16_t prng = (PRNG16 * 2053) + 1384;

            uint8_t fastcycle8 = ticks & 0xFF;
            uint16_t slowcycle16 = (ticks >> 8) + salt;
            slowcycle16 += sin8(slowcycle16);
            slowcycle16 = (slowcycle16 * 2053) + 1384;
            uint8_t slowcycle8 = (slowcycle16 & 0xFF) + (slowcycle16 >> 8);

            if (((slowcycle8 & 0x0E) / 2) < twinkleDensity) {
                uint8_t bright;
                uint8_t ph = fastcycle8;

                if (ph < 86) {
                    bright = ph * 3;
                } else {
                    ph -= 86;
                    bright = 255 - (ph + (ph / 2));
                }

                if (bright > 0) {
                    uint8_t hue = slowcycle8 - salt;
                    uint8_t scaledBright = scale8(bright, stripLed.brightness);
                    CRGB c = CHSV(hue, 255, scaledBright);

                    if (redCool && (fastcycle8 >= 128)) {
                        uint8_t cooling = (fastcycle8 - 128) >> 4;
                        c.g = qsub8(c.g, cooling);
                        c.b = qsub8(c.b, cooling * 2);
                    }

                    leds[i] = c;
                }
            }

            PRNG16 += salt;
        }

        FastLED.show();
    }
};

// Metadata: "Name@labels;defaults"
const char TwinkleEffect::_meta[] =
    "Twinkle@Speed,Density,,,,,,,,RedCool;;;;sx=8,ix=160,m1=1";
