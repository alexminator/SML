#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include <math.h>   // for sqrtf

// ──────────────────────────────────────────────────────────────────────────────
// PopcornEffect — WLED mode_popcorn (port)
// ──────────────────────────────────────────────────────────────────────────────
// Palomitas que saltan: partículas con física de gravedad.
//
// Parámetros:
//   speed     → Gravedad (sx=128) — higher = más gravedad (saltan menos)
//   intensity → Cantidad de palomitas (ix=128)
//   check1    → Overlay (m1=0) — true = no borra fondo
// ──────────────────────────────────────────────────────────────────────────────

#define MAX_POPCORN 16

struct PopcornKernel {
    float pos;
    float vel;
    uint8_t colIndex;
    bool active;
};

class PopcornEffect : public Effect {
private:
    static const char _meta[];
    PopcornKernel _kernels[MAX_POPCORN];

public:
    PopcornEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        for (int i = 0; i < MAX_POPCORN; i++) {
            _kernels[i].active = false;
            _kernels[i].pos = -1.0f;
        }
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        // Background
        if (!params.check1) {
            fill_solid(leds, numLeds, CRGB::Black);
        }

        float gravity = -0.0001f - (params.speed / 200000.0f);
        gravity *= numLeds;

        unsigned numPopcorn = params.intensity * MAX_POPCORN / 255;
        if (numPopcorn == 0) numPopcorn = 1;
        if (numPopcorn > MAX_POPCORN) numPopcorn = MAX_POPCORN;

        for (unsigned i = 0; i < numPopcorn; i++) {
            if (_kernels[i].active) {
                // Update position
                _kernels[i].pos += _kernels[i].vel;
                _kernels[i].vel += gravity;

                // Check if fallen below
                if (_kernels[i].pos < 0) {
                    _kernels[i].active = false;
                }
            } else {
                // Randomly pop
                if (random8() < 2) {
                    _kernels[i].active = true;
                    _kernels[i].pos = 0.01f;

                    unsigned peakHeight = 128 + random8(128);
                    peakHeight = (peakHeight * (numLeds - 1)) >> 8;
                    if (peakHeight < 1) peakHeight = 1;
                    _kernels[i].vel = sqrtf(-2.0f * gravity * peakHeight);

                    _kernels[i].colIndex = random8();
                }
            }

            if (_kernels[i].active && _kernels[i].pos >= 0.0f) {
                unsigned ledIndex = (unsigned)_kernels[i].pos;
                if (ledIndex < numLeds) {
                    uint8_t hue = _kernels[i].colIndex;
                    leds[ledIndex] = CHSV(hue, 255, stripLed.brightness);
                }
            }
        }

        FastLED.show();
    }
};

const char PopcornEffect::_meta[] =
    "Popcorn@!,!,,,,,Overlay;!,!,!;!;;;m12=1";
