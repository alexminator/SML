
#include "Globals.h"
class Ripple {
  public:
    Ripple(){};
    void runPattern();
  private:
};

int wrap(int rippleStep) {
  if (rippleStep < 0) return N_PIXELS + rippleStep;
  if (rippleStep > N_PIXELS - 1) return rippleStep - N_PIXELS;
  return rippleStep;
}

void Ripple::runPattern() {
  const float RIPPLE_FADE_RATE = 0.80;
  const uint8_t MAX_STEPS = 16;
  static uint8_t rippleColor = 0;
  static uint8_t rippleCenter = 0;
  static int rippleStep = -1;

  // Background color evolution
  EVERY_N_MILLISECONDS(10) {
    myhue++;
  }

  EVERY_N_MILLISECONDS(50) {
    // Start new ripple
    if (rippleStep == -1) {
      rippleCenter = random(N_PIXELS);
      rippleColor = myhue + 128;
      rippleStep = 0;
    }

    if (rippleStep == 0) {
      // Initial ripple center
      leds[rippleCenter] = CHSV(rippleColor, 255, stripLed.brightness);

      // Mirror mode: create second ripple at opposite side
      if (rippleMirror) {
        int mirrorCenter = (rippleCenter + N_PIXELS / 2) % N_PIXELS;
        leds[mirrorCenter] = CHSV(rippleColor, 255, stripLed.brightness);
      }

      rippleStep++;
    } else {
      // Calculate step size based on rippleSize parameter (0-7)
      int stepIncrement = map8(rippleSize, 1, 3);

      if (rippleStep < MAX_STEPS) {
        // Apply ripple with configurable size
        int actualStep = rippleStep * stepIncrement;

        // Main ripple expansion
        uint8_t brightness1 = pow(RIPPLE_FADE_RATE, rippleStep) * 255;
        leds[wrap(rippleCenter + actualStep)] = CHSV(rippleColor, 255, brightness1);
        leds[wrap(rippleCenter - actualStep)] = CHSV(rippleColor, 255, brightness1);

        // Mirror mode ripple
        if (rippleMirror) {
          int mirrorCenter = (rippleCenter + N_PIXELS / 2) % N_PIXELS;
          leds[wrap(mirrorCenter + actualStep)] = CHSV(rippleColor, 255, brightness1);
          leds[wrap(mirrorCenter - actualStep)] = CHSV(rippleColor, 255, brightness1);
        }

        // Fade previous ripples
        if (rippleStep > 3) {
          uint8_t brightness2 = pow(RIPPLE_FADE_RATE, rippleStep - 2) * 255;
          leds[wrap(rippleCenter + actualStep - 3)] = CHSV(rippleColor, 255, brightness2);
          leds[wrap(rippleCenter - actualStep + 3)] = CHSV(rippleColor, 255, brightness2);

          if (rippleMirror) {
            int mirrorCenter = (rippleCenter + N_PIXELS / 2) % N_PIXELS;
            leds[wrap(mirrorCenter + actualStep - 3)] = CHSV(rippleColor, 255, brightness2);
            leds[wrap(mirrorCenter - actualStep + 3)] = CHSV(rippleColor, 255, brightness2);
          }
        }
        rippleStep++;
      } else {
        rippleStep = -1; // Reset for next ripple
      }
    }
  }

  FastLED.show();
  vTaskDelay(pdMS_TO_TICKS(50));
}


