
#include "Settings.h"
class Juggle {
  public:
    Juggle(){};
    void runPattern();
  private:
};



void Juggle::runPattern() {
  // WLED Juggle algorithm - eight colored dots weaving in and out of sync
  // Enhanced with configurable number of dots (better than WLED's 8 hardcoded)

  // Fade based on intensity (WLED: 192 - (3*intensity/4))
  uint8_t fadeAmount = 192 - (3 * juggleIntensity / 4);
  fadeToBlackBy(leds, N_PIXELS, fadeAmount);

  byte dothue = 0;

  // Use configurable number of dots (better than WLED's hardcoded 8)
  for (int i = 0; i < juggleDots; i++) {
    // WLED: beatsin88_t((16 + SEGMENT.speed)*(i + 7), 0, SEGLEN -1)
    // We use beatsin16 with configurable speed
    int index = beatsin16((16 + juggleSpeed) * (i + 7), 0, N_PIXELS - 1);

    // Additive color blending for brighter overlap
    leds[index] |= CHSV(dothue, 220, stripLed.brightness);

    dothue += 32; // WLED uses 32 for hue increment
  }

  FastLED.show();
}
