#pragma once
#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"
#include "../effects/PaletteManager.h"

// ──────────────────────────────────────────────────────────────────────────────
// GravimeterVU — Medidor de gravedad con el beat
// ──────────────────────────────────────────────────────────────────────────────
// Una o más bolas "rebotan" en la tira según el nivel de audio.
// Más volumen = más alto rebota. Inspirado en WLED-SR Gravimeter.
// Params:
//   speed     → gravedad (qué tan rápido cae)
//   intensity → número de bolas
// ──────────────────────────────────────────────────────────────────────────────

class GravimeterVUEffect : public VUEffect {
private:
    // int16_t to prevent wraparound when velocity is negative (uint8_t wraps 0-12 → 244)
    int16_t _ballPos[4] = {0, 0, 0, 0};
    int8_t  _ballVel[4] = {0, 0, 0, 0};
    uint8_t _ballCooldown[4] = {0, 0, 0, 0};  // prevents re-trigger on consecutive frames
    uint8_t _prevAudioLevel = 0;               // for rising-edge beat detection

public:
    static const char _meta[];

    GravimeterVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {
        setToDefaults(_meta);
    }

    const char* getMeta() const override { return _meta; }

    void render() override {
        uint8_t gravity = map(params.speed, 0, 255, 1, 10);
        uint8_t numBalls = map(params.intensity, 0, 255, 1, 4);
        if (numBalls < 1) numBalls = 1;
        if (numBalls > 4) numBalls = 4;

        uint16_t height = auxReading();
        uint8_t audioLevel = map(height, 0, TOP, 0, numLeds - 1);

        // Rising-edge detection: one launch per beat onset, not continuous
        bool beat = (audioLevel > 2 && _prevAudioLevel <= 2);
        _prevAudioLevel = audioLevel;

        fill_solid(leds, numLeds, CRGB::Black);

        for (uint8_t b = 0; b < numBalls; b++) {
            // Decrement cooldown
            if (_ballCooldown[b] > 0) _ballCooldown[b]--;

            // Apply gravity (pulls ball toward numLeds-1)
            _ballVel[b] += gravity;
            _ballPos[b] += _ballVel[b];

            // Audio impulse: ONLY on rising edge = one clean launch per beat
            // Cooldown prevents double-trigger within same beat envelope
            if (beat && _ballCooldown[b] == 0) {
                _ballVel[b] = -(audioLevel + random8(0, 5));
                _ballPos[b] = 0;
                _ballCooldown[b] = 8;
            }

            // Ground collision at bottom (numLeds - 1)
            if (_ballPos[b] >= numLeds - 1) {
                _ballPos[b] = numLeds - 1;
                _ballVel[b] = -(_ballVel[b] / 2);  // bounce with energy loss
                if (abs(_ballVel[b]) < 2) _ballVel[b] = 0;
            }

            // Ceiling at top
            if (_ballPos[b] < 0) {
                _ballPos[b] = 0;
                _ballVel[b] = 0;
            }

            // Draw ball with trail (safe cast — position already constrained above)
            uint16_t pos = (uint16_t)_ballPos[b];
            uint8_t hue = (b * 64) % 255;
            CRGBPalette16 pal = PaletteManager::getPalette(_paletteIndex);
            leds[pos] = ColorFromPalette(pal, hue, 255, LINEARBLEND);

            if (pos > 0) leds[pos - 1] += ColorFromPalette(pal, hue, 128, LINEARBLEND);
            if (pos > 1) leds[pos - 2] += ColorFromPalette(pal, hue, 64, LINEARBLEND);
        }

        dropPeak();
        averageReadings();
        FastLED.setBrightness(stripLed.brightness);
        FastLED.show();
    }
};

const char GravimeterVUEffect::_meta[] =
    "Gravimeter@Gravity,Balls;;;;sx=128,ix=128";
