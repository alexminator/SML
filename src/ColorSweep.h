#pragma once
#include "Effect.h"
#include "Settings.h"

// WLED-based Color Sweep effect
class ColorSweepEffect : public Effect {
private:
    uint16_t _index = 0;
    bool _forward = true;
    unsigned long _lastTime = 0;

public:
    ColorSweepEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        params.speed = 128;  // sweepSpeed default
    }

    void render() override {
        uint8_t speedVal = params.speed;
        unsigned long currentTime = millis();
        uint32_t cycleTime = (256 - speedVal) * 2;

        if (currentTime - _lastTime >= cycleTime) {
            _lastTime = currentTime;

            int prevIndex = _forward ? (_index - 1 + N_PIXELS) % N_PIXELS : (_index + 1) % N_PIXELS;
            leds[prevIndex] = CRGB::Black;

            leds[_index] = CHSV(myhue, 255, stripLed.brightness);

            if (_forward) {
                _index++;
                if (_index >= N_PIXELS) {
                    _index = N_PIXELS - 1;
                    _forward = false;
                    myhue++;
                }
            } else {
                if (_index == 0) {
                    _index = 0;
                    _forward = true;
                    myhue++;
                } else {
                    _index--;
                }
            }
        }

        FastLED.show();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
};
