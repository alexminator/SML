#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// Drip — Goteo hipnótico
// ──────────────────────────────────────────────────────────────────────────────
// Una gota de agua cae desde arriba, choca contra el "suelo" y se esparce.
// Inspirado en el efecto Drip de WLED.
// Params:
//   speed     → velocidad de caída
//   intensity → tamaño/trailing de la gota
// ──────────────────────────────────────────────────────────────────────────────

class DripEffect : public Effect {
private:
    int16_t  _dropPos = -5;     // posición de la gota (-5 = fuera)
    uint8_t  _splash = 0;       // fase de salpicadura
    uint8_t  _trail[256];       // estela (max 256 LEDs)

public:
    static const char _meta[];

    DripEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        memset(_trail, 0, sizeof(_trail));
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        uint8_t speed = map(params.speed, 0, 255, 20, 2);
        uint8_t size  = map(params.intensity, 0, 255, 2, numLeds / 4);
        if (size < 1) size = 1;

        // Decay trail
        for (uint16_t i = 0; i < numLeds; i++) {
            _trail[i] = (_trail[i] > 8) ? _trail[i] - 8 : 0;
        }

        EVERY_N_MILLISECONDS(speed) {
            if (_dropPos < 0) {
                _dropPos = numLeds - 1;  // start from top
            }

            if (_splash > 0) {
                // Splash phase - ripple outward
                _splash++;
                if (_splash > 10) {
                    _splash = 0;
                    _dropPos = -5;  // reset
                }
            } else {
                // Drop falling
                _dropPos--;
                if (_dropPos < 0) {
                    _splash = 1;
                    _dropPos = 0;
                }
            }
        }

        // Draw trail (gradient)
        for (int16_t i = numLeds - 1; i >= 0; i--) {
            if (i <= _dropPos && _dropPos > 0 && _splash == 0) {
                uint8_t dist = _dropPos - i;
                if (dist < size) {
                    uint8_t b = map(dist, 0, size, 255, 50);
                    _trail[i] = (_trail[i] + b > 255) ? 255 : _trail[i] + b;
                }
            }
            if (_trail[i] > 0) {
                leds[i] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), 160, _trail[i], LINEARBLEND);
            } else {
                leds[i] = CRGB::Black;
            }
        }

        // Draw splash
        if (_splash > 0) {
            uint8_t splashIntensity = (_splash < 5) ? map(_splash, 1, 5, 255, 100) : map(_splash, 5, 10, 100, 0);
            CRGBPalette16 pal = PaletteManager::getPalette(_paletteIndex);
            leds[0] = ColorFromPalette(pal, 160, splashIntensity, LINEARBLEND);
            if (numLeds > 1) leds[1] = ColorFromPalette(pal, 160, splashIntensity / 2, LINEARBLEND);
        }

        FastLED.show();
    }
};

const char DripEffect::_meta[] =
    "Drip@Speed,Size;;;;sx=128,ix=128,pa=22";
