#pragma once
#include "Effect.h"
#include "../state/AppState.h"

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

        // Colores fijos: rojo, blanco y azul. No es un efecto de paleta (su
        // gracia son los tres colores de la bandera), así que no tiene selector.
        leds[pos1] = CRGB::Blue;
        leds[pos2] = CRGB::Red;
        leds[pos3] = CRGB::White;
        leds[pos1].nscale8(stripLed.brightness);
        leds[pos2].nscale8(stripLed.brightness);
        leds[pos3].nscale8(stripLed.brightness);

        FastLED.show();
    }
};

// Metadata: speed (BPM), intensity (trail)
const char RedWhiteBlueEffect::_meta[] =
    "RWB@Speed,Trail,,,,,,,,;;;;sx=128,ix=128";
