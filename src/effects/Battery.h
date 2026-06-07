#pragma once
#include "Effect.h"
#include "../state/AppState.h"

class ChargeEffect : public Effect {
public:
    ChargeEffect(CRGB* l, uint16_t n) : Effect(l, n) {}
    void render() override {
        FastLED.clear();
        int B = map(lvlCharge, 0, 100, 0, N_PIXELS);

        if (B <= 7) {
            meteorRain(0xff, 0, 0, 10, 64, true, 20, B);
        } else if (B <= 14) {
            meteorRain(0xff, 0xff, 0, 10, 96, true, 30, B);
        } else if (B < 24) {
            meteorRain(0, 0xff, 0, 10, 128, true, 45, B);
        } else {
            RunningLights(0x00, 0x00, 0xff, 50, B);
        }
    }

private:
    void setPixel(int Pixel, byte red, byte green, byte blue) {
        leds[Pixel] = CRGB(red, green, blue);
    }

    void setAll(byte red, byte green, byte blue, int count) {
        for (int i = 0; i < count; i++) {
            setPixel(i, red, green, blue);
        }
        FastLED.show();
    }

    void RunningLights(byte red, byte green, byte blue, int WaveDelay, int count) {
        int Position = 0;
        for (int j = 0; j < count * 2; j++) {
            for (int i = 0; i < count; i++) {
                float level = (sin(i + j) * 127 + 128) / 255.0f;
                setPixel(i, level * red, level * green, level * blue);
            }
            FastLED.show();
            vTaskDelay(pdMS_TO_TICKS(WaveDelay));
        }
    }

    void fadeToBlack(int ledNo, byte fadeValue) {
        leds[ledNo].fadeToBlackBy(fadeValue);
    }

    void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay, int count) {
        setAll(0, 0, 0, count);

        for (int i = 0; i < count + count; i++) {
            for (int j = 0; j < count; j++) {
                if ((!meteorRandomDecay) || (random(10) > 5)) {
                    fadeToBlack(j, meteorTrailDecay);
                }
            }

            for (int j = 0; j < meteorSize; j++) {
                if ((i - j < count) && (i - j >= 0)) {
                    setPixel(i - j, red, green, blue);
                }
            }

            FastLED.show();
            vTaskDelay(pdMS_TO_TICKS(SpeedDelay));
        }
    }
};
