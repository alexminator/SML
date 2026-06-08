#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// PacificaEffect — WLED mode_pacifica (port)
// ──────────────────────────────────────────────────────────────────────────────
// Efecto oceánico hipnótico con 4 capas de ondas superpuestas en tonos azul/verde.
// Capas con distintas velocidades, escalas y brillos que se mezclan.
//
// Parámetros:
//   speed     → Velocidad de olas (sx=128)
//   intensity → Ángulo/amplitud (ix=128)
// ──────────────────────────────────────────────────────────────────────────────

// Forward declare helper
static CRGB pacifica_one_layer(uint16_t i, uint8_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff, uint8_t intensity);

class PacificaEffect : public Effect {
private:
    static const char _meta[];
    uint16_t _sCIStart1, _sCIStart2, _sCIStart3, _sCIStart4;

public:
    PacificaEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        _sCIStart1 = 0;
        _sCIStart2 = 0;
        _sCIStart3 = 0;
        _sCIStart4 = 0;
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        uint32_t now = millis();

        uint32_t deltams = (16 >> 2) + ((16 * params.speed) >> 7);
        if (deltams < 1) deltams = 1;

        uint32_t speedfactor1 = beatsin16(3, 179, 269);
        uint32_t speedfactor2 = beatsin16(4, 179, 269);
        uint32_t deltams1 = (deltams * speedfactor1) / 256;
        uint32_t deltams2 = (deltams * speedfactor2) / 256;
        uint32_t deltams21 = (deltams1 + deltams2) / 2;

        _sCIStart1 += (deltams1 * beatsin88(1011, 10, 13));
        _sCIStart2 -= (deltams21 * beatsin88(777, 8, 11));
        _sCIStart3 -= (deltams1 * beatsin88(501, 5, 7));
        _sCIStart4 -= (deltams2 * beatsin88(257, 4, 6));

        uint8_t basethreshold = beatsin8(9, 55, 65);
        uint8_t wave = beat8(7);

        for (unsigned i = 0; i < numLeds; i++) {
            CRGB c = CRGB(2, 6, 10);

            c += pacifica_one_layer(i, _sCIStart1 >> 8, beatsin16(3, 11 * 256, 14 * 256),
                                    beatsin8(10, 70, 130), 0 - beat16(301), params.intensity);
            c += pacifica_one_layer(i, _sCIStart2 >> 8, beatsin16(4, 6 * 256, 9 * 256),
                                    beatsin8(17, 40, 80), beat16(401), params.intensity);
            c += pacifica_one_layer(i, _sCIStart3 >> 8, 6 * 256,
                                    beatsin8(9, 10, 38), 0 - beat16(503), params.intensity);
            c += pacifica_one_layer(i, _sCIStart4 >> 8, 5 * 256,
                                    beatsin8(8, 10, 28), beat16(601), params.intensity);

            // White highlights donde las capas coinciden
            unsigned threshold = scale8(sin8(wave), 20) + basethreshold;
            wave += 7;
            unsigned l = c.getAverageLight();
            if (l > threshold) {
                unsigned overage = l - threshold;
                unsigned overage2 = qadd8(overage, overage);
                c += CRGB(overage, overage2, qadd8(overage2, overage2));
            }

            // Profundizar azules y verdes
            c.blue  = scale8(c.blue,  145);
            c.green = scale8(c.green, 200);
            c |= CRGB(2, 5, 7);

            // Aplicar brillo global
            c.nscale8(stripLed.brightness);
            leds[i] = c;
        }

        FastLED.show();
    }
};

// ── Helper ───────────────────────────────────────────────────────────────────
static CRGB pacifica_one_layer(uint16_t i, uint8_t cistart, uint16_t wavescale,
                                uint8_t bri, uint16_t ioff, uint8_t intensity) {
    unsigned ci = cistart;
    unsigned waveangle = ioff;
    unsigned wavescale_half = (wavescale >> 1) + 20;

    waveangle += ((120 + intensity) * i);
    unsigned s16 = sin16(waveangle) + 32768;
    unsigned cs = scale16(s16, wavescale_half) + wavescale_half;
    ci += (cs * i);
    unsigned sindex16 = sin16(ci) + 32768;
    unsigned sindex8 = scale16(sindex16, 240);

    // Mapear a tonos azul/verde/cian
    uint8_t hue = 110 + (sindex8 >> 2);  // ~110-170: cyan/azul
    uint8_t sat = 180 + (sindex8 >> 4);  // 180-255
    return CHSV(hue, sat, bri);
}

const char PacificaEffect::_meta[] =
    "Pacifica@!,Angle,,,,,,,,;;;;sx=128,ix=128";
