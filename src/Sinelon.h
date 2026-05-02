// src/Sinelon.h
#pragma once
#include "Effect.h"

class Sinelon : public Effect {
private:
  CRGBPalette16 palette;
  uint8_t hue;

public:
  Sinelon(CRGB* l, uint16_t n) : Effect(l, n) {
    palette = PartyColors_p;
    hue = 0;
  }

protected:
  void render() override {
    uint8_t pos = beatsin16(255 - params.speed, 0, numLeds - 1);
    uint8_t fade = map(params.intensity, 0, 255, 8, 224);
    fadeToBlackBy(leds, numLeds, fade);

    leds[pos] = ColorFromPalette(palette, hue, 255, LINEARBLEND);

    EVERY_N_MILLIS(20) {
      hue++;
    }

    FastLED.show();
  }
};
