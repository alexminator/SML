// src/Fire2012.h
#pragma once
#include "Effect.h"

class Fire2012 : public Effect {
private:
  uint8_t heat[24];
  CRGBPalette16 palette;

public:
  Fire2012(CRGB* l, uint16_t n) : Effect(l, n) {
    memset(heat, 0, sizeof(heat));
    palette = HeatColors_p;
  }

protected:
  void render() override {
    uint8_t cooling = map(params.intensity, 0, 255, 0, 250);
    uint8_t sparking = map(255 - params.speed, 0, 255, 0, 255);

    for (int i = 0; i < numLeds; i++) {
      heat[i] = qsub8(heat[i], random8(0, (cooling * 10) / numLeds + 2));
    }

    if (random8() < sparking) {
      int y = random8(7);
      heat[y] = qadd8(heat[y], random8(160, 255));
    }

    for (int i = 0; i < numLeds; i++) {
      uint8_t colorIndex = scale8(heat[i], 240);
      CRGB color = ColorFromPalette(palette, colorIndex);

      if (params.check1) {
        leds[numLeds - 1 - i] = color;
      } else {
        leds[i] = color;
      }
    }

    FastLED.show();
  }
};
