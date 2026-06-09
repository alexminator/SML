#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// ColorwavesEffect — WLED mode_colorwaves (port)
// ──────────────────────────────────────────────────────────────────────────────
// Ondas de color con paleta (Mark Kriegsman). Similar a Pride 2015 pero con
// paleta configurable y más suave.
//
// Parámetros:
//   speed     → Velocidad de onda (sx=128)
//   intensity → Hue range (ix=128) — higher = más variación de color
// ──────────────────────────────────────────────────────────────────────────────

class ColorwavesEffect : public Effect {
private:
    static const char _meta[];
    uint32_t _sPseudotime;
    uint32_t _sHue16;

public:
    ColorwavesEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        _sPseudotime = 0;
        _sHue16 = 0;
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        uint32_t duration = 10 + params.speed;

        unsigned brightdepth = beatsin88(341, 96, 224);
        unsigned brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
        unsigned msmultiplier = beatsin88(147, 23, 60);

        unsigned hue16 = _sHue16;
        unsigned hueinc16 = beatsin88(113, 60, 300) * params.intensity * 10 / 255;
        if (hueinc16 < 60) hueinc16 = 60;

        _sPseudotime += duration * msmultiplier;
        _sHue16 += duration * beatsin88(400, 5, 9);
        unsigned brightnesstheta16 = _sPseudotime;

        for (unsigned i = 0; i < numLeds; i++) {
            hue16 += hueinc16;

            // Mapeo tipo palette: triángulo simétrico
            unsigned h16_128 = hue16 >> 7;
            uint8_t hue8 = (h16_128 & 0x100) ? (255 - (h16_128 >> 1)) : (h16_128 >> 1);

            brightnesstheta16 += brightnessthetainc16;
            unsigned b16 = sin16(brightnesstheta16) + 32768;
            unsigned bri16 = ((uint32_t)b16 * b16) / 65536;
            uint8_t bri8 = ((uint32_t)bri16 * brightdepth) / 65536;
            bri8 += (255 - brightdepth);

            leds[i] = CHSV(hue8, 255, scale8(bri8, stripLed.brightness));
        }

        FastLED.show();
    }
};

const char ColorwavesEffect::_meta[] =
    "Colorwaves@Speed,Hue Range,,,,,,,,;;;;sx=128,ix=128";
