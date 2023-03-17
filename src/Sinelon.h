#include "Arduino.h"

class Sinelon {
  public:
    Sinelon(){};
    void runPattern();
  private:
};

void Sinelon::runPattern() {

  const uint8_t THIS_BEAT = 23;
  const uint8_t THAT_BEAT = 28;
  const uint8_t THIS_FADE = 2; // How quickly does it fade? Lower = slower fade rate.

  fadeToBlackBy(leds, N_PIXELS, THIS_FADE);
  int pos1 = beatsin16(THIS_BEAT, 0, N_PIXELS - 1);
  int pos2 = beatsin16(THAT_BEAT, 0, N_PIXELS - 1);
  leds[(pos1 + pos2) / 2] += CHSV(myhue, 255, BRIGHTNESS);
  EVERY_N_MILLISECONDS(10) {
    myhue++;
  }
  
  FastLED.show();
}
