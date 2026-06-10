#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// BreathEffect — WLED mode_breath adaptado para SML
// ──────────────────────────────────────────────────────────────────────────────
// Algoritmo original WLED: "standby-breathing" de i-Devices.
// Usa sin16_t para crear una curva parabólica suave que controla la luminosidad
// en un ciclo respiratorio (sube lentamente, baja más rápido).
//
// Parámetros:
//   speed → Breathing rate (sx=128) — higher = faster breathing
// ──────────────────────────────────────────────────────────────────────────────

class BreathEffect : public Effect {
private:
    static const char _meta[];

public:
    BreathEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        // ── WLED mode_breath timing ─────────────────────────────────────────
        // counter = millis * ((speed>>3) + 10), acotado a 16 bits
        unsigned counter = (millis() * ((params.speed >> 3) + 10)) & 0xFFFFU;
        counter = (counter >> 2) + (counter >> 4); // 0-16384 + 0-2048 ≈ 0-18432

        // var = sin16_t(counter) / 103 → curva parabólica suave
        // peak ~23170/103 ≈ 225 → lum = 30 + 225 = 255
        unsigned var = 0;
        if (counter < 16384) {
            if (counter > 8192) counter = 8192 - (counter - 8192);
            var = sin16(counter) / 103;
        }

        uint8_t lum = 30 + var;  // 30-255, nunca negro completo

        // ── Color: evolución lenta del hue ──────────────────────────────────
        uint8_t hue = (millis() >> 11) * 5;  // cambia ~cada 2s

        for (unsigned i = 0; i < numLeds; i++) {
            CRGB color = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hue, stripLed.brightness, LINEARBLEND);
            leds[i] = color.scale8(lum);     // color_blend(BLACK, color, lum)
        }

        FastLED.show();
    }
};

// Metadata: speed solamente
const char BreathEffect::_meta[] =
    "Breath@Speed,,,,,,,,;;;;sx=128";
