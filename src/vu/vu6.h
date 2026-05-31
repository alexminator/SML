#pragma once
#include "../effects/Effect.h"
#include "../Settings.h"

// VU: Ocean waves (stereo)
class OceanVUEffect : public Effect {
public:
    OceanVUEffect(CRGB* l, uint16_t n) : Effect(l, n) {}
    void render() override {
        currentPalette = PartyColors_p;
        EVERY_N_SECONDS(5) {
            for (int i = 0; i < 16; i++) {
                targetPalette[i] = CHSV(random8(), 255, 255);
            }
        }

        EVERY_N_MILLISECONDS(100) {
            uint8_t maxChanges = 24;
            nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);
        }

        EVERY_N_MILLIS_I(thistimer, 20) {
            uint8_t timeval = beatsin8(10, 20, 50);
            thistimer.setPeriod(timeval);
            fadeToBlackBy(leds, N_PIXELS, 16);
            sndwave();
        }

        FastLED.setBrightness(stripLed.brightness);
        FastLED.show();
    }

private:
    void sndwave() {
        int n = analogRead(AUDIO_IN_PIN);
        int sampleLeft = abs(n - BIAS - DC_OFFSET);
        leds[N_PIXELS_HALF] = ColorFromPalette(currentPalette, sampleLeft, sampleLeft * 2, LINEARBLEND);

        for (int i = N_PIXELS - 1; i > N_PIXELS_HALF; i--) {
            leds[i] = leds[i - 1];
        }

        for (int i = 0; i < N_PIXELS_HALF; i++) {
            leds[i] = leds[i + 1];
        }
    }
};
