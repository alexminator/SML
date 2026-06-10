#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// ICUEffect — Monitor de hospital (ICU beep / ECG scan)
// ──────────────────────────────────────────────────────────────────────────────
// Un punto recorre la tira como un monitor de ECG, seguido de un "beep".
// Params:
//   speed     → velocidad de barrido
//   intensity → brillo
// ──────────────────────────────────────────────────────────────────────────────

class ICUEffect : public Effect {
private:
    uint8_t _pos = 0;
    bool    _beeping = false;
    uint8_t _beepPhase = 0;

public:
    static const char _meta[];

    ICUEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        uint8_t speed = map(params.speed, 0, 255, 60, 2);
        uint8_t bright = 255;  // Brillo fijo (el global stripLed.brightness controla el brillo)

        fadeToBlackBy(leds, numLeds, 220);

        if (_beeping) {
            // Flash entire strip brighter
            uint8_t flash = (_beepPhase < 3) ? bright : 0;
            CRGB color = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), 120, flash, LINEARBLEND);
            fill_solid(leds, numLeds, color);
            _beepPhase++;
            if (_beepPhase > 6) {
                _beeping = false;
                _beepPhase = 0;
                _pos = 0;
            }
        } else {
            // Scan dot with green ECG-like trail
            CRGBPalette16 pal = PaletteManager::getPalette(_paletteIndex);
            leds[_pos] = ColorFromPalette(pal, 120, bright, LINEARBLEND);

            // Trail behind
            for (uint8_t i = 1; i <= 4; i++) {
                if (_pos >= i) {
                    leds[_pos - i] = ColorFromPalette(pal, 120, bright / (i * 3), LINEARBLEND);
                }
            }

            EVERY_N_MILLISECONDS(speed) {
                _pos++;
                if (_pos >= numLeds - 1) {
                    _beeping = true;
                }
            }
        }

        FastLED.show();
    }
};

const char ICUEffect::_meta[] =
    "ICU@Speed;;;;sx=128,ix=128,pa=20";
