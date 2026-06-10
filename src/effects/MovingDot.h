#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// MovingDotEffect — Múltiples dots con beatsin16 (WLED-style)
// ──────────────────────────────────────────────────────────────────────────────
// 4 dots que se mueven sinusoidalmente a lo largo de la tira con fases offset.
// Basado en WLED mode_two_dots pero ampliado a 4 dots con beatsin16.
//
// Parámetros:
//   speed     → Bounce BPM (sx=128) — higher = faster
//   intensity → Trail fade (ix=128) — higher = more persistent trail
// ──────────────────────────────────────────────────────────────────────────────

class MovingDotEffect : public Effect {
private:
    static const char _meta[];

public:
    MovingDotEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        // ── CRITICAL: fade FIRST, draw AFTER ─────────────────────────────────
        // Si se dibuja antes del fade, los dots aparecen más tenues
        uint8_t bpm  = max((uint8_t)1, params.speed);
        uint8_t fade = params.intensity;

        fadeToBlackBy(leds, numLeds, fade);

        // ── 4 dots con beatsin16 ────────────────────────────────────────────
        // Pares con fases 0° y 180° para movimiento contrapuesto
        uint16_t pos1 = beatsin16(bpm, 0, numLeds - 1, 0, 0);
        uint16_t pos2 = beatsin16(bpm * 2, 0, numLeds - 1, 0, 0);
        uint16_t pos3 = beatsin16(bpm, 0, numLeds - 1, 0, 32767);
        uint16_t pos4 = beatsin16(bpm * 2, 0, numLeds - 1, 0, 32767);

        uint8_t hue = beatsin8(45, 0, 255);

        CRGBPalette16 pal = PaletteManager::getPalette(_paletteIndex);
        leds[(pos1 + pos2) / 2] = ColorFromPalette(pal, hue, stripLed.brightness, LINEARBLEND);
        leds[(pos3 + pos4) / 2] = ColorFromPalette(pal, hue + 128, stripLed.brightness, LINEARBLEND);

        FastLED.show();
    }
};

// Metadata: speed (BPM), intensity (trail fade)
const char MovingDotEffect::_meta[] =
    "MovingDot@Speed,Trail,,,,,,,,;;;;sx=128,ix=128";
