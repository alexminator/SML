#include "Globals.h"
class RedWhiteBlue {
  public:
    RedWhiteBlue(){};
    void runPattern();
  private:
};

void RedWhiteBlue::runPattern() {

  uint16_t sinBeat   = beatsin16(30, 0, N_PIXELS - 1, 0, 0);
  uint16_t sinBeat2  = beatsin16(30, 0, N_PIXELS - 1, 0, 21845);
  uint16_t sinBeat3  = beatsin16(30, 0, N_PIXELS - 1, 0, 43690);

  leds[sinBeat]   = CHSV(160,255,stripLed.brightness); //blue
  leds[sinBeat2]   = CHSV(0,255,stripLed.brightness); //red
  leds[sinBeat3]   = CHSV(0,0,stripLed.brightness); //white
  
  fadeToBlackBy(leds, N_PIXELS, 10);

  FastLED.show();
}
