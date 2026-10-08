#pragma once
#include "../effects/Effect.h"
#include "../state/AppState.h"
#include "VUEffect.h"
#include "../effects/PaletteManager.h"

// VU: Ripple (mono) with or without background
class RippleVUEffect : public VUEffect {
public:
    RippleVUEffect(CRGB* l, uint16_t n) : VUEffect(l, n) {}
    void render() override {
        EVERY_N_MILLISECONDS(1000) {
            peakspersec = peakcount;
            peakcount = 0;
        }

        soundmems();

        EVERY_N_MILLISECONDS(20) {
            ripple3(true);
        }

        FastLED.setBrightness(stripLed.brightness);
        FastLED.show();
    }

private:
    static constexpr float RIPPLE_FADE_RATE = 0.80;
    static constexpr uint8_t MAX_STEPS = 16;

    int rippleStep = -1;
    int peakspersec = 0;
    int peakcount = 0;
    unsigned int sampleavg = 0;
    uint8_t rippleHue = 0;
    uint8_t bgcol = 0;

    void ripple3(bool show_background) {
        static int center = 0;

        if (show_background) {
            for (int i = 0; i < N_PIXELS; i++) {
                leds[i] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), bgcol, sampleavg * 2, LINEARBLEND);
            }
        } else {
            fadeToBlackBy(leds, N_PIXELS, 64);
        }

        switch (rippleStep) {
            case -1:
                center = random(N_PIXELS);
                rippleHue = (peakspersec * 10) % 255;
                rippleStep = 0;
                bgcol = bgcol + 8;
                break;

            case 0:
                leds[center] = ColorFromPalette(PaletteManager::getPalette(_paletteIndex), rippleHue, 255, LINEARBLEND);
                rippleStep++;
                break;

            case MAX_STEPS:
                break;

            default:
                leds[(center + rippleStep + N_PIXELS) % N_PIXELS] += ColorFromPalette(PaletteManager::getPalette(_paletteIndex), rippleHue, 255 / (rippleStep * 2), LINEARBLEND);
                leds[(center - rippleStep + N_PIXELS) % N_PIXELS] += ColorFromPalette(PaletteManager::getPalette(_paletteIndex), rippleHue, 255 / (rippleStep * 2), LINEARBLEND);
                rippleStep++;
                break;
        }
    }

    void soundmems() {
        static int samplecount;
        static unsigned long samplesum;
        static unsigned long oldtime;
        unsigned long newtime = millis();
        unsigned int sample = abs((analogRead(AUDIO_IN_PIN)) - BIAS);

        samplesum = samplesum + sample - _vol[samplecount];
        sampleavg = samplesum / SAMPLES;
        _vol[samplecount] = sample;
        samplecount = (samplecount + 1) % SAMPLES;

        if ((sample > (sampleavg + 50)) && (newtime > (oldtime + 100))) {
            rippleStep = -1;
            peakcount++;
            oldtime = newtime;
        }
    }
};
