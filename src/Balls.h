#pragma once
#include "Effect.h"
#include "Settings.h"
#include <math.h>

class BallsEffect : public Effect {
private:
    // Physics state — member variables instead of globals
    float  _h[NUM_BALLS];
    float  _vImpact[NUM_BALLS];
    float  _tCycle[NUM_BALLS];
    int    _pos[NUM_BALLS];
    long   _tLast[NUM_BALLS];
    float  _COR[NUM_BALLS];

public:
    BallsEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        params.custom1 = 3;  // ballsCount default
        params.check1  = false; // randomColors default

        float v0 = sqrt(-2.0f * GRAVITY * h0);
        for (int i = 0; i < NUM_BALLS; i++) {
            _tLast[i]   = millis();
            _h[i]       = h0;
            _pos[i]     = 0;
            _vImpact[i] = v0;
            _tCycle[i]  = 0;
            _COR[i]     = 0.90f - float(i) / pow(NUM_BALLS, 2);
        }
    }

    void render() override {
        uint8_t count = params.custom1;
        if (count > NUM_BALLS) count = NUM_BALLS;
        bool randomColors = params.check1;

        for (int i = 0; i < count; i++) {
            _tCycle[i] = millis() - _tLast[i];
            _h[i] = 0.5 * GRAVITY * pow(_tCycle[i] / 1000.0, 2.0) + _vImpact[i] * _tCycle[i] / 1000.0;

            if (_h[i] < 0) {
                _h[i] = 0;
                _vImpact[i] = _COR[i] * _vImpact[i];
                _tLast[i] = millis();
                if (_vImpact[i] < 0.01) {
                    _vImpact[i] = sqrt(-2.0f * GRAVITY * h0);
                }
            }
            _pos[i] = round(_h[i] * (N_PIXELS - 1) / h0);
        }

        for (int i = 0; i < count; i++) {
            if (randomColors) {
                leds[_pos[i]] = CHSV(random8(), 255, stripLed.brightness);
            } else {
                leds[_pos[i]] = CHSV(uint8_t(i * 40), 255, stripLed.brightness);
            }
        }
        FastLED.show();
        for (int i = 0; i < count; i++) {
            leds[_pos[i]] = CRGB::Black;
        }
    }
};
