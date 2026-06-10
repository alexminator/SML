#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// RippleEffect — WLED mode_ripple + ripple_base (port completo)
// ──────────────────────────────────────────────────────────────────────────────
// Ondas expansivas que se propagan desde puntos aleatorios de la tira.
// Cada ripple es una struct { state, color, pos } que controla la animación:
//   - state: edad del ripple (0=inactivo, 1-255=propagándose)
//   - color: hue del ripple
//   - pos: posición de origen en la tira
//
// La propagación usa cubicwave8 para crear bordes suaves en el frente de onda.
// Blur opcional suaviza los bordes.
//
// Parámetros:
//   speed     → Propagation speed (sx=128) — higher = faster waves
//   intensity → Drop rate (ix=128) — higher = more ripples per second
//   custom1   → Blur amount (c1=0) — suaviza los bordes de la onda
//   check2    → Overlay (m2=0) — no limpia el fondo, superpone sobre lo anterior
// ──────────────────────────────────────────────────────────────────────────────

class RippleEffect : public Effect {
private:
    static const char _meta[];

    // ── WLED Ripple struct (4 bytes) ───────────────────────────────────────
    struct Ripple {
        uint8_t  state;   // 0 = inactive, 1-255 = active (age)
        uint8_t  color;   // hue index for color
        uint16_t pos;     // position on strip (0 - numLeds-1)
    };

    // For 24 LEDs: maxRipples = min(1 + 24/4, 100) = 7
    static const unsigned MAX_RIPPLES = 8;
    Ripple _ripples[MAX_RIPPLES];

    // ── WLED ripple_base ──────────────────────────────────────────────────
    void ripple_base(uint8_t blurAmount) {
        unsigned maxRipples = min(1 + (int)(numLeds >> 2), (int)MAX_RIPPLES);

        for (unsigned i = 0; i < maxRipples; i++) {
            unsigned state = _ripples[i].state;
            if (state) {
                // ── Propagación ─────────────────────────────────────────────
                unsigned decay = (params.speed >> 4) + 1;   // frames between steps
                uint16_t origin = _ripples[i].pos;
                CRGB col = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), _ripples[i].color, 255, LINEARBLEND);

                // propagation = ((state/decay - 1) * (speed + 1)) / 256
                // propI = integer pixel offset, propF = fractional (for sub-pixel smoothing)
                unsigned propagation = ((state / decay - 1) * ((unsigned)params.speed + 1));
                int propI = propagation >> 8;
                unsigned propF = propagation & 0xFF;

                // Amplitude: bright at start, fades as it expands
                unsigned amp = (state < 17)
                    ? triwave8((state - 1) * 8)        // first 16 frames: fade in
                    : map(state, 17, 255, 255, 2);     // then fade out

                // ── Render 4-pixel wave front (both sides) ─────────────────
                int left  = (int)origin - propI - 1;
                int right = (int)origin + propI + 2;

                for (int v = 0; v < 4; v++) {
                    // cubicwave8 creates smooth edge at wave front
                    uint8_t mag = scale8(cubicwave8((propF >> 2) + v * 64), amp);

                    int lPos = left + v;
                    if (lPos >= 0 && lPos < (int)numLeds) {
                        leds[lPos] = blend(leds[lPos], col, mag);
                    }

                    int rPos = right - v;
                    if (rPos >= 0 && rPos < (int)numLeds) {
                        leds[rPos] = blend(leds[rPos], col, mag);
                    }
                }

                // ── Advance state ──────────────────────────────────────────
                state += decay;
                _ripples[i].state = (state > 254) ? 0 : (uint8_t)state;

            } else {
                // ── Spawn new ripple randomly ──────────────────────────────
                // Probability per frame proportional to intensity
                if (random8() < params.intensity) {
                    _ripples[i].state = 1;
                    _ripples[i].pos = random16(numLeds);
                    _ripples[i].color = random8();  // random hue
                }
            }
        }

        // ── Optional blur ─────────────────────────────────────────────────
        if (blurAmount > 0) {
            blur1d(leds, numLeds, blurAmount);
        }
    }

public:
    RippleEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
        memset(_ripples, 0, sizeof(_ripples));
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        // ── Background: fade_out (blur/overlay) or fill black ──────────────
        // custom1 > 0  → blur mode (softer edges, wave persists)
        // check2 true  → overlay mode (no clear, builds over time)
        if (params.custom1 > 0 || params.check2) {
            fadeToBlackBy(leds, numLeds, 250);
        } else {
            fill_solid(leds, numLeds, CRGB::Black);
        }

        // ── Render ripples ─────────────────────────────────────────────────
        ripple_base(params.custom1 >> 1);   // blurAmount = custom1/2

        FastLED.show();
    }
};

// Metadata completa estilo WLED
const char RippleEffect::_meta[] =
    "Ripple@Speed,Drops,Blur,,,,Overlay,,,,;;;;sx=128,ix=128,c1=0,m2=0";
