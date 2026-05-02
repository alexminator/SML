// src/BouncingBalls.h
#pragma once
#include "Effect.h"

class BouncingBalls : public Effect {
private:
  struct Ball {
    float height;
    float velocity;
    float impactVelocity;
    CRGB color;
  };

  static const float GRAVITY = -9.81;
  static const float BOUNCE_DAMPING = 0.7;

  Ball balls[16];
  uint8_t numBalls;

  void initBalls() {
    numBalls = map(params.intensity, 0, 255, 1, 16);

    for (uint8_t i = 0; i < numBalls; i++) {
      balls[i].height = numLeds - 1;
      balls[i].velocity = 0;
      balls[i].impactVelocity = 0;

      if (params.check1) {
        balls[i].color = CHSV(random8(), 255, 255);
      } else {
        balls[i].color = CHSV(i * 256 / numBalls, 255, 255);
      }
    }
  }

public:
  BouncingBalls(CRGB* l, uint16_t n) : Effect(l, n) {
    initBalls();
  }

protected:
  void render() override {
    fadeToBlackBy(leds, numLeds, 20);

    const float timeSinceLastFrame = 0.016;

    for (uint8_t i = 0; i < numBalls; i++) {
      balls[i].velocity += GRAVITY * timeSinceLastFrame;
      balls[i].height += balls[i].velocity * timeSinceLastFrame;

      if (balls[i].height < 0) {
        balls[i].height = 0;
        balls[i].velocity = -balls[i].velocity * BOUNCE_DAMPING;

        if (abs(balls[i].velocity) < 0.5) {
          balls[i].velocity = 0;
        }
      }

      uint8_t ledPos = constrain(balls[i].height, 0, numLeds - 1);
      leds[ledPos] = balls[i].color;
    }

    FastLED.show();
  }
};
