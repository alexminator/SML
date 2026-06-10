#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// ColorSweepEffect — WLED color_sweep adaptado para SML
// ──────────────────────────────────────────────────────────────────────────────
// Barrido de color desde ambos extremos hacia el centro.
// Algoritmo basado en WLED color_wipe(rev=true, back=false):
//   - Llena LEDs desde ambos extremos gradualmente
//   - El color evoluciona lentamente a través del espectro
//
// Parámetros:
//   speed → Sweep speed (sx=128) — higher = faster sweep
//   intensity → Gradient width (ix=128) — higher = wider gradient trail
// ──────────────────────────────────────────────────────────────────────────────

class ColorSweepEffect : public Effect {
private:
    static const char _meta[];

public:
    ColorSweepEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        // ── WLED-style timing ───────────────────────────────────────────────
        // cycleTime: how fast the sweep moves (ms per step)
        uint32_t cycleTime = map(params.speed, 0, 255, 150, 10);
        if (cycleTime < 1) cycleTime = 1;

        // Progress: position of the sweep wave (0 → numLeds/2)
        uint32_t now = millis();
        unsigned halfLen = numLeds / 2;
        unsigned prog = (now / cycleTime) % (halfLen + 1);

        // ── Color: hue evolves slowly ───────────────────────────────────────
        uint8_t hue = (now >> 11) * 3;

        // ── Gradient width from intensity ───────────────────────────────────
        uint8_t gradient = map(params.intensity, 0, 255, 1, halfLen / 2 + 1);
        if (gradient < 1) gradient = 1;

        // ── Render ──────────────────────────────────────────────────────────
        fill_solid(leds, numLeds, CRGB::Black);

        // Fill from both ends with gradient
        for (unsigned i = 0; i <= prog && i < halfLen; i++) {
            // Gradient: brighter near the tip, dimmer near the base
            uint8_t bright = stripLed.brightness;
            if (i + gradient > prog) {
                // Trail gradient
                unsigned distToTip = prog - i;
                bright = scale8(bright, map(distToTip, 0, gradient, 255, 20));
            }
            CRGB color = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hue + i * 6, bright, LINEARBLEND);
            leds[i] = color;
            leds[numLeds - 1 - i] = color;
        }

        // Center LED (odd length)
        if (prog == halfLen && (numLeds & 1)) {
            leds[halfLen] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hue + halfLen * 6, stripLed.brightness, LINEARBLEND);
        }

        FastLED.show();
    }
};

// Metadata: speed (sweep rate), intensity (gradient width)
const char ColorSweepEffect::_meta[] =
    "Sweep@Speed,Gradient,,,,,,,,;;;;sx=128,ix=128";
