#pragma once
#include "Effect.h"
#include "../state/AppState.h"

// WLED-based Breath effect - "standby-breathing" of well known i-Devices
class BreathEffect : public Effect {
public:
    BreathEffect(CRGB* l, uint16_t n) : Effect(l, n) {
        params.speed = 128;  // breathSpeed default
    }

    void render() override {
        uint8_t speedVal = params.speed;
        unsigned long currentTime = millis();
        unsigned counter = (currentTime * ((speedVal >> 3) + 10)) & 0xFFFFU;

        byte breathIntensity = sin8(counter >> 8);

        for (int i = 0; i < N_PIXELS; i++) {
            leds[i] = CHSV(myhue, 255, map(breathIntensity, 0, 255, 0, stripLed.brightness));
        }

        EVERY_N_MILLISECONDS(1000) {
            myhue++;
        }

        FastLED.show();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
};
