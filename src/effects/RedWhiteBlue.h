#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// RedWhiteBlueEffect — Tres dots bouncing con beatsin16
// ──────────────────────────────────────────────────────────────────────────────
// Tres puntos (rojo, blanco, azul) rebotando independientemente a lo largo de
// la tira usando beatsin16 con fases offset.
//
// Parámetros:
//   speed     → Bounce BPM (sx=128) — higher = faster bouncing
//   intensity → Trail fade (ix=128) — higher = shorter trail
// ──────────────────────────────────────────────────────────────────────────────

class RedWhiteBlueEffect : public Effect {
private:
    static const char _meta[];

public:
    RedWhiteBlueEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        // ── BPM de movimiento ────────────────────────────────────────────────
        uint8_t bpm = max((uint8_t)1, params.speed);

        // ── Trail fade from intensity ───────────────────────────────────────
        uint8_t fader = map(params.intensity, 0, 255, 5, 100);
        fadeToBlackBy(leds, numLeds, fader);

        // ── Tres dots con fases offset ──────────────────────────────────────
        uint16_t pos1 = beatsin16(bpm, 0, numLeds - 1, 0, 0);
        uint16_t pos2 = beatsin16(bpm, 0, numLeds - 1, 0, 21845);  // 120° offset
        uint16_t pos3 = beatsin16(bpm, 0, numLeds - 1, 0, 43690);  // 240° offset

        // Colores desde paleta
        CRGBPalette16 pal = PaletteManager::getPalette(_paletteIndex);
        leds[pos1] = ColorFromPalette(pal, 170, stripLed.brightness, LINEARBLEND);
        leds[pos2] = ColorFromPalette(pal, 0,   stripLed.brightness, LINEARBLEND);
        leds[pos3] = ColorFromPalette(pal, 85,  stripLed.brightness, LINEARBLEND);

        FastLED.show();
    }
};

// Metadata: speed (BPM), intensity (trail)
const char RedWhiteBlueEffect::_meta[] =
    "RWB@Speed,Trail,,,,,,,,;;;;sx=128,ix=128,pa=21";
