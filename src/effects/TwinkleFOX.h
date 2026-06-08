#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// TwinkleFOXEffect — WLED mode_twinklefox (port)
// ──────────────────────────────────────────────────────────────────────────────
// TwinkleFOX de Mark Kriegsman: luces que parpadean con fade in/out suave.
// Cada LED tiene su propio "reloj" pseudoaleatorio para el timing.
//
// Parámetros:
//   speed     → Velocidad de parpadeo (sx=128)
//   intensity → Densidad (ix=128) — higher = más LEDs encendidos
//   custom1   → Cool (c1=0) — simula enfriamiento hacia rojo
//   check1    → Cool on/off (m1=0)
// ──────────────────────────────────────────────────────────────────────────────

class TwinkleFOXEffect : public Effect {
private:
    static const char _meta[];
    uint8_t _aux0;   // speed divisor

public:
    TwinkleFOXEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        _aux0 = 22;
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        // Calculate speed divisor
        if (params.speed > 100) {
            _aux0 = 3 + ((255 - params.speed) >> 3);
        } else {
            _aux0 = 22 + ((100 - params.speed) >> 1);
        }
        if (_aux0 < 1) _aux0 = 1;

        uint32_t ms = millis();
        uint16_t PRNG16 = 11337;  // MUST reset to same value each frame

        // Background: secondary color (dim dark)
        CRGB bg = CRGB::Black;

        for (unsigned i = 0; i < numLeds; i++) {
            PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384;
            unsigned myclockoffset16 = PRNG16;
            PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384;
            unsigned myspeedmultiplierQ5_3 = ((((PRNG16 & 0xFF) >> 4) + (PRNG16 & 0x0F)) & 0x0F) + 0x08;
            uint32_t myclock30 = ((ms * myspeedmultiplierQ5_3) >> 3) + myclockoffset16;
            unsigned myunique8 = PRNG16 >> 8;

            // One twinkle calculation
            unsigned ticks = myclock30 / _aux0;
            unsigned fastcycle8 = uint8_t(ticks);
            uint16_t slowcycle16 = (ticks >> 8) + myunique8;
            slowcycle16 += sin8(slowcycle16);
            slowcycle16 = (slowcycle16 * 2053) + 1384;
            uint8_t slowcycle8 = (slowcycle16 & 0xFF) + (slowcycle16 >> 8);

            unsigned twinkleDensity = (params.intensity >> 5) + 1;
            unsigned bright = 0;

            if (((slowcycle8 & 0x0E) / 2) < twinkleDensity) {
                unsigned ph = fastcycle8;
                // Triangle wave with fast attack, slow decay (twinklefox style)
                if (ph < 86) {
                    bright = ph * 3;
                } else {
                    ph -= 86;
                    bright = 255 - (ph + (ph / 2));
                }
            }

            unsigned hue = slowcycle8 - myunique8;

            if (bright > 0) {
                uint8_t bri = scale8(bright, stripLed.brightness);
                leds[i] = CHSV(hue, 200, bri);

                // Simulate incandescent cooling toward red as they dim
                if (params.check1 && fastcycle8 >= 128) {
                    unsigned cooling = (fastcycle8 - 128) >> 4;
                    leds[i].g = qsub8(leds[i].g, cooling);
                    leds[i].b = qsub8(leds[i].b, cooling * 2);
                }
            } else {
                leds[i] = bg;
            }
        }

        FastLED.show();
    }
};

const char TwinkleFOXEffect::_meta[] =
    "TwinkleFOX@!,Twinkle rate,,,,Cool;!,!;!;;;;sx=128,ix=128";
