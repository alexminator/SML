#pragma once
#include "Effect.h"
#include "../state/AppState.h"
#include "PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// HalloweenEyes — Ojos espeluznantes
// ──────────────────────────────────────────────────────────────────────────────
// Pares de ojos (blanco con pupila negra) que se mueven lateralmente.
// Inspirado en el efecto de WLED.
// Params:
//   speed     → velocidad de movimiento
//   intensity → cantidad de pares de ojos
// ──────────────────────────────────────────────────────────────────────────────

class HalloweenEyesEffect : public Effect {
private:
    int8_t  _eyePos = 0;      // 0 = center, negative = left, positive = right
    int8_t  _direction = 1;   // 1 = moving right, -1 = moving left
    uint8_t _blinkTimer = 0;
    bool    _blinking = false;

public:
    static const char _meta[];

    HalloweenEyesEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        uint8_t speed = map(params.speed, 0, 255, 20, 2);
        uint8_t numEyes = map(params.intensity, 0, 255, 1, 3);
        if (numEyes < 1) numEyes = 1;
        if (numEyes > numLeds / 2) numEyes = numLeds / 2;

        fill_solid(leds, numLeds, CRGB::Black);

        // Eye movement
        EVERY_N_MILLISECONDS(speed) {
            _eyePos += _direction;
            if (_eyePos > 3) _direction = -1;
            if (_eyePos < -3) _direction = 1;

            // Random blink
            if (random8() < 5) {
                _blinking = true;
                _blinkTimer = 5;
            }
        }

        // Blink timer
        if (_blinking) {
            _blinkTimer--;
            if (_blinkTimer == 0) _blinking = false;
        }

        // Calculate eye positions evenly spaced on strip
        uint16_t spacing = numLeds / numEyes;

        for (uint8_t e = 0; e < numEyes; e++) {
            uint16_t center = spacing * e + spacing / 2;
            if (center >= numLeds) break;

            // Limit position within bounds
            int16_t leftEye  = center + _eyePos - 1;
            int16_t rightEye = center + _eyePos + 1;

            CRGB eyeColor = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), 0, 255, LINEARBLEND);
            if (leftEye >= 0 && leftEye < numLeds && !_blinking) {
                leds[leftEye] = eyeColor;  // Eye color from palette
            }
            if (rightEye >= 0 && rightEye < numLeds && !_blinking) {
                leds[rightEye] = eyeColor; // Eye color from palette
            }

            // Pupil (black dot in center of eye pair)
            int16_t pupil = center + _eyePos;
            if (pupil >= 0 && pupil < numLeds) {
                leds[pupil] = CRGB::Black;    // Pupil
            }
        }

        FastLED.show();
    }
};

const char HalloweenEyesEffect::_meta[] =
    "Hallow Eyes@Speed,Eye Pairs;;;;sx=128,ix=128,pa=25";
