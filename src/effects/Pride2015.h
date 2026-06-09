#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// Pride2015Effect — WLED mode_pride_2015 (port)
// ──────────────────────────────────────────────────────────────────────────────
// Arcoíris animado que cambia solo (Mark Kriegsman). Ondas de color orgánicas
// con variaciones de brillo y saturación.
//
// Parámetros:
//   speed     → Velocidad de cambio (sx=128)
//   intensity → (sin uso)
// ──────────────────────────────────────────────────────────────────────────────

class Pride2015Effect : public Effect {
private:
    static const char _meta[];
    uint32_t _sPseudotime;
    uint32_t _sHue16;

public:
    Pride2015Effect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        _sPseudotime = 0;
        _sHue16 = 0;
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        uint32_t now = millis();
        uint32_t duration = 10 + params.speed;

        uint8_t sat8 = beatsin88(87, 220, 250);
        unsigned brightdepth = beatsin88(341, 96, 224);
        unsigned brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
        unsigned msmultiplier = beatsin88(147, 23, 60);

        unsigned hue16 = _sHue16;
        unsigned hueinc16 = beatsin88(113, 300, 3000); // full color range

        _sPseudotime += duration * msmultiplier;
        _sHue16 += duration * beatsin88(400, 5, 9);
        unsigned brightnesstheta16 = _sPseudotime;

        for (unsigned i = 0; i < numLeds; i++) {
            hue16 += hueinc16;
            uint8_t hue8 = hue16 >> 8;

            brightnesstheta16 += brightnessthetainc16;
            unsigned b16 = sin16(brightnesstheta16) + 32768;
            unsigned bri16 = ((uint32_t)b16 * b16) / 65536;
            uint8_t bri8 = ((uint32_t)bri16 * brightdepth) / 65536;
            bri8 += (255 - brightdepth);

            leds[i] = CHSV(hue8, sat8, scale8(bri8, stripLed.brightness));
        }

        FastLED.show();
    }
};

const char Pride2015Effect::_meta[] =
    "Pride 2015@Speed,,,,,,,,;;;;sx=128";
