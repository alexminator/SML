#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

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

        CRGBPalette16 pal = PaletteManager::getPalette(_paletteIndex);
        fadeToBlackBy(leds, numLeds, 200 - trail * 2);

        EVERY_N_MILLISECONDS(speed) {
            _pos += _dir;
            if (_pos >= numLeds - 1) { _pos = numLeds - 1; _dir = -1; }
            if (_pos == 0) { _pos = 0; _dir = 1; }
        }

        // Center glow from palette
        if (_pos > 0) leds[_pos - 1] = ColorFromPalette(pal, 0, 128, LINEARBLEND);
        if (_pos < numLeds - 1) leds[_pos + 1] = ColorFromPalette(pal, 0, 128, LINEARBLEND);
        leds[_pos] = ColorFromPalette(pal, 0, 255, LINEARBLEND);

        FastLED.show();
    }
};

const char LarsonScannerEffect::_meta[] =
    "Scanner@Speed,Trail;;;;sx=128,ix=128,pa=26";
