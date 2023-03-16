#include "Arduino.h"

class Rainbow {
public:
  Rainbow(){};
  void runPattern();
private:
};

void Rainbow::runPattern() {
  uint8_t rate;
  static uint8_t hueNow = 0;
  fill_rainbow(leds, N_PIXELS, hueNow, 7);
  EVERY_N_MILLISECONDS(20)
  {
    hueNow = (hueNow + rate) % 255;
  }
  FastLED.show();
}