#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include <math.h>

// ──────────────────────────────────────────────────────────────────────────────
// BallsEffect — WLED mode_bouncing_balls (port completo)
// ──────────────────────────────────────────────────────────────────────────────
// Simulación física de pelotas rebotando con gravedad real (-9.81 m/s²).
// Cada bola tiene:
//   - Posición determinada por cinemática: h = 0.5*g*t² + v0*t
//   - Damping individual: d = 0.9 - i/N² (cada bola rebota menos)
//   - Velocidad de impacto aleatorizada al reiniciar
//
// Algoritmo WLED original, adaptado para SML con parámetros unificados.
//
// Parámetros:
//   speed     → Speed divisor (sx=64) — 255=normal, 0=más lento
//   intensity → Number of balls (ix=128) — 1-16 bolas (escala lineal)
//   custom1   → Max balls limit (c1=5) — cap de bolas para 24 LEDs
//   check1    → Random colors (m1=0) — colores aleatorios vs secuenciales
//   check2    → Overlay (m2=0) — no limpia el fondo
// ──────────────────────────────────────────────────────────────────────────────

class BallsEffect : public Effect {
private:
    static const char _meta[];

    // ── WLED Ball struct (12 bytes each) ────────────────────────────────────
    struct Ball {
        unsigned long lastBounceTime;
        float impactVelocity;
        float height;
    };

    static const unsigned MAX_BALLS = 16;
    Ball _balls[MAX_BALLS];
    float _gravity = -9.81f;

public:
    BallsEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);

        // Initialize all balls
        unsigned long now = millis();
        for (unsigned i = 0; i < MAX_BALLS; i++) {
            _balls[i].lastBounceTime = now;
            _balls[i].impactVelocity = 0.0f;
            _balls[i].height = 0.0f;
        }
    }

    const char* getMeta() const override {
        return _meta;
    }

    void render() override {
        if (numLeds <= 1) return;

        // ── WLED mode_bouncing_balls ────────────────────────────────────────
        // Number of balls: 1-16 based on intensity
        unsigned numBalls = ((unsigned)params.intensity * (MAX_BALLS - 1)) / 255 + 1;
        // Cap to custom1 for smaller strips
        uint8_t maxCap = params.custom1;
        if (maxCap < 1) maxCap = 1;
        if (maxCap > MAX_BALLS) maxCap = MAX_BALLS;
        if (numBalls > maxCap) numBalls = maxCap;

        // ── Background: clear or overlay ────────────────────────────────────
        if (!params.check2) {
            fill_solid(leds, numLeds, CRGB::Black);
        }

        const unsigned long time = millis();

        // ── Physics update & render per ball ────────────────────────────────
        for (unsigned i = 0; i < numBalls; i++) {
            // Time since last bounce, scaled by speed divisor
            // speed=255 → divisor=1 (real time)
            // speed=0   → divisor≈4 (4x slower)
            float speedDiv = ((255.0f - params.speed) / 64.0f) + 1.0f;
            float timeSinceBounce = (float)(time - _balls[i].lastBounceTime) / speedDiv;
            float timeSec = timeSinceBounce / 1000.0f;

            // ── Kinematic equation: h = 0.5*g*t² + v0*t ────────────────────
            // Avoid pow(x,2) — WLED optimization (extremely slow on ESP32)
            _balls[i].height = (0.5f * _gravity * timeSec + _balls[i].impactVelocity) * timeSec;

            // ── Bounce detection ────────────────────────────────────────────
            if (_balls[i].height <= 0.0f) {
                _balls[i].height = 0.0f;

                // Damping: each ball bounces less (0.9 - i/N²)
                float dampening = 0.9f - (float)i / (float)(numBalls * numBalls);
                _balls[i].impactVelocity = dampening * _balls[i].impactVelocity;
                _balls[i].lastBounceTime = time;

                // If velocity too low, reset with randomized start
                if (_balls[i].impactVelocity < 0.015f) {
                    float vStart = sqrtf(-2.0f * _gravity) * (float)random8(5, 11) / 10.0f;
                    _balls[i].impactVelocity = vStart;
                }
            } else if (_balls[i].height > 1.0f) {
                // Ball above strip — skip (don't draw OOB)
                continue;
            }

            // ── Color ──────────────────────────────────────────────────────
            CRGB color;
            if (params.check1) {
                // Random colors
                uint8_t hue = i * (256 / MAX_BALLS);
                color = CHSV(hue, 255, stripLed.brightness);
            } else {
                // Sequential colors: each ball gets a hue offset
                color = CHSV(uint8_t(i * 40), 255, stripLed.brightness);
            }

            // ── Position on strip ──────────────────────────────────────────
            int pos = roundf(_balls[i].height * (float)(numLeds - 1));
            pos = constrain(pos, 0, (int)numLeds - 1);

            leds[pos] = color;
        }

        FastLED.show();
    }
};

// Metadata: speed (gravity divisor), intensity (# of balls), custom1 (max balls cap), check1 (random), check2 (overlay)
const char BallsEffect::_meta[] =
    "Balls@Gravity,Balls,Max,,,Random,Overlay,,,,;;;;sx=64,ix=128,c1=5,m1=0,m2=0";
