#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// SparkleEffect — WLED mode_sparkle (port completo)
// ──────────────────────────────────────────────────────────────────────────────
// Enciende LEDs aleatorios uno a la vez sobre fondo tenue.
// Algoritmo WLED:
//   - Si no overlay: pinta todo el strip con color tenue de paleta (col1)
//   - Cada cycleTime = 10 + (255-speed)*2, enciende un LED aleatorio
//     a color primario (col0)
//
// Parámetros:
//   speed   → Sparkle speed (sx=128) — higher = más rápido
//   check2  → Overlay (m2=0) — no limpia el fondo
// ──────────────────────────────────────────────────────────────────────────────

class SparkleEffect : public Effect {
private:
    static const char _meta[];
    uint16_t _sparkleIndex = 0;

public:
    SparkleEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        // ── Background: si no overlay, pintar tenue ──────────────────────────
        if (!params.check2) {
            for (unsigned i = 0; i < numLeds; i++) {
                leds[i] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), 0, stripLed.brightness >> 3, LINEARBLEND);
            }
        }

        // ── Timing ───────────────────────────────────────────────────────────
        uint32_t cycleTime = 10 + (255 - params.speed) * 2;
        uint32_t it = millis() / cycleTime;

        static uint32_t lastIt = 0;
        if (it != lastIt) {
            _sparkleIndex = random16(numLeds);
            lastIt = it;
        }

        // ── Sparkle ──────────────────────────────────────────────────────────
        leds[_sparkleIndex] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), 0, stripLed.brightness, LINEARBLEND);

        FastLED.show();
    }
};

const char SparkleEffect::_meta[] =
    "Sparkle@Speed,,,,,,,Overlay;;;;sx=128,m2=0";
