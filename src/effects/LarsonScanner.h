#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// ──────────────────────────────────────────────────────────────────────────────
// LarsonScanner — Clásico KITT / Knight Rider
// ──────────────────────────────────────────────────────────────────────────────
// Un punto brillante rebota de un extremo a otro con estela.
// Params:
//   speed     → velocidad de barrido
//   intensity → largo de la estela (trail)
// ──────────────────────────────────────────────────────────────────────────────

class LarsonScannerEffect : public Effect {
private:
    int8_t  _dir = 1;
    uint8_t _pos = 0;

public:
    static const char _meta[];

    LarsonScannerEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        uint8_t trail = map(params.intensity, 0, 255, 1, numLeds / 3);
        uint8_t speed = map(params.speed, 0, 255, 80, 2);

        fadeToBlackBy(leds, numLeds, 200 - trail * 2);

        leds[_pos] = CRGB::White;

        EVERY_N_MILLISECONDS(speed) {
            _pos += _dir;
            if (_pos >= numLeds - 1) { _pos = numLeds - 1; _dir = -1; }
            if (_pos == 0) { _pos = 0; _dir = 1; }
        }

        // Center glow
        if (_pos > 0) leds[_pos - 1] = CRGB(128, 128, 128);
        if (_pos < numLeds - 1) leds[_pos + 1] = CRGB(128, 128, 128);
        leds[_pos] = CRGB::White;

        FastLED.show();
    }
};

const char LarsonScannerEffect::_meta[] =
    "Scanner@Speed,Trail;;;;sx=128,ix=128";
