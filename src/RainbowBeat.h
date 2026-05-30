#include "Settings.h"

class RainbowBeat {
  public:
    RainbowBeat(){};
    void runPattern();
  private:
};

void RainbowBeat::runPattern() {
  // WLED Rainbow algorithm with configurable speed and delta
  uint16_t beatA = beatsin16(rainbowSpeed, 0, 255);
  uint16_t beatB = beatsin16(rainbowSpeed - 10, 0, 255);
  fill_rainbow(leds, N_PIXELS, (beatA + beatB) / 2, rainbowDelta);

  FastLED.show();
}
