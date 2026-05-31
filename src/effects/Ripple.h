#pragma once
#include "Effect.h"
#include "../state/AppState.h"

class RippleEffect : public Effect {
private:
    uint8_t _rippleColor = 0;
    uint8_t _rippleCenter = 0;
    int _rippleStep = -1;

public:
    RippleEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        params.custom1 = 3;    // rippleSize default
        params.check1  = false; // rippleMirror default
    }

    void render() override {
        const float RIPPLE_FADE_RATE = 0.80;
        const uint8_t MAX_STEPS = 16;
        uint8_t size    = params.custom1;
        bool mirror     = params.check1;

        // Background color evolution
        EVERY_N_MILLISECONDS(10) {
            myhue++;
        }

        EVERY_N_MILLISECONDS(50) {
            // Start new ripple
            if (_rippleStep == -1) {
                _rippleCenter = random(N_PIXELS);
                _rippleColor = myhue + 128;
                _rippleStep = 0;
            }

            if (_rippleStep == 0) {
                // Initial ripple center
                leds[_rippleCenter] = CHSV(_rippleColor, 255, stripLed.brightness);

                // Mirror mode: create second ripple at opposite side
                if (mirror) {
                    int mirrorCenter = (_rippleCenter + N_PIXELS / 2) % N_PIXELS;
                    leds[mirrorCenter] = CHSV(_rippleColor, 255, stripLed.brightness);
                }

                _rippleStep++;
            } else {
                int stepIncrement = map8(size, 1, 3);

                if (_rippleStep < MAX_STEPS) {
                    int actualStep = _rippleStep * stepIncrement;

                    uint8_t brightness1 = pow(RIPPLE_FADE_RATE, _rippleStep) * 255;
                    leds[wrap(_rippleCenter + actualStep)] = CHSV(_rippleColor, 255, brightness1);
                    leds[wrap(_rippleCenter - actualStep)] = CHSV(_rippleColor, 255, brightness1);

                    if (mirror) {
                        int mirrorCenter = (_rippleCenter + N_PIXELS / 2) % N_PIXELS;
                        leds[wrap(mirrorCenter + actualStep)] = CHSV(_rippleColor, 255, brightness1);
                        leds[wrap(mirrorCenter - actualStep)] = CHSV(_rippleColor, 255, brightness1);
                    }

                    if (_rippleStep > 3) {
                        uint8_t brightness2 = pow(RIPPLE_FADE_RATE, _rippleStep - 2) * 255;
                        leds[wrap(_rippleCenter + actualStep - 3)] = CHSV(_rippleColor, 255, brightness2);
                        leds[wrap(_rippleCenter - actualStep + 3)] = CHSV(_rippleColor, 255, brightness2);

                        if (mirror) {
                            int mirrorCenter = (_rippleCenter + N_PIXELS / 2) % N_PIXELS;
                            leds[wrap(mirrorCenter + actualStep - 3)] = CHSV(_rippleColor, 255, brightness2);
                            leds[wrap(mirrorCenter - actualStep + 3)] = CHSV(_rippleColor, 255, brightness2);
                        }
                    }
                    _rippleStep++;
                } else {
                    _rippleStep = -1;
                }
            }
        }

        FastLED.show();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

private:
    int wrap(int rippleStep) {
        if (rippleStep < 0) return N_PIXELS + rippleStep;
        if (rippleStep > N_PIXELS - 1) return rippleStep - N_PIXELS;
        return rippleStep;
    }
};
