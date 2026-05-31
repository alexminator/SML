#pragma once
#include "Effect.h"
#include "../state/AppState.h"

class CometEffect : public Effect {
private:
    byte _hue = HUE_RED;
    int _iDirection = 1;
    double _iPos = 0.0;
    unsigned long _lastUpdate = 0;

public:
    CometEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        params.speed    = 8;   // cometSpeed default
        params.custom1  = 4;   // cometTrail default
        params.check1   = false; // cometBlur default
    }

    void render() override {
        uint8_t speedVal = params.speed;
        uint8_t trail    = params.custom1;
        bool blur        = params.check1;

        unsigned long delayMs = map(speedVal, 0, 255, 200, 30);
        unsigned long currentTime = millis();

        if (currentTime - _lastUpdate >= delayMs) {
            _lastUpdate = currentTime;

            int cometSize = map(trail, 0, 10, 2, 8);
            const int deltaHue = 4;

            _hue += deltaHue;
            _iPos += _iDirection * (speedVal / 16.0);

            if (_iPos >= (N_PIXELS - cometSize) || _iPos <= 0) {
                _iDirection *= -1;
            }

            if (blur) {
                fadeToBlackBy(leds, N_PIXELS, 30);
            } else {
                fadeToBlackBy(leds, N_PIXELS, 80);
            }

            for (int i = 0; i < cometSize; i++) {
                int pos = (int)_iPos + i;
                if (pos >= 0 && pos < N_PIXELS) {
                    uint8_t intensity = map(i, 0, cometSize - 1, 255, 50);
                    leds[pos] = CHSV(_hue, 255, stripLed.brightness * intensity / 255);
                }
            }

            FastLED.show();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
};
