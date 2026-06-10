#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// ColorWipeEffect — WLED mode_color_wipe (port completo)
// ──────────────────────────────────────────────────────────────────────────────
// Enciende LEDs uno tras otro desde el principio hasta el final, luego
// retrocede. Soporta gradiente entre color primario y secundario.
//
// Algoritmo WLED: cycleTime = 750 + (255-speed)*150
//   prog = (perc * 65535) / cycleTime, back = (prog > 32767)
//   ledIndex = (prog * SEGLEN) >> 15
//   rem = (prog * SEGLEN * 2) / (intensity + 1)
//
// Parámetros:
//   speed     → Wipe speed (sx=128) — higher = faster
//   intensity → Gradient amount (ix=128) — suavizado entre colores
//   check1    → Reverse (m1=0) — wipe en dirección inversa
// ──────────────────────────────────────────────────────────────────────────────

class ColorWipeEffect : public Effect {
private:
    static const char _meta[];

public:
    ColorWipeEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        uint32_t cycleTime = 750 + (255 - params.speed) * 150;
        uint32_t perc = millis() % cycleTime;
        unsigned prog = (perc * 65535) / cycleTime;
        bool back = (prog > 32767);
        if (back) {
            prog -= 32767;
        }

        unsigned ledIndex = (prog * numLeds) >> 15;
        uint16_t rem = (prog * numLeds) * 2; // mod 0xFFFF by truncating
        rem /= (params.intensity + 1);
        if (rem > 255) rem = 255;

        // Color primario: palette (usamos CHSV basado en índice)
        // Color secundario: complementario (mismo hue, saturación completa)
        uint8_t hueBase = 0; // podríamos usar stripLed.R/G/B pero es más simple así

        // Efecto: usamos el hue de la posición para crear gradiente
        for (unsigned i = 0; i < numLeds; i++) {
            unsigned index = (params.check1 && back) ? numLeds - 1 - i : i;
            CRGB col0, col1;

            // col0 = color base (cálido o del hue actual)
            uint8_t hue = (i * 32) + hueBase;
            col0 = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hue, stripLed.brightness, LINEARBLEND);

            // col1 = color secundario (oscuro / negro)
            col1 = CRGB::Black;

            if (i < ledIndex) {
                leds[index] = back ? col1 : col0;
            } else {
                leds[index] = back ? col0 : col1;
                if (i == ledIndex) {
                    // Blend en el frente de la onda
                    leds[index] = blend(back ? col0 : col1, back ? col1 : col0, uint8_t(rem));
                }
            }
        }

        FastLED.show();
    }
};

const char ColorWipeEffect::_meta[] =
    "ColorWipe@Speed,Gradient,,,,,Reverse;;;;sx=128,ix=128,m1=0";
