#pragma once
#include "Effect.h"
#include "../state/AppState.h"

class ChargeEffect : public Effect {
public:
    ChargeEffect(CRGB* l, uint16_t n) : Effect(l, n), _pos(0) {}

    void render() override {
        int B = map(lvlCharge, 0, 100, 0, N_PIXELS);

        if (B <= 7) {
            meteorRainFrame(0xff, 0, 0, 10, 64, true, B);
        } else if (B <= 14) {
            meteorRainFrame(0xff, 0xff, 0, 10, 96, true, B);
        } else if (B < 24) {
            meteorRainFrame(0, 0xff, 0, 10, 128, true, B);
        } else {
            runningLightsFrame(0x00, 0x00, 0xff, B);
        }

        _pos++;
    }

private:
    uint16_t _pos;

    void setPixel(int Pixel, byte red, byte green, byte blue) {
        leds[Pixel] = CRGB(red, green, blue);
    }

    void fadeToBlack(int ledNo, byte fadeValue) {
        leds[ledNo].fadeToBlackBy(fadeValue);
    }

    /// One frame of meteor-rain. No vTaskDelay — returns immediately.
    void meteorRainFrame(byte red, byte green, byte blue, byte meteorSize,
                         byte meteorTrailDecay, boolean meteorRandomDecay,
                         int count) {
        int maxPos = (count + count);
        if (maxPos < 2) maxPos = 2;
        int pos = _pos % maxPos;

        for (int j = 0; j < count; j++) {
            if ((!meteorRandomDecay) || (random(10) > 5)) {
                fadeToBlack(j, meteorTrailDecay);
            }
        }
        for (int j = 0; j < meteorSize; j++) {
            int idx = pos - j;
            if (idx >= 0 && idx < count) {
                setPixel(idx, red, green, blue);
            }
        }
        FastLED.show();
    }

    /// One frame of running-lights. No vTaskDelay — returns immediately.
    void runningLightsFrame(byte red, byte green, byte blue, int count) {
        FastLED.clear();
        int maxPos = (count * 2);
        if (maxPos < 2) maxPos = 2;
        int pos = _pos % maxPos;

        for (int i = 0; i < count; i++) {
            float level = (sin(i + pos) * 127 + 128) / 255.0f;
            setPixel(i, level * red, level * green, level * blue);
        }
        FastLED.show();
    }
};
