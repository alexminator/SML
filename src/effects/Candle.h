#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// Candle — Llama de vela realista
// ──────────────────────────────────────────────────────────────────────────────
// Cada LED parpadea independientemente simulando una llama.
// Colores cálidos: negro → rojo → naranja → amarillo → blanco.
// check1 = multi-candle mode (más variación entre LEDs).
// Params:
//   speed     → velocidad de parpadeo
//   intensity → brillo máximo
//   check1    → multi-candle
// ──────────────────────────────────────────────────────────────────────────────

class CandleEffect : public Effect {
private:
    uint8_t _heat[256];  // max 256 LEDs

public:
    static const char _meta[];

    CandleEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        memset(_heat, 0, sizeof(_heat));
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        uint8_t flickerSpeed = map(params.speed, 0, 255, 3, 1);
        bool multi = params.check1;

        EVERY_N_MILLISECONDS(flickerSpeed) {
            for (uint16_t i = 0; i < numLeds; i++) {
                if (multi) {
                    // Multi-candle: each LED independent
                    _heat[i] = random8(85, 255);
                } else {
                    // Single candle: slight variation, all similar
                    _heat[i] = 191 + random8(64);
                }
            }
        }

        // Map heat to palette colors
        for (uint16_t i = 0; i < numLeds; i++) {
            uint8_t heat = _heat[i];
            uint8_t colorindex = scale8(heat, 240);

            CRGB color = ColorFromPalette(
                PaletteManager::getPalette(_paletteIndex),
                colorindex, 255
            );
            leds[i] = color;
        }

        FastLED.show();
    }
};

const char CandleEffect::_meta[] =
    "Candle@Flicker,,,,Multi;;;;sx=96,ix=224,c1=0,m1=0,pa=23";
