// ──────────────────────────────────────────────────────────────────────────────
// PaletteBlendVU — Sound-reactive palette blending waterfall
// ──────────────────────────────────────────────────────────────────────────────
// Ported from VUminator's vu10.
// Usa la paleta seleccionada desde PaletteManager como base.
// Cada N segundos regenera colores objetivo con jitter para variación,
// y hace una transición suave (nblendPaletteTowardPalette) entre estados.
// Cuando el usuario cambia de paleta desde la UI, el cambio es instantáneo.
// Params:
//   speed     → intervalo de cambio (2-20s)
//   intensity → velocidad de transición (0=lento, 255=rápido)
// ──────────────────────────────────────────────────────────────────────────────
#pragma once

#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"
#include "../effects/PaletteManager.h"

class PaletteBlendVUEffect : public VUEffect {
private:
    CRGBPalette16 _currentPalette;
    CRGBPalette16 _targetPalette;
    uint8_t _lastPaletteIdx = 0xFF;
    unsigned long _lastChangeMs = 0;
    unsigned long _lastBlendMs = 0;

    /// Regenera _targetPalette muestreando la paleta de PaletteManager
    void reseed() {
        CRGBPalette16 mgrPal = PaletteManager::getPalette(_paletteIndex);
        for (int i = 0; i < 16; i++) {
            _targetPalette[i] = ColorFromPalette(
                mgrPal, (i * 256 / 16) + random8(0, 20), 255, LINEARBLEND
            );
        }
        _lastPaletteIdx = _paletteIndex;
        _lastChangeMs = millis();
    }

public:
    static const char _meta[];

    PaletteBlendVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {
        setToDefaults(_meta);
        reseed();
        _currentPalette = _targetPalette;
        _lastBlendMs = millis();
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        // ── 1. Cambio de paleta desde UI → INSTANT force ──
        if (_lastPaletteIdx != _paletteIndex) {
            reseed();
            _currentPalette = _targetPalette;  // salta el blend
        }

        // ── 2. Reseed periódico (cada N segundos) ──
        uint16_t interval = map(params.speed, 0, 255, 2000, 20000);
        if (millis() - _lastChangeMs > interval) {
            reseed();
        }

        // ── 3. Blend gradual hacia _targetPalette (60fps) ──
        if (millis() - _lastBlendMs >= 60) {
            _lastBlendMs = millis();
            uint8_t rate = map(params.intensity, 0, 255, 4, 64);
            nblendPaletteTowardPalette(_currentPalette, _targetPalette, rate);
        }

        // ── 4. Audio reactivo ──
        uint16_t height = auxReading();
        uint8_t audioLevel = map(height, 0, TOP, 0, 255);

        CRGB newColor = ColorFromPalette(
            _currentPalette,
            audioLevel,
            constrain(audioLevel + 50, 50, 255),
            LINEARBLEND
        );

        nblend(leds[0], newColor, 128);
        for (int i = numLeds - 1; i > 0; i--) {
            leds[i] = leds[i - 1];
        }

        averageReadings();
        FastLED.setBrightness(stripLed.brightness);
        FastLED.show();
    }
};

const char PaletteBlendVUEffect::_meta[] =
    "Palette Blend@Speed,Response;;;;sx=128,ix=128";
