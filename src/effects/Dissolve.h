#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// DissolveEffect — WLED mode_dissolve (port completo)
// ──────────────────────────────────────────────────────────────────────────────
// Disolución: píxeles aleatorios aparecen/desaparecen gradualmente.
// Algoritmo WLED dissolve():
//   - Fase 1: reemplaza fondo (col1) por color primario/paleta
//   - Fase 2: reemplaza color primario por fondo
//   - Cada frame, spawn aleatorio de nuevos píxeles según intensity
//   - check1 = random colors, check2 = complete mode (espera a todos)
//
// Parámetros:
//   speed     → Repeat speed (sx=128) — higher = cambia fase más rápido
//   intensity → Dissolve rate (ix=128) — higher = más píxeles por frame
//   check1    → Random colors (m1=0) — colores aleatorios vs secuenciales
//   check2    → Complete mode (m2=0) — espera a todos los píxeles
// ──────────────────────────────────────────────────────────────────────────────

class DissolveEffect : public Effect {
private:
    static const char _meta[];

    // Estado persistente
    struct Pixel {
        CRGB color;
    };
    Pixel* _pixels = nullptr;
    bool _pixelsAllocated = false;
    bool _dissolvePhase = true;  // true = dissolving TO primary, false = dissolving TO secondary
    uint8_t _step = 0;

    void allocatePixels() {
        if (!_pixelsAllocated && numLeds > 0) {
            _pixels = new Pixel[numLeds];
            _pixelsAllocated = true;
            // Initialize all to black (secondary)
            for (unsigned i = 0; i < numLeds; i++) {
                _pixels[i].color = CHSV(0, 0, 0);
            }
        }
    }

public:
    DissolveEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    ~DissolveEffect() {
        if (_pixels) delete[] _pixels;
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;
        allocatePixels();
        if (!_pixels) return;

        CRGB secondaryColor = CHSV(0, 0, 0); // Negro

        // ── Spawn nuevos píxeles ────────────────────────────────────────────
        for (unsigned j = 0; j <= numLeds / 15; j++) {
            if (random8() <= params.intensity) {
                for (size_t attempts = 0; attempts < 10; attempts++) {
                    unsigned i = random16(numLeds);

                    if (_dissolvePhase) {
                        // Disolver a primario
                        if (_pixels[i].color == secondaryColor) {
                            // Color: aleatorio o secuencial
                            if (params.check1) {
                                uint8_t hue = random8();
                                _pixels[i].color = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hue, stripLed.brightness, LINEARBLEND);
                            } else {
                                uint8_t hue = uint8_t(i * (256 / numLeds) + (millis() >> 10));
                                _pixels[i].color = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), hue, stripLed.brightness, LINEARBLEND);
                            }
                            break;
                        }
                    } else {
                        // Disolver a secundario
                        if (_pixels[i].color != secondaryColor) {
                            _pixels[i].color = secondaryColor;
                            break;
                        }
                    }
                }
            }
        }

        // ── Render ──────────────────────────────────────────────────────────
        unsigned incompletePixels = 0;
        for (unsigned i = 0; i < numLeds; i++) {
            leds[i] = _pixels[i].color;
            if (params.check2) {
                if (_dissolvePhase) {
                    if (_pixels[i].color == secondaryColor) incompletePixels++;
                } else {
                    if (_pixels[i].color != secondaryColor) incompletePixels++;
                }
            }
        }

        // ── Avanzar fase ───────────────────────────────────────────────────
        if (_step > (255 - params.speed) + 15U) {
            _dissolvePhase = !_dissolvePhase;
            _step = 0;
        } else {
            if (params.check2) {
                if (incompletePixels == 0) _step++;
            } else {
                _step++;
            }
        }

        FastLED.show();
    }
};

const char DissolveEffect::_meta[] =
    "Dissolve@Speed,Rate,,,,,Random,Complete;;;;sx=128,ix=128,m1=0,m2=0";
