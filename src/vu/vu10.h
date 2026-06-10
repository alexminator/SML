#pragma once
#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"
#include "../effects/PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// PS1DGEQVU — Ecualizador gráfico 1D con partículas
// ──────────────────────────────────────────────────────────────────────────────
// Inspirado en WLED-SR PS1DGEQ. Divide la tira en bandas de frecuencia
// simuladas con niveles de audio que suben/bajan.
// Usa el nivel de audio para crear un efecto de ecualizador con partículas.
// Params:
//   speed     → velocidad de decaimiento
//   intensity → número de bandas
// ──────────────────────────────────────────────────────────────────────────────

class PS1DGEQVUEffect : public VUEffect {
private:
    uint8_t _barHeights[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t _barTargets[8] = {0, 0, 0, 0, 0, 0, 0, 0};

public:
    static const char _meta[];

    PS1DGEQVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        uint8_t decay = map(params.speed, 0, 255, 1, 10);
        uint8_t bands = map(params.intensity, 0, 255, 3, 8);
        if (bands < 3) bands = 3;
        if (bands > 8) bands = 8;

        uint16_t height = auxReading();
        uint8_t audioLevel = map(height, 0, TOP, 0, numLeds);

        fill_solid(leds, numLeds, CRGB::Black);

        // Simulate frequency bands using the same audio level with offsets
        uint8_t bandWidth = numLeds / bands;

        for (uint8_t b = 0; b < bands; b++) {
            // Simulate different "frequencies" by staggering sensitivity
            uint8_t sensitivity = map(b, 0, bands - 1, 80, 200);
            uint8_t target = (audioLevel * sensitivity / 200);

            // Add some random fluctuation per band for visual interest
            if (audioLevel > 5) {
                target = qadd8(target, random8(0, 10));
            }

            _barTargets[b] = constrain(target, 0, numLeds);

            // Smooth toward target
            if (_barHeights[b] < _barTargets[b]) {
                _barHeights[b] = qadd8(_barHeights[b], 6);
            } else {
                _barHeights[b] = (_barHeights[b] > decay) ? _barHeights[b] - decay : 0;
            }

            if (_barHeights[b] > numLeds) _barHeights[b] = numLeds;

            // Draw bar
            uint16_t startX = b * bandWidth;
            uint16_t barHeight = _barHeights[b];
            uint8_t hue = map(b, 0, bands - 1, 96, 0);  // green to red

            for (uint16_t x = 0; x < bandWidth && (startX + x) < numLeds; x++) {
                uint16_t ledIdx = startX + x;
                if (x < barHeight) {
                    leds[ledIdx] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hue, 255, LINEARBLEND);
                } else {
                    leds[ledIdx] = CRGB::Black;
                }
            }

            // Peak particle
            if (barHeight > 0 && barHeight < numLeds) {
                uint16_t peakIdx = startX;
                if (peakIdx < numLeds) {
                    leds[peakIdx] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hue, 255, LINEARBLEND);
                }
            }
        }

        dropPeak();
        averageReadings();
        FastLED.show();
    }
};

const char PS1DGEQVUEffect::_meta[] =
    "PS1DGEQ@Decay,Bands;;;;sx=128,ix=128";
