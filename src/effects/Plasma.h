#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// PlasmaEffect — WLED mode_plasma (port)
// ──────────────────────────────────────────────────────────────────────────────
// Efecto plasma psicodélico. Combina cubicwave8 y cos8 para generar patrones
// de colores cambiantes.
//
// Parámetros:
//   speed     → Phase speed (sx=128)
//   intensity → Brightness cutoff (ix=128)
// ──────────────────────────────────────────────────────────────────────────────

class PlasmaEffect : public Effect {
private:
    static const char _meta[];
    uint8_t _phaseInit;

public:
    PlasmaEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        _phaseInit = random8(0, 2);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        uint8_t thisPhase = beatsin8(6 + _phaseInit, 64, 192) - 64;
        uint8_t thatPhase = beatsin8(7 + _phaseInit, 64, 192) - 64;

        for (unsigned i = 0; i < numLeds; i++) {
            uint8_t colorIndex = cubicwave8((i * (2 + 3 * (params.speed >> 5)) + thisPhase) & 0xFF) / 2
                               + cos8((i * (1 + 2 * (params.speed >> 5)) + thatPhase) & 0xFF) / 2;
            uint8_t thisBright = qsub8(colorIndex, beatsin8(7, 0, (128 - (params.intensity >> 1))));
            leds[i] = CHSV(colorIndex, 255, scale8(thisBright, stripLed.brightness));
        }

        FastLED.show();
    }
};

const char PlasmaEffect::_meta[] =
    "Plasma@Phase,Brightness,,,,,,,,;;;;sx=128,ix=128";
